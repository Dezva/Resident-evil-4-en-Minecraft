import ctypes
import json
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch
import re4_cube_test as cube
from re4_reader import ReaderError

class Function:
    def __init__(self,callback):self.callback=callback
    def __call__(self,*args):return self.callback(*args)

class Clock:
    def __init__(self):self.now=0
    def monotonic(self):return self.now
    def sleep(self,amount):self.now+=amount

class Kernel:
    def __init__(self,clock=None,wait=0,map_fail=None,busy=False,f8=False,fail_at=None,version=1,build=418):
        self.clock=clock or Clock();self.closed=[];self.unmapped=[];self.released=0;self.map_requests=[]
        self.command=ctypes.create_string_buffer(struct.pack('<8I',0x52453454,version,2,0,0,0,0,0)+bytes(96320 if version==6 else 3120 if version in (3,4,5) else 24 if version==2 else 0))
        words=[0x52453455,1,3 if busy else 2,build,42]+[0]*18;words[5]=1;words[16]=1
        self.telemetry=ctypes.create_string_buffer(struct.pack('<23I',*words))
        self.collision=ctypes.create_string_buffer(struct.pack('<12I',0x52453456,1,2,424,42,0,0,0,0,1,0,0))
        def open_mapping(access,inherit,name):
            assert not inherit
            if name=='Local\\RE4Bridge-Collision-42':assert access==4;return 13 if version==6 else 0
            if name=='Local\\RE4Bridge-CubeTelemetry-42':assert access==4;return 10
            assert name=='Local\\RE4Bridge-CubeCommand-42' and access==6;return 11
        def view(handle,access,high,low,size):
            self.map_requests.append((handle,access,size));assert high==low==0
            if handle==map_fail:return 0
            if handle==13:assert access==4 and size==48;return ctypes.addressof(self.collision)
            if handle==10:
                assert access==4 and size==92
                if struct.unpack_from('<I',self.command,16)[0]:
                    for field in ('frames','draws'):offset=(3+cube.FIELDS.index(field))*4;struct.pack_into('<I',self.telemetry,offset,struct.unpack_from('<I',self.telemetry,offset)[0]+1)
                    if fail_at and self.clock.now>=fail_at:struct.pack_into('<I',self.telemetry,44,1)
                return ctypes.addressof(self.telemetry)
            assert handle==11 and access==6 and size in (32,56,3152,96352);return ctypes.addressof(self.command)
        def mutex(attributes,owner,name):assert attributes is None and not owner and name=='Local\\RE4Bridge-CubeWriter-42';return 12
        def waiting(handle,timeout):assert handle==12 and timeout==0;return wait
        def release(handle):assert handle==12;self.released+=1;return 1
        self.OpenFileMappingW=Function(open_mapping);self.MapViewOfFile=Function(view)
        self.UnmapViewOfFile=Function(lambda address:self.unmapped.append(address) or 1)
        self.CloseHandle=Function(lambda handle:self.closed.append(handle) or 1)
        self.CreateMutexW=Function(mutex);self.WaitForSingleObject=Function(waiting);self.ReleaseMutex=Function(release)
        self.GetTickCount=Function(lambda:int(self.clock.now*1000)&0xffffffff)
        self.GetAsyncKeyState=Function(lambda key:0x8000 if f8 else 0)

class CubeTests(unittest.TestCase):
    def test_telemetry_readonly_snapshot_validation_cleanup(self):
        kernel=Kernel();before=kernel.telemetry.raw
        result=cube.read_cube_status(kernel,42)
        self.assertEqual(result['live_devices'],1);self.assertEqual(result['reason'],'desactivado')
        self.assertEqual(kernel.telemetry.raw,before);self.assertEqual(kernel.closed,[10])
        self.assertEqual(len(kernel.unmapped),1)
        for offset,value in [(0,0),(4,2),(12,417),(16,43),(60,2),(64,99)]:
            kernel=Kernel();struct.pack_into('<I',kernel.telemetry,offset,value)
            with self.assertRaises(ReaderError):cube.read_cube_status(kernel,42)
            self.assertEqual(kernel.closed,[10]);self.assertEqual(len(kernel.unmapped),1)
        kernel=Kernel(busy=True)
        with self.assertRaises(ReaderError):cube.read_cube_status(kernel,42)
        kernel=Kernel(map_fail=10)
        with self.assertRaises(ReaderError):cube.read_cube_status(kernel,42)
        self.assertEqual(kernel.closed,[10]);self.assertEqual(kernel.unmapped,[])

    def test_single_writer_publish_and_disable_on_close(self):
        kernel=Kernel();before=kernel.command.raw;session=cube.CubeSession(kernel,42,user=kernel)
        self.assertEqual(kernel.command.raw,before)
        session.publish(True);words=struct.unpack('<8I',kernel.command.raw[:32])
        self.assertEqual(words[4:],(1,session.epoch,0,0));self.assertFalse(words[2]&1)
        session.close();session.close()
        self.assertEqual(struct.unpack_from('<I',kernel.command,16)[0],0)
        self.assertEqual(kernel.closed,[11,12]);self.assertEqual(kernel.released,1)
        for wait,map_fail in [(0x102,None),(0,11)]:
            kernel=Kernel(wait=wait,map_fail=map_fail);before=kernel.command.raw
            with self.assertRaises(ReaderError):cube.CubeSession(kernel,42,user=kernel)
            self.assertEqual(kernel.command.raw,before);self.assertIn(12,kernel.closed)
            self.assertEqual(kernel.released,int(wait==0))

    def test_countdown_duration_f8_and_native_failure(self):
        for f8,fail_at,stop in [(False,None,'duration'),(True,None,'F8 before activation'),(False,5.2,'native safety stop')]:
            clock=Clock();kernel=Kernel(clock,f8=f8,fail_at=fail_at);session=cube.CubeSession(kernel,42,user=kernel)
            with patch.object(cube.time,'monotonic',clock.monotonic),patch.object(cube.time,'sleep',clock.sleep):
                result=cube.run_test(session,1)
            self.assertEqual(result['stop'],stop)
            if not f8:self.assertTrue(result['drawing_observed']);self.assertGreaterEqual(clock.now,5)
            session.close();self.assertEqual(struct.unpack_from('<I',kernel.command,16)[0],0)

    def test_main_always_stops_and_closes_on_error(self):
        class Process:
            def __init__(self):self.k=Kernel();self.closed=False
            def close(self):self.closed=True
        with tempfile.TemporaryDirectory() as directory:
            process=Process()
            with patch.object(cube,'find_pid',return_value=42),patch.object(cube,'WindowsProcess',return_value=process),patch.object(cube.ctypes,'WinDLL',create=True,return_value=process.k),patch.object(cube,'run_test',side_effect=ReaderError('fixture')):
                self.assertEqual(cube.main(['--output-dir',directory]),1)
            self.assertTrue(process.closed)
            self.assertEqual(process.k.closed,[11,12]);self.assertEqual(process.k.released,1)
            report=json.loads(next(Path(directory).glob('cube-test-*.json')).read_text())
            self.assertEqual(report['error'],'fixture')

    def test_graphics_capture_collects_cube_without_activation(self):
        import re4_graphics_capture as capture
        class Process:
            def __init__(self):self.k=Kernel();self.closed=False
            def close(self):self.closed=True
        with tempfile.TemporaryDirectory() as directory:
            process=Process();before=process.k.command.raw
            state={'build':'0.4.18','hook_description':'importacion interceptada','device_success':1}
            with patch.object(capture,'find_pid',return_value=42),patch.object(capture,'capture_kernel',return_value=process.k),patch.object(capture,'read_capture',return_value=state):
                self.assertEqual(capture.main(['--output-dir',directory]),0)
            self.assertFalse(process.closed);self.assertEqual(process.k.command.raw,before)
            self.assertEqual(process.k.closed,[10]);self.assertEqual(process.k.released,0)
            report=json.loads(next(Path(directory).glob('graphics-capture-*.json')).read_text())
            self.assertEqual(report['capture']['cube']['live_devices'],1)

if __name__=='__main__':unittest.main()
