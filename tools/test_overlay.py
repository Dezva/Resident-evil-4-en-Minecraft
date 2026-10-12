import base64,ctypes,struct,unittest,zlib
from re4_overlay import NativeOverlay,validate_overlay,pixels,SIZE,MAGIC
from fake_re4_host import ProtocolError
from re4_control import ControlPolicy
from test_cube_test import Function
from test_hud import Writer

def frame(menu=False):return {'image':base64.b64encode(zlib.compress(bytes([17,34,51,128])*8192,1)).decode(),'width':256,'height':32,'menu':menu,'encoding':'zlib-argb'}
class Kernel:
 def __init__(self):
  self.data=ctypes.create_string_buffer(SIZE);struct.pack_into('<2I',self.data,0,MAGIC,1);self.closed=[];self.unmapped=[]
  self.OpenFileMappingW=Function(lambda access,inherit,name:7 if access==6 and not inherit and name=='Local\\RE4Bridge-OverlayCommand-42' else 0)
  self.MapViewOfFile=Function(lambda handle,access,high,low,size:ctypes.addressof(self.data) if size==SIZE else 0)
  self.UnmapViewOfFile=Function(lambda address:self.unmapped.append(address));self.CloseHandle=Function(lambda h:self.closed.append(h));self.GetTickCount=Function(lambda:1234)
class OverlayTests(unittest.TestCase):
 def test_compressed_image_revision_outline_telemetry_and_disable(self):
  k=Kernel();hud=NativeOverlay(42,k);f=frame();f['selection']=[1,2,3,.3,0,0,0,1000,500,1000]
  struct.pack_into('<4I',k.data,40,1366,768,76,72);hud.publish(True,3,f)
  self.assertEqual(struct.unpack_from('<10I',k.data),(MAGIC,1,2,1234,1,3,256,32,0,1));self.assertEqual(k.data.raw[128:128+32768],bytes([17,34,51,128])*8192)
  self.assertEqual(struct.unpack_from('<I',k.data,64)[0],1)
  hud.publish(True,3,f);self.assertEqual(struct.unpack_from('<I',k.data,36)[0],1)
  f['menu']=True;hud.publish(True,3,f);self.assertEqual(struct.unpack_from('<I',k.data,32)[0],1);self.assertEqual(struct.unpack_from('<I',k.data,64)[0],0)
  self.assertEqual(hud.status()['overlay_viewport_width'],1366);self.assertEqual(hud.status()['native_hud_hidden'],72)
  hud.close();hud.close();self.assertEqual(k.closed,[7]);self.assertEqual(len(k.unmapped),1);self.assertEqual(struct.unpack_from('<I',k.data,16)[0],0)
 def test_bad_streams_dimensions_bounds_and_zip_bombs_make_no_changes(self):
  k=Kernel();hud=NativeOverlay(42,k);f=frame();hud.publish(True,1,f);before=k.data.raw
  bad=[]
  for key,value in [('width',True),('width',1601),('height',901),('menu',1),('encoding','png'),('image','!'),('selection',[0]*10)]:bad.append(dict(f,**{key:value}))
  raw=base64.b64decode(f['image'])
  for stream in [raw[:-1],raw+b'trailing',raw+raw,zlib.compress(bytes(32769)),zlib.compress(bytes(32767)),zlib.compress(bytes(6000000))]:bad.append(dict(f,image=base64.b64encode(stream).decode()))
  for b in bad:
   with self.assertRaises(ProtocolError):hud.publish(True,1,b)
   self.assertEqual(k.data.raw,before)
  hud.close()
 def test_armed_menu_keeps_lease_with_zero_buttons_then_stop_clears(self):
  k=Kernel();hud=NativeOverlay(42,k);writer=Writer();policy=ControlPolicy(writer,hud=hud)
  f={'type':'re4_control','armed':True,'buttons':0,'arm_id':1,'hud':frame(True),'look':{'enabled':True,'first_person':True,'absolute_yaw':False,'total_yaw':0,'pitch':0,'eye_height':1620}}
  self.assertTrue(policy.apply(f,{'state':'player'})['armed']);self.assertTrue(writer.armed)
  f['buttons']=1;self.assertFalse(policy.apply(f,{'state':'player'})['armed']);self.assertFalse(writer.armed)
  f['buttons']=0;f['arm_id']=2;self.assertTrue(policy.apply(f,{'state':'player'})['armed'])
  f['arm_id']=3;f['hud']={'image':base64.b64encode(bytes(32768)).decode()};self.assertFalse(policy.apply(f,{'state':'player'})['armed']);self.assertFalse(writer.armed)
  policy.stop();self.assertEqual(struct.unpack_from('<I',k.data,16)[0],0);hud.close()
