import struct,unittest
from unittest.mock import patch
from re4_object_probe import snapshot,ENEMY,GLOBAL,MESSAGE
class ObjectProbeTests(unittest.TestCase):
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
