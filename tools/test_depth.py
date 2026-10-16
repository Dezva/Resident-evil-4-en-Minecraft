import ctypes,struct,unittest
from re4_depth import NativeDepth
from re4_reader import ReaderError
class Kernel:
 def __init__(self):
  self.buffer=ctypes.create_string_buffer(256);struct.pack_into('<5I',self.buffer,0,0x52453444,1,2,435,42)
  self.access=[];self.closed=0
 def OpenFileMappingW(self,access,inherit,name):self.access.append(access);assert name=='Local\\RE4Bridge-Depth-42';return 1
 def MapViewOfFile(self,*args):return ctypes.addressof(self.buffer)
 def UnmapViewOfFile(self,*args):pass
 def CloseHandle(self,*args):self.closed+=1
 def GetTickCount(self):return 12345
class DepthTests(unittest.TestCase):
 def test_command_writes_only_host_owned_fields_and_readonly_never_writes(self):
  k=Kernel();depth=NativeDepth(k,42);before=bytes(k.buffer);depth.publish(True,7)
  self.assertEqual(bytes(k.buffer)[:208],before[:208]);self.assertEqual(bytes(k.buffer)[224:],before[224:])
  self.assertEqual(struct.unpack_from('<3I',k.buffer,212),(12345,1,7));depth.close()
  k=Kernel();depth=NativeDepth(k,42,readonly=True);before=bytes(k.buffer);depth.publish(True,7);depth.close();self.assertEqual(bytes(k.buffer),before);self.assertEqual(k.access,[4])
 def test_busy_diagnostics_use_cache_and_invalid_mapping_is_rejected(self):
  k=Kernel();depth=NativeDepth(k,42);status=depth.status();struct.pack_into('<I',k.buffer,8,3);self.assertEqual(depth.status(),status);depth.close()
  k=Kernel();struct.pack_into('<I',k.buffer,12,999)
  with self.assertRaises(ReaderError):NativeDepth(k,42)
  self.assertEqual(k.closed,1)
 def test_compatibility_draw_count_is_distinct_from_depth_success(self):
  for build in (435,436,437,438,439,441,442,443,444,445,446,447,448,449,450,451,452,453,455,456):
   k=Kernel();struct.pack_into('<I',k.buffer,12,build);struct.pack_into('<I',k.buffer,20,2);struct.pack_into('<I',k.buffer,60,7)
   depth=NativeDepth(k,42);status=depth.status();self.assertEqual(status['depth_draws'],0);self.assertEqual(status['depth_fallback_draws'],7);self.assertEqual(status['depth_reason_code'],2);depth.close()
 def test_nonfinite_projection_is_fatal_not_reported_as_valid(self):
  k=Kernel();depth=NativeDepth(k,42);struct.pack_into('<f',k.buffer,64,float('nan'))
  with self.assertRaises(ReaderError):depth.status()
  depth.close()

 def test_patch_viewport_marker_distinguishes_old_reserved_tail_and_rejects_nonfinite(self):
  k=Kernel();depth=NativeDepth(k,42);self.assertNotIn('depth_min_z',depth.status())
  struct.pack_into('<2f',k.buffer,240,.25,.75);struct.pack_into('<I',k.buffer,252,0x44584631)
  status=depth.status();self.assertEqual((status['depth_min_z'],status['depth_max_z']),(.25,.75))
  struct.pack_into('<f',k.buffer,240,float('nan'))
  with self.assertRaises(ReaderError):depth.status()
  depth.close()
