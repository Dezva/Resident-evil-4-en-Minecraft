import base64
import ctypes
import struct
import unittest
from re4_hud import NativeHud,validate_hud,SIZE
from fake_re4_host import ProtocolError
from re4_control import ControlPolicy
from test_cube_test import Function

class Kernel:
    def __init__(self,present=True):
        self.data=ctypes.create_string_buffer(SIZE);struct.pack_into('<3I',self.data,0,0x52453448,1,2)
        self.closed=[];self.unmapped=[]
        self.OpenFileMappingW=Function(lambda access,inherit,name:self.open(access,inherit,name,present))
        self.MapViewOfFile=Function(lambda handle,access,high,low,size:ctypes.addressof(self.data) if size==SIZE else 0)
        self.UnmapViewOfFile=Function(lambda address:self.unmapped.append(address))
        self.CloseHandle=Function(lambda handle:self.closed.append(handle))
        self.GetTickCount=Function(lambda:1234)
    def open(self,access,inherit,name,present):
        assert access==6 and not inherit and name=='Local\\RE4Bridge-HudCommand-42';return 7 if present else 0
class Writer:
    def __init__(self):self.armed=False
    def reset_stop(self):pass
    def status(self):return {'stop':0,'ticking':True,'hook_ticks':20,'capabilities':7}
    def publish(self,armed,buttons,**kw):self.armed=armed
class HudTests(unittest.TestCase):
    def test_publication_pixels_generation_disable_and_cleanup(self):
        k=Kernel();hud=NativeHud(42,k);image={'image':base64.b64encode(bytes([17,34,51,255])*8192).decode()}
        hud.publish(True,8,image)
        self.assertEqual(struct.unpack_from('<6I',k.data),(0x52453448,1,4,1234,1,8));self.assertEqual(k.data.raw[24:SIZE],bytes([17,34,51,255])*8192)
        before=k.data.raw
        for invalid in [{'image':'a'}, {'image':'!'*43692}, {'image':image['image'],'extra':1}]:
            with self.assertRaises(ProtocolError):hud.publish(True,8,invalid)
            self.assertEqual(before,k.data.raw)
        hud.close();hud.close();self.assertEqual(k.closed,[7]);self.assertEqual(len(k.unmapped),1);self.assertEqual(struct.unpack_from('<2I',k.data,16),(0,0));self.assertFalse(any(k.data.raw[24:SIZE]))
    def test_unavailable_capability_and_wrong_mode_do_not_arm(self):
        for present,fp in [(False,True),(True,False)]:
            k=Kernel(present);hud=NativeHud(42,k);writer=Writer();policy=ControlPolicy(writer,hud=hud)
            frame={'type':'re4_control','armed':True,'buttons':1,'arm_id':1,'hud':{'image':base64.b64encode(bytes(32768)).decode()},'look':{'enabled':True,'first_person':fp,'total_yaw':0,'pitch':0,'eye_height':1600}}
            result=policy.apply(frame,{'state':'player'});self.assertFalse(result['armed']);self.assertFalse(writer.armed);hud.close()
    def test_disarm_clears_hud_with_live_network(self):
        k=Kernel();hud=NativeHud(42,k);writer=Writer();policy=ControlPolicy(writer,hud=hud)
        frame={'type':'re4_control','armed':True,'buttons':1,'arm_id':1,'hud':{'image':base64.b64encode(bytes(32768)).decode()},'look':{'enabled':True,'first_person':True,'total_yaw':0,'pitch':0,'eye_height':1600}}
        self.assertTrue(policy.apply(frame,{'state':'player'})['armed']);self.assertEqual(struct.unpack_from('<I',k.data,16)[0],1)
        policy.stop();self.assertEqual(struct.unpack_from('<I',k.data,16)[0],0);hud.close()
