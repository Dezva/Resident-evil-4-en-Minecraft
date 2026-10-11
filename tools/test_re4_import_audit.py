import contextlib
import io
import json
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch
import re4_import_audit as audit
from re4_reader import ReaderError


class Memory:
    def __init__(self):
        self.base=0x400000;self.size=65536;self.dll=0x2000000
        self.rows={self.base:bytearray(self.size),self.dll:bytearray(self.size)}
        self.denied=None
        self.modules=[dict(name='bio4.exe',base=self.base,size=self.size,path=r'D:\Games\bio4.exe'),
                      dict(name='d3d9.dll',base=self.dll,size=self.size,path=r'C:\Windows\SysWOW64\d3d9.dll'),
                      dict(name='dinput8.dll',base=0x3000000,size=self.size,path=r'D:\Games\dinput8.dll')]
        for image in self.rows.values():
            struct.pack_into('<H',image,0,0x5a4d);struct.pack_into('<I',image,0x3c,0x80)
            struct.pack_into('<I',image,0x80,0x4550);struct.pack_into('<H',image,0x84,0x14c)
            struct.pack_into('<H',image,0x94,224);struct.pack_into('<H',image,0x98,0x10b)
            struct.pack_into('<I',image,0xd0,self.size);struct.pack_into('<I',image,0xf4,16)
        main=self.rows[self.base];dll=self.rows[self.dll]
        struct.pack_into('<2I',main,0x100,0x400,40)
        struct.pack_into('<5I',main,0x400,0x600,0,0,0x500,0x700)
        main[0x500:0x509]=b'd3d9.dll\0'
        struct.pack_into('<I',main,0x600,0x800);struct.pack_into('<I',main,0x700,self.dll+0x2000)
        main[0x802:0x811]=b'Direct3DCreate9\0'
        struct.pack_into('<2I',dll,0xf8,0x1000,1024)
        struct.pack_into('<6I',dll,0x1010,10,1,1,0x1200,0x1210,0x1220)
        struct.pack_into('<I',dll,0x1200,0x2000);struct.pack_into('<I',dll,0x1210,0x1300)
        struct.pack_into('<H',dll,0x1220,0)
        dll[0x1300:0x130f]=b'Direct3DCreate9\0'

    def put(self,rva,value,module=None):struct.pack_into('<I',self.rows[module or self.base],rva,value)
    def read(self,address,length):
        if self.denied and address<=self.denied<address+length:raise ReaderError('Guarded')
        for base,image in self.rows.items():
            if base<=address and length<=len(image)-(address-base):return bytes(image[address-base:address-base+length])
        raise ReaderError('Synthetic out-of-range memory')
    def report(self,**kw):return audit.inspect(self.read,self.base,self.size,self.modules,r'C:\Windows\SysWOW64',**kw)


class ImportAuditTests(unittest.TestCase):
    def test_current_named_import_and_bytes_preserved(self):
        memory=Memory();before={base:bytes(row) for base,row in memory.rows.items()}
        report=memory.report()
        self.assertEqual(report['current_native_lookup_status'],1)
        self.assertEqual(report['direct3dcreate9_export']['ordinal'],10)
        self.assertTrue(report['matching_named_imports'][0]['matches_system_export'])
        self.assertEqual(before,{base:bytes(row) for base,row in memory.rows.items()})

    def test_missing_name_table_bound_iat_and_changed_target(self):
        memory=Memory();memory.put(0x400,0);report=memory.report()
        self.assertEqual(report['current_native_lookup_status'],3)
        self.assertIn('d3d9_original_first_thunk_missing',report['reasons'])
        self.assertEqual(report['d3d9_descriptors'][0]['export_found_in_bound_iat'],['0x700'])
        memory=Memory();memory.put(0x700,0x3001000);report=memory.report()
        self.assertIn('create9_iat_target_differs_from_system_export',report['reasons'])
        self.assertEqual(report['matching_named_imports'][0]['target_location']['module'],'dinput8.dll')

    def test_duplicate_ordinal_and_nonterminated_imports(self):
        memory=Memory();memory.put(0x604,0x800);memory.put(0x704,memory.dll+0x2000)
        self.assertIn('create9_named_import_ambiguous',memory.report()['reasons'])
        memory=Memory();memory.put(0x600,0x8000000a);report=memory.report()
        self.assertEqual(report['current_native_lookup_status'],2)
        self.assertTrue(report['d3d9_descriptors'][0]['entries'][0]['is_create9_ordinal'])
        memory=Memory();memory.put(0x104,20)
        self.assertIn('main_import_descriptors_unterminated',memory.report()['reasons'])

    def test_library_path_export_forwarder_bounds_and_read_limits(self):
        memory=Memory();memory.modules[1]['path']=r'D:\Games\d3d9.dll'
        self.assertIn('d3d9_system_path_mismatch',memory.report()['reasons'])
        memory=Memory();memory.put(0x1200,0x1300,memory.dll);report=memory.report()
        self.assertIn('d3d9_create9_export_forwarded',report['reasons'])
        self.assertIsNone(report['current_native_lookup_status'])
        memory=Memory();memory.put(0x100,0xfffffff0)
        self.assertIn('main_import_directory_bounds',memory.report()['reasons'])
        memory=Memory();memory.denied=memory.base+0x700
        self.assertIn('main_memory_unreadable',memory.report()['reasons'])
        for limits in [dict(max_reads=1),dict(max_bytes=64),dict(seconds=0)]:
            report=Memory().report(**limits)
            self.assertIn('audit_resource_limit',report['reasons'])
            self.assertTrue(report['inspection_incomplete'])
            self.assertIsNone(report['current_native_lookup_status'])

    def test_main_saves_report_and_cleans_handles_even_on_error(self):
        class Process:
            def __init__(self,m):self.base=m.base;self.size=m.size;self.read=m.read;self.k=object();self.closed=False
            def close(self):self.closed=True
        for failed in [False,True]:
            memory=Memory();process=Process(memory)
            with tempfile.TemporaryDirectory() as directory:
                with patch.object(audit,'find_pid',return_value=42),patch.object(audit,'WindowsProcess',return_value=process),patch.object(audit,'loaded_modules',side_effect=ReaderError('Access denied') if failed else None,return_value=memory.modules),patch.object(audit,'system_directory',return_value=r'C:\Windows\SysWOW64'),patch.object(audit,'read_capture',return_value={'hook_status':3}),contextlib.redirect_stdout(io.StringIO()):
                    self.assertEqual(audit.main(['--output-dir',directory]),int(failed))
                self.assertTrue(process.closed)
                report=json.loads(next(Path(directory).glob('imports-audit-*.json')).read_text())
                self.assertEqual(report['mode'],'READ ONLY IMPORT AUDIT')
                if not failed:
                    self.assertEqual(report['current_native_lookup_status'],1)
                    self.assertIn('remains unknown',report['startup_note'])
                else:self.assertIn('error',report)


if __name__=='__main__':unittest.main()
