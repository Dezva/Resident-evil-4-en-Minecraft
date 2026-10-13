import ctypes
import unittest
from re4_runtime import NativeRuntime,MAGIC
from re4_control import ControlPolicy,validate_control
from test_re4_control import Writer
from test_cube_test import Function
from fake_re4_host import ProtocolError

class Kernel:
    def __init__(self):
        self.buffer=ctypes.create_string_buffer(64);self.words=(ctypes.c_uint32*16).from_buffer(self.buffer)
        self.words[:4]=(MAGIC,1,442,42);self.closed=[]
        self.OpenFileMappingW=Function(lambda a,b,n:7)
        self.MapViewOfFile=Function(lambda a,b,c,d,s:ctypes.addressof(self.buffer) if s==64 else 0)
        self.UnmapViewOfFile=Function(lambda a:self.closed.append(a));self.CloseHandle=Function(lambda a:self.closed.append(a))
        self.GetTickCount=Function(lambda:1000)
class RuntimeTests(unittest.TestCase):
    def test_native_mapping_action_only_writes_request_and_preserves_scene(self):
        k=Kernel();native=NativeRuntime(42,k);k.words[4:12]=[2,1000,1,0x10000000,3,12,10,5];before=list(k.words[4:12])
        native.publish(7,8);self.assertEqual(before,list(k.words[4:12]));self.assertEqual(tuple(k.words[12:]),(4,1000,8,7))
        state=native.status();self.assertEqual(state['scene'],1);self.assertTrue(state['action_available']);self.assertTrue(state['enemies_available'])
        k.words[4]=3;self.assertFalse(native.status()['fresh']);native.close();self.assertEqual(k.words[14],0);self.assertEqual(len(k.closed),2)
    def test_cinematic_missing_player_and_idle_pad_keep_existing_generation_but_not_a_new_arm(self):
        class Runtime:
            scene=0;fresh=True
            def status(self):return dict(available=True,scene=self.scene,fresh=self.fresh)
            def publish(self,*args):pass
        runtime=Runtime();writer=Writer();policy=ControlPolicy(writer,runtime=runtime)
        frame=dict(type='re4_control',armed=True,buttons=65,arm_id=7)
        writer.status=lambda:dict(stop=0,ticking=writer.ticking,hook_ticks=123,capabilities=7)
        self.assertTrue(policy.apply(frame,{'state':'player'})['armed'])
        runtime.scene=1;writer.ticking=False
        for _ in range(100):self.assertTrue(policy.apply(frame,{'state':'waiting_player'})['armed']);self.assertEqual(writer.writes[-1],(True,0))
        runtime.scene=0;writer.ticking=True;self.assertTrue(policy.apply(frame,{'state':'player'})['armed']);self.assertEqual(writer.writes[-1],(True,65))
        runtime.scene=1;runtime.fresh=False
        self.assertFalse(policy.apply(frame,{'state':'waiting_player'})['armed'])
        runtime.fresh=True;self.assertFalse(policy.apply(frame,{'state':'player'})['armed']) # stopped session cannot resume itself
    def test_action_validation_never_sends_in_menus_or_disarmed(self):
        base=dict(type='re4_control',armed=True,buttons=0,arm_id=1,action_id=1)
        validate_control(base)
        for change in (dict(action_id=True),dict(action_id=-1),dict(action_id=2147483648),dict(menu=True),dict(armed=False)):
            with self.assertRaises(ProtocolError):validate_control(dict(base,**change))

if __name__=='__main__':unittest.main()
