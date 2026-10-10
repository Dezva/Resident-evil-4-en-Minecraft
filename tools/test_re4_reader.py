"""Synthetic process-memory tests; these do not replace the Windows game test."""
import math
import struct
import unittest
from re4_reader import (PLAYER_SIGNATURE, VERSION_SIGNATURE, ReaderError,
                        code_sections, locate_player_slot, matches, sample_player)


class Memory:
    base = 0x400000
    size = 0x5000
    slot = base + 0x3100
    player = base + 0x4000

    def __init__(self, base=0x400000):
        self.base = base
        self.slot = base + 0x3100
        self.player = base + 0x4000
        self.data = bytearray(self.size)
        self.data[:2] = b"MZ"
        struct.pack_into("<I", self.data, 0x3C, 0x80)
        self.data[0x80:0x84] = b"PE\0\0"
        struct.pack_into("<HH", self.data, 0x84, 0x14C, 2)
        struct.pack_into("<H", self.data, 0x94, 0xE0)
        struct.pack_into("<H", self.data, 0x98, 0x10B)
        struct.pack_into("<I", self.data, 0x98 + 56, self.size)
        for offset, rva, flags in [(0x178, 0x1000, 0x60000020), (0x1A0, 0x3000, 0xC0000040)]:
            struct.pack_into("<II", self.data, offset + 8, 0x1000, rva)
            struct.pack_into("<I", self.data, offset + 36, flags)
        self.put_signature(0x1100, VERSION_SIGNATURE, self.base + 0x3000)
        self.put_signature(0x1200, PLAYER_SIGNATURE, self.slot)
        self.data[0x3000:0x3006] = b"1.1.0\0"
        struct.pack_into("<I", self.data, 0x3100, self.player)
        struct.pack_into("<I", self.data, 0x4000, self.base + 0x3200)
        struct.pack_into("<6f", self.data, 0x4094, 1.5, -2, 3, 0, 0.7, 0)

    def put_signature(self, offset, signature, operand):
        data = bytearray(0 if p == "?" else int(p, 16) for p in signature.split())
        struct.pack_into("<I", data, 1, operand)
        self.data[offset:offset + len(data)] = data

    def read(self, address, size):
        offset = address - self.base
        if offset < 0 or offset + size > len(self.data):
            raise ReaderError("Unmapped test memory")
        return bytes(self.data[offset:offset + size])


class ReaderTests(unittest.TestCase):
    def test_resolve_and_read_pose_with_relocated_image(self):
        for base in (0x400000, 0x15000000):
            with self.subTest(base=base):
                memory = Memory(base)
                self.assertEqual(locate_player_slot(memory.read, memory.base, memory.size), memory.slot)
                pose = sample_player(memory.read, memory.slot, memory.base, memory.size)
                self.assertEqual(pose["state"], "player")
                self.assertEqual(pose["position_raw"], [1.5, -2, 3])
                self.assertAlmostEqual(pose["angles_raw"][1], 0.7)

    def test_missing_duplicate_or_data_only_signature_rejected(self):
        for mutation in ("missing", "duplicate", "data_only"):
            with self.subTest(mutation=mutation):
                m = Memory()
                if mutation == "duplicate":
                    m.put_signature(0x1300, PLAYER_SIGNATURE, m.slot)
                else:
                    m.data[0x1200] = 0
                    if mutation == "data_only":
                        m.put_signature(0x3300, PLAYER_SIGNATURE, m.slot)
                with self.assertRaises(ReaderError):
                    locate_player_slot(m.read, m.base, m.size)

    def test_wrong_version_and_operand_outside_image_rejected(self):
        m = Memory()
        m.data[0x3000:0x3006] = b"1.0.6\0"
        with self.assertRaisesRegex(ReaderError, "Version"):
            locate_player_slot(m.read, m.base, m.size)
        m = Memory()
        m.put_signature(0x1200, PLAYER_SIGNATURE, 0x1234)
        with self.assertRaisesRegex(ReaderError, "fuera"):
            locate_player_slot(m.read, m.base, m.size)

    def test_malformed_pe_rejected(self):
        for address, fmt, value in [(0x3C, "I", 0xFFFFFF), (0x84, "H", 0x8664),
                                    (0x86, "H", 97), (0x98, "H", 0x20B),
                                    (0x178 + 8, "I", 0x5000)]:
            with self.subTest(address=address):
                m = Memory()
                struct.pack_into("<" + fmt, m.data, address, value)
                with self.assertRaises(ReaderError):
                    code_sections(m.read, m.base, m.size)

    def test_menu_transition_invalid_object_and_nan(self):
        m = Memory()
        struct.pack_into("<I", m.data, 0x3100, 0)
        self.assertEqual(sample_player(m.read, m.slot, m.base, m.size)["state"], "waiting_player")
        m = Memory()
        reads = 0
        def changing(address, size):
            nonlocal reads
            if address == m.slot:
                reads += 1
                if reads == 2:
                    return b"\0" * 4
            return m.read(address, size)
        self.assertEqual(sample_player(changing, m.slot, m.base, m.size)["state"], "transition")
        for offset, fmt, value in [(0x4000, "I", 1), (0x4094, "f", math.nan), (0x3100, "I", 3)]:
            m = Memory()
            struct.pack_into("<" + fmt, m.data, offset, value)
            with self.assertRaises(ReaderError):
                sample_player(m.read, m.slot, m.base, m.size)

    def test_wildcard_scan_and_truncated_match(self):
        self.assertEqual(matches(b"\xA1\x00\xCC\xA1\xFF\xCC\xA1", "A1 ? CC"), [0, 3])


if __name__ == "__main__":
    unittest.main()
