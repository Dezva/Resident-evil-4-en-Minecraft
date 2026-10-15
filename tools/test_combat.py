import ctypes,struct,unittest
from re4_combat import NativeCombat,validate_combat,MAGIC,SIZE
from re4_control import ControlPolicy,validate_control
from fake_re4_host import ProtocolError
from re4_reader import ReaderError
from test_re4_control import Writer
from test_cube_test import Function
class Kernel:
 def __init__(self):
  self.buffer=ctypes.create_string_buffer(SIZE);self.words=(ctypes.c_uint32*(SIZE//4)).from_buffer(self.buffer);self.words[:4]=(MAGIC,3,453,42);self.closed=[]
  self.OpenFileMappingW=Function(lambda a,b,n:7);self.MapViewOfFile=Function(lambda a,b,c,d,n:ctypes.addressof(self.buffer) if n==SIZE else 0)
  self.UnmapViewOfFile=Function(lambda a:self.closed.append(a));self.CloseHandle=Function(lambda a:self.closed.append(a));self.GetTickCount=Function(lambda:1000)
 def report(self):
  self.words[20:34]=[2,1000,7,1,0,10,0,1000,950,1,0,1,1,1]
  self.words[1314:1320]=[1,1,0,6,0,0]
  struct.pack_into('<3fI',self.buffer,5280,1800,1000,0,0x12)
  struct.pack_into('<5I5f',self.buffer,136,0,10,0x12,950,1000,2000,0,0,250,1600)
class CombatTests(unittest.TestCase):
 def frame(self):return dict(type='re4_control',armed=True,buttons=0,arm_id=7,combat_enabled=True,combat_scale=1000,look=dict(enabled=True,first_person=True,total_yaw=0,pitch=0,eye_height=1000),combat=dict(id=1,slot=0,guid=10,damage=50,scale=1000,reach=3,start=[0,1000,0],end=[3000,1000,0]))
 def test_validation_rejects_bad_ray_damage_identity_and_session_before_any_write(self):
  base=self.frame();validate_control(base)
  for field,value in [('id',True),('id',0),('slot',4096),('guid',-1),('damage',50001),('damage',0),('scale',float('nan')),('reach',7),('start',[True,0,0]),('end',[7000,1000,0]),('end',[0,1000,0]),('end',[float('inf'),0,0])]:
   frame=dict(base,combat=dict(base['combat'],**{field:value}));writer=Writer()
   with self.assertRaises(ProtocolError):ControlPolicy(writer).apply(frame,dict(state='player'))
   self.assertFalse(writer.writes)
  for change in [dict(combat_enabled=False),dict(armed=False),dict(menu=True),dict(combat_scale=800),dict(combat_enabled=1),dict(look=dict(enabled=True,first_person=False))]:
   with self.assertRaises(ProtocolError):validate_combat(dict(base,**change))
 def test_host_commands_preserve_native_actors_hp_ack_and_count(self):
  k=Kernel();native=NativeCombat(42,k);k.report();before=k.buffer.raw[80:]
  frame=self.frame();native.publish(True,7,1000,frame['combat'])
  self.assertEqual(before,k.buffer.raw[80:]);self.assertEqual(k.words[6:10],[1,7,1,50]);self.assertEqual(k.words[12:14],[0,10]);self.assertEqual(struct.unpack_from('<3f',k.buffer,56),(0,1000,0));self.assertEqual(k.words[4]%2,0)
  status=native.status();self.assertEqual(status['ack'],1);self.assertEqual(status['enemies'][0]['position'],[2000,0,0]);self.assertEqual(status['hp_after'],950)
  native.close();self.assertEqual(k.words[6],0);self.assertEqual(len(k.closed),2)
 def test_report_keeps_impact_kind_independent_of_last_actor_and_reads_callback_diagnostics(self):
  k=Kernel();native=NativeCombat(42,k);k.report();k.words[33]=2
  struct.pack_into('<5I5f',k.buffer,176,1,11,0x53,1000,1000,2300,0,0,700,1800)
  struct.pack_into('<7I',k.buffer,5296,2,1,1,1,0x20a01,1,3)
  status=native.status();self.assertEqual(status['target_kind'],0x12);self.assertEqual(status['enemies'][-1]['kind'],0x53)
  self.assertEqual(status['callback_thread_changed'],1);self.assertEqual(status['boxes_broken'],3);native.close()
 def test_control_policy_stops_watch_and_attacks_in_menus_disarm_and_script(self):
  k=Kernel();native=NativeCombat(42,k);writer=Writer();writer.status=lambda:dict(stop=0,ticking=True,hook_ticks=10,capabilities=31)
  class Runtime:
   scene=0
   def status(self):return dict(available=True,fresh=True,scene=self.scene)
   def publish(self,*args):pass
  runtime=Runtime();policy=ControlPolicy(writer,combat=native,runtime=runtime);frame=self.frame()
  state=policy.apply(frame,dict(state='player'));self.assertIn('combat_state',state);self.assertEqual(k.words[8],1)
  menu=dict(frame,menu=True,combat=None);policy.apply(menu,dict(state='player'));self.assertEqual(k.words[6],0);self.assertEqual(k.words[8],0)
  runtime.scene=1;policy.apply(frame,dict(state='player'));self.assertEqual(k.words[6],0);self.assertEqual(k.words[8],0)
  policy.stop();self.assertEqual(k.words[6],0);native.close()
 def test_invalid_native_report_is_rejected_without_command_side_effects(self):
  for field,value in [(24,10),(33,129),(32,2),(27,32768),(28,1001)]:
   k=Kernel();native=NativeCombat(42,k);k.report();k.words[field]=value;before=bytes(k.buffer)
   with self.assertRaises(ReaderError):native.status()
   self.assertEqual(bytes(k.buffer),before);native.close()
 def test_torn_catalogue_is_inactive_and_missing_channel_has_no_stale_targets(self):
  k=Kernel();native=NativeCombat(42,k);k.report();k.words[20]=3
  state=native.status();self.assertFalse(state['active']);self.assertFalse(state['fresh']);self.assertEqual(state['enemies'],[]);native.close()
 def test_build_mismatch_releases_both_mapping_resources(self):
  k=Kernel();k.words[2]=451
  with self.assertRaisesRegex(ReaderError,'DLL0.4.53'):NativeCombat(42,k)
  self.assertEqual(len(k.closed),2)
