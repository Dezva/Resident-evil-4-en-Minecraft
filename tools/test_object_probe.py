import struct,unittest
from unittest.mock import patch
from re4_object_probe import snapshot,ENEMY,GLOBAL,MESSAGE
class ObjectProbeTests(unittest.TestCase):
 def test_hazard_profile_reads_both_bounded_handlers_only_for_verified_dispatch(self):
  base=0x400000;memory=bytearray(0x200000)
  entry=0xa209;move=0xaaa00
  memory[entry]=0xe9;struct.pack_into('<i',memory,entry+1,move-entry-5)
  prefix=bytes.fromhex('56 8b f1 80 be fc 00 00 00 00 74 1b 0f b6 86 01 01 00 00 48 74 0a 48 74 07')
  memory[move:move+len(prefix)]=prefix
  for offset,target in [(0x19,0xa8840),(0x22,0xaa920)]:
   memory[move+offset]=0xe8;struct.pack_into('<i',memory,move+offset+1,target-move-offset-5)
  def read(address,n):
   self.assertGreaterEqual(address,base);self.assertLessEqual(address+n,base+len(memory));return bytes(memory[address-base:address-base+n])
  with patch('re4_object_probe.sample_player',return_value={'state':'player','position_raw':[0,0,0]}),patch('re4_object_probe.code_sections',return_value=[]):
   before=bytes(memory);report=snapshot(read,base,len(memory),1,True);self.assertEqual(bytes(memory),before)
   self.assertEqual(report['code']['trap_impact']['rva'],'0xa8840');self.assertEqual(len(bytes.fromhex(report['code']['mine_impact']['bytes'])),2048)
   memory[move]=0xcc;report=snapshot(read,base,len(memory),1,True)
   self.assertNotIn('trap_impact',report['code'])
 def test_snapshot_keeps_zero_hp_boxes_and_dialog_choices_without_writing(self):
  base=0x400000;memory=bytearray(0x200000)
  for offset,signature,operand,value in [(0x100,ENEMY,13,0x1000),(0x200,GLOBAL,1,0x800),(0x300,MESSAGE,5,0x10000)]:
   raw=bytes(int(v,16) if v!='?' else 0 for v in signature.split());memory[offset:offset+len(raw)]=raw;struct.pack_into('<I',memory,offset+operand,base+value)
  struct.pack_into('<I',memory,0x800,base+0x20000);memory[0x20020]=3;struct.pack_into('<4I',memory,0x2501c,0x1000,2,0,0)
  struct.pack_into('<4I',memory,0x1000,base+0x5000,base+0x2000,1,0x408)
  struct.pack_into('<2I',memory,0x2000,base+0x5100,1);memory[0x20fc]=1;memory[0x2100]=0x53
  struct.pack_into('<I',memory,0x10010,1);memory[0x10100]=2
  def read(address,n):return bytes(memory[address-base:address-base+n])
  before=bytes(memory)
  with patch('re4_object_probe.sample_player',return_value={'state':'player','position_raw':[0,0,0]}),patch('re4_object_probe.code_sections',return_value=[(base+0x100,0x400)]):report=snapshot(read,base,len(memory),1)
  self.assertEqual(bytes(memory),before);self.assertEqual(report['status_flags'][1],'0x2');self.assertEqual(report['messages'][0]['choices'],2)
  actor=report['pools'][0]['actors'][0];self.assertEqual(actor['kind'],'0x53');self.assertEqual(actor['hp'],[0,0])
