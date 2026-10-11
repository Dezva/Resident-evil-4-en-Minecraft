import math
import struct
import unittest

from re4_camera_trace import decode_diagnostics
from re4_reader import ReaderError


class ProjectionDiagnosticTest(unittest.TestCase):
    def sample(self, **changes):
        words = [0x52453444,1,22,409,123,10,7,3,900,81,80,1]
        angles = [0.2,0.0,0.21,0.0]
        if 'sequence' in changes: words[2]=changes['sequence']
        if 'version' in changes: words[1]=changes['version']
        if 'angle' in changes: angles[0]=changes['angle']
        return struct.pack('<12I4f',*words,*angles)

    def test_preserves_before_written_after_and_loaded_build(self):
        row=decode_diagnostics(self.sample())
        self.assertEqual(row['build'],409)
        self.assertEqual(row['calls'],row['writes']+row['rejected'])
        self.assertEqual(row['yaw_written'],0.0)
        self.assertAlmostEqual(row['yaw_after'],0.21,places=6)
        self.assertEqual(row['pid'],123)

    def test_odd_sequence_is_not_a_complete_projection(self):
        with self.assertRaises(ReaderError): decode_diagnostics(self.sample(sequence=23))

    def test_unknown_and_short_mapping_rejected(self):
        with self.assertRaises(ReaderError): decode_diagnostics(self.sample(version=2))
        with self.assertRaises(ReaderError): decode_diagnostics(self.sample()[:48])

    def test_nonfinite_angles_rejected(self):
        with self.assertRaises(ReaderError): decode_diagnostics(self.sample(angle=math.nan))


if __name__=='__main__': unittest.main()
