import ctypes
import json
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch
import re4_graphics_capture as capture
from re4_reader import ReaderError


class ApiFunction:
    def __init__(self,callback):self.callback=callback
    def __call__(self,*args):return self.callback(*args)


class Kernel:
    def __init__(self,words,absent=False,map_fail=False,extended_fail=False):
        self.buffer=ctypes.create_string_buffer(struct.pack('<'+str(len(words))+'I',*words))
        self.closed=0;self.unmapped=0;self.viewed=0
        def open_mapping(access,inherit,name):
            assert access==4 and not inherit and name=='Local\\RE4Bridge-GraphicsCapture-42'
            return 0 if absent else 1
        def map_view(handle,access,high,low,size):
            assert handle==1 and access==4 and high==low==0 and size in (96,len(words)*4)
            self.viewed+=1
            return 0 if map_fail or (extended_fail and size>96) else ctypes.addressof(self.buffer)
        def unmap(view):
            assert view==ctypes.addressof(self.buffer);self.unmapped+=1;return 1
        def close(handle):assert handle==1;self.closed+=1;return 1
        self.OpenFileMappingW=ApiFunction(open_mapping)
        self.MapViewOfFile=ApiFunction(map_view)
        self.UnmapViewOfFile=ApiFunction(unmap)
        self.CloseHandle=ApiFunction(close)


def snapshot():
    return [capture.MAGIC,1,2,415,42,1,1,1,1,0,2,1,1,0x8876086c,0x1010,0x2020,
            0,1,0x40,1,1366,768,0x3030,0]


class GraphicsCaptureTests(unittest.TestCase):
    def test_v3_paths_snapshot_and_validation(self):
        data=bytearray(capture.SIZES[3])
        words=snapshot();words[1]=3;words[3]=417
        struct.pack_into('<24I',data,0,*words)
        struct.pack_into('<18I',data,96,1,1,1,1,0,0x700,0x2000,0x2000,0x80,65536,
                         1,1,0,0x700,0x2000,0x2000,0x80,65536)
        paths=['C:\\WINDOWS\\System32\\d3d9.dll','C:\\Windows\\SysWOW64\\d3d9.dll','C:\\Windows\\System32\\d3d9.dll']
        for index in range(2):
            start=168+index*1564;struct.pack_into('<I',data,start,2)
            for n,value in enumerate(paths):
                encoded=(value+'\0').encode('utf-16-le');offset=start+4+n*520
                data[offset:offset+len(encoded)]=encoded
        kernel=Kernel(struct.unpack('<824I',data));before=kernel.buffer.raw
        report=capture.read_capture(kernel,42)
        self.assertEqual(report['build'],'0.4.17')
        self.assertEqual(report['installation']['latest']['paths']['loaded'],paths[0])
        self.assertEqual(report['installation']['initial']['paths']['matched'],'SystemDirectory')
        self.assertEqual(kernel.buffer.raw,before)
        self.assertEqual((kernel.viewed,kernel.unmapped,kernel.closed),(2,2,1))
        struct.pack_into('<I',data,12,418)
        newer=capture.decode_capture(bytes(data),42)
        self.assertEqual(newer['build'],'0.4.18');self.assertIsNone(newer['rendering_enabled'])
        struct.pack_into('<I',data,12,419)
        newer=capture.decode_capture(bytes(data),42)
        self.assertEqual(newer['build'],'0.4.19');self.assertIsNone(newer['rendering_enabled'])
        struct.pack_into('<I',data,12,421)
        newer=capture.decode_capture(bytes(data),42)
        self.assertEqual(newer['build'],'0.4.21');self.assertIsNone(newer['rendering_enabled'])
        struct.pack_into('<I',data,12,422)
        newer=capture.decode_capture(bytes(data),42)
        self.assertEqual(newer['build'],'0.4.22');self.assertIsNone(newer['rendering_enabled'])
        struct.pack_into('<I',data,12,423)
        newer=capture.decode_capture(bytes(data),42)
        self.assertEqual(newer['build'],'0.4.23');self.assertIsNone(newer['rendering_enabled'])
        struct.pack_into('<I',data,12,424)
        newer=capture.decode_capture(bytes(data),42)
        self.assertEqual(newer['build'],'0.4.24');self.assertIsNone(newer['rendering_enabled'])
        for build in (426,427,428,429,430):
            struct.pack_into('<I',data,12,build)
            newer=capture.decode_capture(bytes(data),42)
            self.assertEqual(newer['build'],f'0.4.{build-400}');self.assertIsNone(newer['rendering_enabled'])
        for offset,replacement in [(168,b'\3\0\0\0'),(172,b'x\0'*260),(172,b'\0\xd8')]:
            invalid=bytearray(data);invalid[offset:offset+len(replacement)]=replacement
            with self.assertRaises(ReaderError):capture.decode_capture(bytes(invalid),42)

    def test_v2_installer_journal_and_readonly_remap(self):
        words=snapshot();words[1]=2;words[3]=416
        words += [2,2,3,15,0,0x700,0x800,0x20002000,0x80,65536,
                  1,1,0,0x700,0x20002000,0x20002000,0x80,65536]
        kernel=Kernel(words);before=kernel.buffer.raw
        report=capture.read_capture(kernel,42)
        self.assertEqual(report['build'],'0.4.16')
        self.assertEqual(report['installation']['initial']['reason_code'],15)
        self.assertEqual(report['installation']['initial']['observed'],'0x800')
        self.assertEqual(report['installation']['latest']['status'],1)
        self.assertEqual(report['installation']['phase'],'DirectInput8Create')
        self.assertEqual(kernel.buffer.raw,before)
        self.assertEqual((kernel.viewed,kernel.unmapped,kernel.closed),(2,2,1))
        failed=Kernel(words,extended_fail=True)
        with self.assertRaisesRegex(ReaderError,'extendido'):capture.read_capture(failed,42)
        self.assertEqual((failed.viewed,failed.unmapped,failed.closed),(2,1,1))
        for index,value in [(24,3),(25,3),(27,99),(34,99)]:
            invalid=list(words);invalid[index]=value
            with self.assertRaises(ReaderError):capture.decode_capture(struct.pack('<42I',*invalid),42)

    def test_readonly_report_and_handle_cleanup(self):
        kernel=Kernel(snapshot());before=kernel.buffer.raw
        report=capture.read_capture(kernel,42)
        self.assertTrue(report['creation_observed'])
        self.assertFalse(report['rendering_enabled'])
        self.assertEqual(report['last_hresult'],'0x8876086c')
        self.assertEqual(report['last_device_address'],'0x00002020')
        self.assertEqual((report['width'],report['height']),(1366,768))
        self.assertEqual(kernel.buffer.raw,before)
        self.assertEqual((kernel.unmapped,kernel.closed),(1,1))

    def test_invalid_header_pid_build_busy_and_unavailable_mapping(self):
        for index,value in [(0,0),(1,2),(2,3),(3,414),(4,43),(5,99)]:
            with self.subTest(index=index):
                words=snapshot();words[index]=value;kernel=Kernel(words)
                with self.assertRaises(ReaderError):capture.read_capture(kernel,42)
                self.assertEqual((kernel.unmapped,kernel.closed),(1,1))
        for absent,map_fail in [(True,False),(False,True)]:
            kernel=Kernel(snapshot(),absent,map_fail)
            with self.assertRaises(ReaderError):capture.read_capture(kernel,42)
            self.assertEqual(kernel.closed,0 if absent else 1)
            self.assertEqual(kernel.unmapped,0)

    def test_changing_snapshot_retries_before_acceptance(self):
        kernel=Kernel(snapshot());view=ctypes.addressof(kernel.buffer)
        actual=ctypes.string_at;sequence_reads=0
        def changing(address,size):
            nonlocal sequence_reads
            if address==view+8:
                sequence_reads+=1
                if sequence_reads==2:struct.pack_into('<I',kernel.buffer,8,4)
            return actual(address,size)
        with patch.object(capture.ctypes,'string_at',side_effect=changing):
            self.assertTrue(capture.read_capture(kernel,42)['creation_observed'])
        self.assertEqual(sequence_reads,4)
        self.assertEqual((kernel.closed,kernel.unmapped),(1,1))

    def test_main_writes_success_or_error_report_and_closes_process(self):
        class Process:
            def __init__(self,k):self.k=k;self.closed=False
            def close(self):self.closed=True
        for failed in [False,True]:
            with tempfile.TemporaryDirectory() as directory:
                process=Process(Kernel(snapshot(),absent=failed))
                with patch.object(capture,'find_pid',return_value=42),patch.object(capture,'WindowsProcess',return_value=process):
                    self.assertEqual(capture.main(['--output-dir',directory]),int(failed))
                self.assertTrue(process.closed)
                files=list(Path(directory).glob('graphics-capture-*.json'));self.assertEqual(len(files),1)
                report=json.loads(files[0].read_text())
                self.assertEqual(report['mode'],'READ ONLY CAPTURE IPC')
                self.assertEqual('error' in report,failed)


if __name__=='__main__':unittest.main()
