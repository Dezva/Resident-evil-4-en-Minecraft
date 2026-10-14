"""Synthetic audit tests. No Windows hook is installed or executed."""
import struct
import unittest
from re4_native_probe import (inspect, KEY_SIGNATURE, PAD_SIGNATURE, GLOBAL_SIGNATURE, PROJECTION_SIGNATURE,
                              observed_projection_profile, OBSERVED_PROJECTION_CALLS, ENEMY_SIGNATURE,
                              inspect_enemy_manager,sample_native_input)
from test_re4_reader import Memory


def fixture():
    m = Memory()
    m.put_signature(0x1300, KEY_SIGNATURE, m.base + 0x3200)
    m.put_signature(0x1400, PAD_SIGNATURE, 0x1600 - 0x1405)
    m.data[0x1600] = 0xe8
    struct.pack_into('<i', m.data, 0x1601, 0x1700 - 0x1605)
    return m


class ProbeTests(unittest.TestCase):
    def test_manager_details_distinguish_small_pool_from_live_cem_layout_without_writes(self):
        m=fixture();struct.pack_into('<4I',m.data,0x3400,m.base+0x3200,m.base+0x4000,1,0x408)
        struct.pack_into('<II',m.data,0x4000,m.base+0x3200,1)
        m.data[0x40fc]=1;m.data[0x4100]=0x12;struct.pack_into('<h',m.data,0x4324,100)
        before=bytes(m.data);row=inspect_enemy_manager(m.read,m.base,m.size,m.base+0x3400)
        self.assertTrue(row['cem_layout_possible']);self.assertEqual(row['ids'],{'0x12':1});self.assertEqual(row['live_cem_fields'],1)
        self.assertEqual(bytes(m.data),before)
        struct.pack_into('<I',m.data,0x340c,0x100)
        self.assertFalse(inspect_enemy_manager(m.read,m.base,m.size,m.base+0x3400)['cem_layout_possible'])

    def test_bounded_native_input_capture_reports_real_edges_and_never_writes(self):
        elapsed=[0.];memory=bytearray(16)
        def read(address,size):
            struct.pack_into('<QQ',memory,0,0x400 if elapsed[0]>=.01 else 0,0x400 if .01<=elapsed[0]<.015 else 0)
            return bytes(memory[address-0x1000:address-0x1000+size])
        def wait(seconds):elapsed[0]+=seconds
        rows=sample_native_input(read,0x1000,dict(on_rva='0x0',trigger_rva='0x8'),.02,lambda:elapsed[0],wait)
        self.assertEqual([(r['native_on'],r['native_trigger']) for r in rows],[('0x0','0x0'),('0x400','0x400'),('0x400','0x0')])
        for duration in (-1,21):
            with self.assertRaises(ValueError):sample_native_input(read,0x1000,{},duration)

    def test_enemy_sites_with_shared_or_distinct_operands_are_reported_read_only(self):
        m=fixture()
        signature=bytes(0 if p=='?' else int(p,16) for p in ENEMY_SIGNATURE.split())
        for offset in (0x1800,0x1900):
            m.data[offset:offset+len(signature)]=signature
            struct.pack_into('<I',m.data,offset+13,m.base+0x3400)
        struct.pack_into('<4I',m.data,0x3400,m.base+0x3200,m.base+0x4000,1,0x408)
        before=bytes(m.data);report=inspect(m.read,m.base,m.size)
        self.assertEqual(report['checks']['enemy_manager_matches'],2)
        self.assertTrue(report['checks']['enemy_manager_shared_target'])
        self.assertEqual(report['enemy_manager']['capacity'],1)
        self.assertEqual(bytes(m.data),before)
        struct.pack_into('<I',m.data,0x190d,m.base+0x3500)
        report=inspect(m.read,m.base,m.size)
        self.assertFalse(report['checks']['enemy_manager_shared_target']);self.assertNotIn('enemy_manager',report)

    def test_observed_profile_requires_all_calls_and_both_verified_destinations(self):
        rows = [dict(call_rva=hex(call),target_rva=hex(0x344f if call==0x1f7bad else 0xe395),
                     resolved_target_rva=hex(0x202f60 if call==0x1f7bad else 0x19f3a0))
                for call in sorted(OBSERVED_PROJECTION_CALLS)]
        self.assertTrue(observed_projection_profile(rows))
        self.assertFalse(observed_projection_profile(rows[:-1]))
        rows[0]['resolved_target_rva'] = '0x19f3a1'
        self.assertFalse(observed_projection_profile(rows))
        rows[0]['resolved_target_rva'] = '0x19f3a0'
        rows[0]['resolved_target_executable'] = False
        self.assertFalse(observed_projection_profile(rows))

    def test_seven_projection_calls_require_one_target(self):
        m = fixture()
        for i in range(7):
            rva = 0x1800+0x20*i
            m.put_signature(rva,PROJECTION_SIGNATURE,(0x1700-rva-5) & 0xFFFFFFFF)
        before = bytes(m.data)
        report = inspect(m.read,m.base,m.size)
        self.assertEqual(report['checks']['projection_matches'],7)
        self.assertEqual(report['checks']['projection_unique_targets'],1)
        self.assertTrue(report['checks']['projection_shared_target'])
        self.assertEqual(len(report['projection_calls']),7)
        self.assertEqual(bytes(m.data),before)
        struct.pack_into('<i',m.data,0x1801,0x1701-0x1805)
        report = inspect(m.read,m.base,m.size)
        self.assertFalse(report['checks']['projection_shared_target'])
        self.assertEqual(report['checks']['projection_unique_targets'],2)

    def test_render_prerequisites_and_invalid_target_are_read_only(self):
        m = fixture()
        m.put_signature(0x1800, GLOBAL_SIGNATURE, m.base+0x3300)
        m.put_signature(0x1900, PROJECTION_SIGNATURE, (0x1700-0x1905) & 0xFFFFFFFF)
        before = bytes(m.data)
        report = inspect(m.read,m.base,m.size,lambda address: {'remaining':m.size,'protection':0x20,'committed':True})
        self.assertTrue(report['checks']['global_slot_in_module'])
        self.assertTrue(report['checks']['projection_target_in_module'])
        self.assertTrue(report['checks']['projection_target_executable'])
        self.assertEqual(bytes(m.data),before)
        struct.pack_into('<i',m.data,0x1901,0x6000)
        report = inspect(m.read,m.base,m.size)
        self.assertFalse(report['checks']['projection_target_in_module'])

    def test_valid_audit_is_read_only_and_reports_split_section(self):
        m = fixture()
        before = bytes(m.data)
        def query(address):
            return {"remaining": 0x800, "protection": 4 if address >= m.base + 0x3000 else 0x20,
                    "committed": True}
        report = inspect(m.read, m.base, m.size, query)
        self.assertEqual(report['checks']['pad_matches'], 1)
        self.assertTrue(report['checks']['target_executable'])
        self.assertTrue(report['checks']['keys_writable'])
        self.assertTrue(report['checks']['version_1_1_0'])
        self.assertFalse(report['sections'][0]['fits_single_region'])
        self.assertEqual(before, bytes(m.data))

    def test_missing_and_ambiguous_signatures_reported(self):
        m = fixture()
        m.data[0x1400] = 0
        report = inspect(m.read, m.base, m.size)
        self.assertEqual(report['checks']['pad_matches'], 0)
        self.assertNotIn('wrapper_in_module', report['checks'])
        m = fixture()
        m.put_signature(0x1800, KEY_SIGNATURE, m.base + 0x3200)
        report = inspect(m.read, m.base, m.size)
        self.assertEqual(report['checks']['keys_matches'], 2)
        self.assertNotIn('keys_pointer_in_module', report['checks'])

    def test_invalid_nested_call_and_version_identified(self):
        m = fixture()
        m.data[0x1600] = 0x90
        m.data[0x3000:0x3006] = b'1.0.6\0'
        report = inspect(m.read, m.base, m.size)
        self.assertFalse(report['checks']['wrapper_starts_call'])
        self.assertFalse(report['checks']['version_1_1_0'])
        self.assertNotIn('target_in_module', report['checks'])
        self.assertTrue(report['branch_details']['wrapper_bytes'].startswith('90 '))

    def test_relative_jump_is_reported_without_installing_hook(self):
        m = fixture()
        m.data[0x1600] = 0xe9
        before = bytes(m.data)
        report = inspect(m.read, m.base, m.size)
        self.assertFalse(report['checks']['wrapper_starts_call'])
        self.assertTrue(report['checks']['wrapper_starts_jump'])
        self.assertTrue(report['checks']['target_in_module'])
        self.assertEqual(report['branch_details']['wrapper_rva'], '0x1600')
        self.assertEqual(report['branch_details']['target_rva'], '0x1700')
        self.assertEqual(bytes(m.data), before)


if __name__ == '__main__':
    unittest.main()
