"""Synthetic audit tests. No Windows hook is installed or executed."""
import struct
import unittest
from re4_native_probe import inspect, KEY_SIGNATURE, PAD_SIGNATURE
from test_re4_reader import Memory


def fixture():
    m = Memory()
    m.put_signature(0x1300, KEY_SIGNATURE, m.base + 0x3200)
    m.put_signature(0x1400, PAD_SIGNATURE, 0x1600 - 0x1405)
    m.data[0x1600] = 0xe8
    struct.pack_into('<i', m.data, 0x1601, 0x1700 - 0x1605)
    return m


class ProbeTests(unittest.TestCase):
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
