"""Synthetic trace checks; actual camera behavior still requires Windows RE4."""
import ctypes
import math
import struct
import unittest
from unittest.mock import Mock, patch

from re4_camera_trace import (CAMERA_CTRL_SIGNATURE, SMOOTH_CTRL_SIGNATURE,
                              ReadOnlyIPC, camera_addresses, decode_ipc, observation)
from re4_reader import ReaderError
from re4_native_probe import GLOBAL_SIGNATURE
from test_re4_reader import Memory


class CameraTraceTests(unittest.TestCase):
    def memory(self):
        m = Memory()
        for offset, signature, operand_offset, rva in [
            (0x1400, CAMERA_CTRL_SIGNATURE, 3, 0x2200),
            (0x1500, SMOOTH_CTRL_SIGNATURE, 2, 0x3500),
        ]:
            data = bytearray(0 if p == '?' else int(p, 16) for p in signature.split())
            struct.pack_into('<I', data, operand_offset, m.base + rva)
            m.data[offset:offset+len(data)] = data
        return m

    def test_records_camera_changes_independently_of_player_without_writes(self):
        m = self.memory()
        addresses = camera_addresses(m.read, m.base, m.size)
        struct.pack_into('<8f', m.data, 0x2238, 0, 0, 0, 0, 0, 1, 0, 60)
        original = bytes(m.data)
        moving = observation(m.read, m.base, m.size, m.slot, addresses)
        self.assertEqual(bytes(m.data), original)
        struct.pack_into('<8f', m.data, 0x2238, 0, 0, 0, 1, 0, 0, 0, 60)
        idle = observation(m.read, m.base, m.size, m.slot, addresses)
        self.assertEqual(moving['player'], idle['player'])
        self.assertEqual(moving['ctrl_point']['yaw_from_points'], 0)
        self.assertAlmostEqual(idle['ctrl_point']['yaw_from_points'], math.pi/2)

    def test_missing_ambiguous_and_out_of_image_camera_rejected(self):
        for mode in ('missing', 'duplicate', 'outside'):
            m = self.memory()
            if mode == 'missing': m.data[0x1400] = 0
            elif mode == 'duplicate':
                length = len(CAMERA_CTRL_SIGNATURE.split())
                m.data[0x1600:0x1600+length] = m.data[0x1400:0x1400+length]
            else: struct.pack_into('<I', m.data, 0x1403, m.base+m.size-4)
            with self.assertRaises(ReaderError): camera_addresses(m.read, m.base, m.size)

    def test_nonfinite_camera_rejected(self):
        m = self.memory()
        addresses = camera_addresses(m.read, m.base, m.size)
        struct.pack_into('<f', m.data, 0x2238, math.nan)
        with self.assertRaises(ReaderError): observation(m.read, m.base, m.size, m.slot, addresses)

    def test_global_camera_snapshot_is_read_only_and_optional(self):
        m = self.memory()
        m.put_signature(0x1700, GLOBAL_SIGNATURE, m.base+0x3200)
        struct.pack_into('<I', m.data, 0x3200, m.base+0x3800)
        struct.pack_into('<8f',m.data,0x3800+0x74+0xA4,1,2,3,2,2,3,0,50)
        before = bytes(m.data)
        addresses = camera_addresses(m.read,m.base,m.size)
        row = observation(m.read,m.base,m.size,m.slot,addresses)
        self.assertAlmostEqual(row['global_camera_point']['yaw_from_points'],math.pi/2)
        self.assertEqual(bytes(m.data),before)
        struct.pack_into('<I',m.data,0x3200,0)
        self.assertNotIn('global_camera_point',observation(m.read,m.base,m.size,m.slot,addresses))

    def test_trace_collision_metrics_are_readonly_and_include_reference_reasons(self):
        from re4_camera_trace import add_barrier_observation
        from test_cube_test import Kernel
        kernel=Kernel(version=6,build=429);before=kernel.collision.raw;row={}
        add_barrier_observation(row,kernel,42)
        self.assertEqual(kernel.collision.raw,before);self.assertEqual(row['collision_state']['reason'],1)
        self.assertEqual(row['cube_state']['build'],429)
        self.assertNotIn('collision_error',row)

    def frame(self):
        data = bytearray(80)
        struct.pack_into('<10I', data, 0, 0x52453443, 2, 2, 100, 1, 0, 0, 1, 10, 0)
        struct.pack_into('<3f', data, 40, 20, 0.2, 1600)
        struct.pack_into('<3I', data, 52, 3, 1, 3)
        struct.pack_into('<f3I', data, 64, 0.7, 8, 4, 1)
        return data

    def test_ipc_versions_and_invalid_frames(self):
        data = self.frame()
        self.assertTrue(decode_ipc(data)['camera_active'])
        with self.assertRaises(ReaderError): decode_ipc(data[:40])
        struct.pack_into('<I', data, 4, 1)
        self.assertEqual(decode_ipc(data[:40])['armed'], 1)
        struct.pack_into('<I', data, 0, 0)
        with self.assertRaises(ReaderError): decode_ipc(data)
        data = self.frame()
        struct.pack_into('<f', data, 64, math.nan)
        with self.assertRaises(ReaderError): decode_ipc(data)

    def test_ipc_handles_request_read_access_only_and_never_change_frame(self):
        data = self.frame()
        memory = ctypes.create_string_buffer(bytes(data), 80)
        kernel = Mock()
        kernel.OpenFileMappingW.return_value = 7
        kernel.MapViewOfFile.return_value = ctypes.addressof(memory)
        with patch.object(ctypes, 'WinDLL', return_value=kernel, create=True):
            ipc = ReadOnlyIPC(99)
            self.assertEqual(ipc.sample()['armed'], 1)
            struct.pack_into('<I', memory, 8, 3)
            self.assertEqual(ipc.sample(), {'state': 'torn_sample'})
            struct.pack_into('<I', memory, 8, 2)
            ipc.close()
        self.assertEqual(memory.raw, bytes(data))
        self.assertEqual(kernel.OpenFileMappingW.call_args.args[0], 4)
        self.assertEqual([call.args[1] for call in kernel.MapViewOfFile.call_args_list], [4, 4])
        kernel.CloseHandle.assert_called_once_with(7)


if __name__ == '__main__': unittest.main()
