import ctypes
import unittest
from re4_runtime import NativeRuntime,MAGIC
from re4_control import ControlPolicy,validate_control
from test_re4_control import Writer
from test_cube_test import Function
from fake_re4_host import ProtocolError
from re4_reader import ReaderError

class Kernel:
    def __init__(self):
        self.buffer=ctypes.create_string_buffer(160);self.words=(ctypes.c_uint32*40).from_buffer(self.buffer)
        self.words[:4]=(MAGIC,5,455,42);self.closed=[]
        self.OpenFileMappingW=Function(lambda a,b,n:7)
        self.MapViewOfFile=Function(lambda a,b,c,d,s:ctypes.addressof(self.buffer) if s==160 else 0)
        self.UnmapViewOfFile=Function(lambda a:self.closed.append(a));self.CloseHandle=Function(lambda a:self.closed.append(a))
        self.GetTickCount=Function(lambda:1000)
class RuntimeTests(unittest.TestCase):
    def test_corrected_gameplay_report_keeps_w_sprint_and_survives_menu(self):
        k=Kernel();native=NativeRuntime(42,k);k.words[4:12]=[2,1000,0,50331920,3,0,0,0]
        writer=Writer();status=writer.status;writer.status=lambda:dict(status(),capabilities=31)
        policy=ControlPolicy(writer,runtime=native)
        frame=dict(type='re4_control',armed=True,buttons=65,arm_id=7)
        for _ in range(100):
            reply=policy.apply(frame,{'state':'player'})
            self.assertTrue(reply['armed']);self.assertEqual(writer.writes[-1],(True,65));self.assertEqual(reply['runtime_state']['scene'],0)
        self.assertTrue(policy.apply(dict(frame,menu=True,buttons=0),{'state':'player'})['armed'])
        self.assertTrue(policy.apply(frame,{'state':'player'})['armed']);self.assertEqual(writer.writes[-1],(True,65));native.close()
    def test_old_broken_dll_rejected_with_update_message(self):
        for build in (442,445,446,447,448,449,450,451,452,453):
            k=Kernel();k.words[2]=build
            with self.assertRaisesRegex(ReaderError,'DLL0.4.55'):NativeRuntime(42,k)
            self.assertEqual(len(k.closed),2)
    def test_native_mapping_action_only_writes_request_and_preserves_scene(self):
        k=Kernel();native=NativeRuntime(42,k);k.words[4:12]=[2,1000,1,0x10000000,3,12,10,5];before=list(k.words[4:12])
        native.publish(7,8);self.assertEqual(before,list(k.words[4:12]));self.assertEqual(tuple(k.words[12:16]),(4,1000,8,7))
        state=native.status();self.assertEqual(state['scene'],1);self.assertTrue(state['action_available']);self.assertTrue(state['enemies_available'])
        k.words[4]=3;self.assertFalse(native.status()['fresh']);native.close();self.assertEqual(k.words[14],0);self.assertEqual(len(k.closed),2)
    def test_dialog_arrows_confirm_and_cancel_preserve_native_reports_and_publish_ack(self):
        k=Kernel();native=NativeRuntime(42,k);k.words[4:12]=[2,1000,1,0x03000110,3,0,0,0]
        k.words[32:38]=[1,6,0x10000202,10,7,1];before=bytes(k.buffer[128:])
        for kind in range(2,8):
            native.publish(7,kind,None,kind)
            self.assertEqual(k.words[20],kind);self.assertEqual(bytes(k.buffer[128:]),before)
        state=native.status();self.assertTrue(state['dialog_active']);self.assertEqual(state['action_ack'],10);self.assertEqual(state['action_generation'],7)
        self.assertEqual(state['dialog_actions'],6);native.close()
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
    def test_vertical_mapping_is_separate_from_native_report_and_stop_cancels_it(self):
        k=Kernel();native=NativeRuntime(42,k);native.publish(7,0,dict(lift=1250,scale=1000,token=9,feet_y=-125,native_ground=False))
        self.assertEqual(ctypes.c_float.from_buffer(k.buffer,64).value,1250);self.assertEqual(k.words[17],9);self.assertEqual(ctypes.c_float.from_buffer(k.buffer,112).value,-125)
        self.assertEqual(ctypes.c_float.from_buffer(k.buffer,72).value,1000);self.assertEqual(k.words[19],1)
        # Applied actor height is native telemetry, never echoed from the request.
        self.assertEqual(native.status()['actor_y'],0);self.assertFalse(native.status()['vertical_active'])
        ctypes.c_float.from_buffer(k.buffer,116).value=-125;k.words[30:32]=[1,24]
        self.assertEqual(native.status()['actor_y'],-125);self.assertTrue(native.status()['vertical_active']);self.assertEqual(native.status()['vertical_writes'],24)
        native.publish(7,0,dict(lift=1300,scale=1000,token=10,feet_y=-75,native_ground=False))
        self.assertEqual(native.status()['actor_y'],-125)
        writer=Writer();policy=ControlPolicy(writer,runtime=native);policy.stop();self.assertEqual(k.words[19],0);native.close()
    def test_native_ground_publishes_ownership_without_echoing_an_applied_height(self):
        k=Kernel();native=NativeRuntime(42,k)
        native.publish(7,0,dict(lift=0,scale=800,token=9,feet_y=315.845642,native_ground=True))
        self.assertEqual(k.words[19],2);self.assertEqual(ctypes.c_float.from_buffer(k.buffer,72).value,800)
        self.assertFalse(native.status()['vertical_active']);self.assertEqual(native.status()['vertical_writes'],0)
        native.close();self.assertEqual(k.words[19],0)
    def test_invalid_vertical_requests_never_write_control(self):
        base=dict(type='re4_control',armed=True,buttons=0,arm_id=7,vertical=dict(lift=1250,scale=1000,token=1,feet_y=-125,native_ground=False))
        validate_control(base)
        for change in (dict(lift=float('nan')),dict(lift=-1),dict(lift=128001),dict(scale=0),dict(token=True),dict(token=0),dict(feet_y=True),dict(feet_y=float('nan')),dict(feet_y=20000001,native_ground=False)):
            writer=Writer();frame=dict(base,vertical=dict(base['vertical'],**change))
            with self.assertRaises(ProtocolError):ControlPolicy(writer).apply(frame,{'state':'player'})
            self.assertEqual(writer.writes,[])
    def test_skip_kind_and_enemy_diagnostics_are_separate_from_commands(self):
        k=Kernel();native=NativeRuntime(42,k);k.words[4:12]=[2,1000,1,0x10000000,3,12,10,5]
        k.words[21:28]=[0,300,3,2,0,300,0x408]
        native.publish(7,3,None,1);self.assertEqual(k.words[20],1);self.assertEqual(k.words[21],0)
        state=native.status();self.assertEqual(state['enemy_scanned'],300);self.assertEqual(state['enemy_eligible'],3);self.assertEqual(state['skips'],2)
        for frame in (dict(action_id=1,action_kind=True),dict(action_id=1,action_kind=8),dict(action_kind=1)):
            with self.assertRaises(ProtocolError):validate_control(dict(type='re4_control',armed=True,buttons=0,arm_id=7,**frame))
        native.close()
    def test_action_validation_never_sends_in_menus_or_disarmed(self):
        base=dict(type='re4_control',armed=True,buttons=0,arm_id=1,action_id=1)
        validate_control(base)
        for change in (dict(action_id=True),dict(action_id=-1),dict(action_id=2147483648),dict(menu=True),dict(armed=False)):
            with self.assertRaises(ProtocolError):validate_control(dict(base,**change))

if __name__=='__main__':unittest.main()
