import ctypes
import unittest
from re4_health import NativeHealth,validate_health,MAGIC,SIZE
from re4_control import validate_control,ControlPolicy
from fake_re4_host import ProtocolError
from test_re4_control import Writer
from test_cube_test import Function
from re4_reader import ReaderError

class Kernel:
    def __init__(self):
        self.buffer=ctypes.create_string_buffer(SIZE);self.words=(ctypes.c_uint32*25).from_buffer(self.buffer)
        self.words[:4]=(MAGIC,1,441,42);self.closed=[]
        self.OpenFileMappingW=Function(lambda access,inherit,name:7 if name=='Local\\RE4Bridge-Health-42' and access==6 else 0)
        self.MapViewOfFile=Function(lambda handle,access,high,low,size:ctypes.addressof(self.buffer) if handle==7 and size==SIZE else 0)
        self.UnmapViewOfFile=Function(lambda address:self.closed.append(address))
        self.CloseHandle=Function(lambda handle:self.closed.append(handle))
        self.GetTickCount=Function(lambda:1000)

class HealthTests(unittest.TestCase):
    def request(self):return dict(mode=1,ratio=.8,damage_ack=200000,healing_ack=0)
    def test_validation_precedes_every_control_write(self):
        for key,value in [('mode',True),('mode',3),('ratio',float('nan')),('ratio',-1),('ratio',1.1),('damage_ack',-1),('healing_ack',4294967296)]:
            request=dict(self.request(),**{key:value})
            frame=dict(type='re4_control',armed=True,buttons=0,arm_id=1,health=request)
            writer=Writer()
            with self.assertRaises(ProtocolError):ControlPolicy(writer).apply(frame,{'state':'player'})
            self.assertEqual(writer.writes,[])
        validate_health(None);validate_health(self.request())
        for menu in [1,'true']:
            with self.assertRaises(ProtocolError):validate_control(dict(type='re4_control',armed=True,buttons=0,arm_id=1,menu=menu))
        with self.assertRaises(ProtocolError):validate_control(dict(type='re4_control',armed=True,buttons=1,arm_id=1,menu=True))
    def test_native_request_seqlock_preserves_owned_report_and_stop_disables_health(self):
        k=Kernel();native=NativeHealth(42,k)
        k.words[12:25]=[2,1000,0,1,7,4096,800,1000,200000,0,0,9,1];report=list(k.words[12:])
        native.publish(True,7,self.request())
        self.assertEqual((k.words[4]%2,k.words[5],k.words[6],k.words[7],k.words[8],k.words[10]),(0,1000,1,7,1,200000))
        self.assertAlmostEqual(ctypes.c_float.from_address(native.address+36).value,.8)
        self.assertEqual(report,list(k.words[12:]))
        result=native.status();self.assertTrue(result['active']);self.assertEqual((result['hp'],result['damage'],result['mode']),(800,200000,1))
        writer=Writer();policy=ControlPolicy(writer,health=native)
        state=policy.apply(dict(type='re4_control',armed=True,buttons=0,arm_id=7,health=self.request()),{'state':'player'})
        self.assertTrue(state['health_state']['available']);self.assertEqual(k.words[6],1)
        policy.stop();self.assertEqual(k.words[6],0);native.close();self.assertEqual(len(k.closed),2)
    def test_torn_report_and_wrong_pid_or_build_are_never_applied(self):
        k=Kernel();native=NativeHealth(42,k);k.words[12]=1
        self.assertFalse(native.status()['fresh']);native.close()
        for index,value in [(0,0),(2,439),(3,43)]:
            k=Kernel();k.words[index]=value
            with self.assertRaises(ReaderError):NativeHealth(42,k)
            self.assertEqual(len(k.closed),2)

if __name__=='__main__':unittest.main()
