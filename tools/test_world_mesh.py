import base64,ctypes,struct,unittest,zlib
from fake_re4_host import ProtocolError
from re4_reader import ReaderError
from re4_world_mesh import NativeWorldMesh,decode_world_mesh,SIZE
from re4_control import ControlPolicy
from test_re4_control import Writer

class Kernel:
 def __init__(self):
  self.buffer=ctypes.create_string_buffer(SIZE);struct.pack_into('<4I',self.buffer,0,0x52453457,1,457,42);self.closed=[]
 def OpenFileMappingW(self,a,b,n):assert a==6 and n=='Local\\RE4Bridge-WorldMesh-42';return 1
 def MapViewOfFile(self,*args):assert args[-1]==77888;return ctypes.addressof(self.buffer)
 def UnmapViewOfFile(self,a):self.closed.append(a)
 def CloseHandle(self,a):self.closed.append(a)
 def GetTickCount(self):return 1000
class WorldMeshTests(unittest.TestCase):
 def mesh(self):return dict(encoding='world-quads-v1',vertices=base64.b64encode(struct.pack('<5fI',100,200,-300,.25,.5,0xFFFFFFFF)*4).decode(),texture=base64.b64encode(zlib.compress(bytes(128*128*4))).decode())
 def frame(self,mesh):return dict(type='re4_control',armed=True,buttons=0,arm_id=7,depth=True,look=dict(enabled=True,first_person=True,total_yaw=0,pitch=0,eye_height=1000),world_mesh=mesh)
 def test_strict_bounded_decode_before_any_control_write(self):
  good=self.mesh();decoded=decode_world_mesh(good);self.assertEqual(tuple(map(len,decoded)),(96,65536))
  bad=[dict(good,extra=1),dict(good,encoding='hud'),dict(good,vertices='!'),dict(good,texture=base64.b64encode(zlib.compress(bytes(65537))).decode()),dict(good,texture=good['texture']+'AAAA'),dict(good,vertices=base64.b64encode(struct.pack('<5fI',float('nan'),0,0,0,0,0)*4).decode()),dict(good,vertices=base64.b64encode(struct.pack('<5fI',0,0,0,1.1,0,0)*4).decode()),dict(good,vertices=good['vertices']*129)]
  for mesh in bad:
   writer=Writer()
   with self.assertRaises(ProtocolError):ControlPolicy(writer).apply(self.frame(mesh),dict(state='player'))
   self.assertFalse(writer.writes)
 def test_mapping_identity_command_layout_and_stop(self):
  k=Kernel();native=NativeWorldMesh(k,42);header=bytes(k.buffer)[:16];native.publish(True,7,decode_world_mesh(self.mesh()))
  self.assertEqual(bytes(k.buffer)[:16],header);self.assertEqual(struct.unpack_from('<4I',k.buffer,20),(1000,1,7,1));self.assertEqual(struct.unpack_from('<3f',k.buffer,64),(100,200,-300));self.assertEqual(struct.unpack_from('<I',k.buffer,16)[0]%2,0)
  native.close();self.assertEqual(struct.unpack_from('<3I',k.buffer,24),(0,0,0));self.assertEqual(len(k.closed),2)
  k=Kernel();struct.pack_into('<I',k.buffer,8,456);before=bytes(k.buffer)
  with self.assertRaises(ReaderError):NativeWorldMesh(k,42)
  self.assertEqual(bytes(k.buffer),before)
 def test_world_visuals_obey_menu_script_stop_and_depth_without_boxes(self):
  class Depth:
   def publish(self,*args):self.last=args
  class Preview:
   depth=Depth()
   def publish(self,*args):pass
   def status(self):return {}
  class Runtime:
   scene=0
   def status(self):return dict(fresh=True,scene=self.scene)
   def publish(self,*args):pass
  class Mesh:
   available=True
   def publish(self,*args):self.last=args
  writer=Writer();writer.status=lambda:dict(stop=0,ticking=True,hook_ticks=1,capabilities=31)
  mesh=Mesh();preview=Preview();runtime=Runtime();policy=ControlPolicy(writer,preview=preview,runtime=runtime,world_mesh=mesh);frame=self.frame(self.mesh())
  policy.apply(frame,dict(state='player'));self.assertTrue(mesh.last[0]);self.assertEqual(preview.depth.last,(True,7))
  runtime.scene=1;policy.apply(frame,dict(state='player'));self.assertFalse(mesh.last[0])
  runtime.scene=0;policy.apply(dict(frame,menu=True,world_mesh=None),dict(state='player'));self.assertFalse(mesh.last[0])
  policy.stop();self.assertEqual(mesh.last,(False,0,None))

 def test_current_native_build457_retains_runtime_and_combat_abi(self):
  from test_combat import Kernel as CombatKernel
  from test_runtime import Kernel as RuntimeKernel
  from re4_combat import NativeCombat
  from re4_runtime import NativeRuntime
  k=CombatKernel();k.words[2]=457;native=NativeCombat(42,k);self.assertTrue(native.available);native.close()
  k=RuntimeKernel();struct.pack_into('<I',k.buffer,8,457);native=NativeRuntime(42,k);self.assertTrue(native.available);native.close()
