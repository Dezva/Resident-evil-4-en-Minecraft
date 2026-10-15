import struct,unittest
from unittest.mock import patch
from re4_combat_probe import describe_profile,FUNCTIONS

class CombatProbeTests(unittest.TestCase):
 def test_read_only_timestamp_function_bytes_and_unique_thunk_call_counts(self):
  base=0x400000;memory=bytearray(0x200000)
  struct.pack_into('<I',memory,0x3C,0x100);struct.pack_into('<I',memory,0x108,0x5A6192E5)
  for j,(name,rva) in enumerate(FUNCTIONS.items()):
   entry=0x1000+j*0x100;memory[entry-5]=memory[entry]=memory[entry+5]=0xE9
   struct.pack_into('<i',memory,entry+1,rva-entry-5);memory[rva:rva+3]=b'\x55\x8b\xec'
   for i in range(3):
    call=0x5000+j*0x100+i*10;memory[call]=0xE8;struct.pack_into('<i',memory,call+1,entry-call-5)
  before=bytes(memory)
  def read(address,n):return bytes(memory[address-base:address-base+n])
  with patch('re4_combat_probe.code_sections',return_value=[(base+0x1000,0x6000)]):report=describe_profile(read,base,len(memory))
  self.assertEqual(report['timestamp'],'0x5a6192e5');self.assertEqual(bytes(memory),before)
  for name,row in report['functions'].items():self.assertEqual(len(row['ilt_entries']),1);self.assertEqual(row['ilt_calls'],3);self.assertTrue(row['bytes'].startswith('55 8b ec'))
