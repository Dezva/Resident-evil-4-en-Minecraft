import unittest
from copy import deepcopy
import contextlib
import io
import json
from pathlib import Path
import struct
import tempfile
from types import SimpleNamespace
from unittest.mock import Mock,patch
import re4_movement_measure as measure
from re4_movement_measure import capture_window,key_address,summarize
from re4_native_probe import KEY_SIGNATURE
from re4_reader import ReaderError
from test_re4_reader import Memory


class MovementMeasureTests(unittest.TestCase):
    def test_cli_runs_both_phases_with_real_readonly_reader_and_process_without_pid_attribute(self):
        memory=Memory();memory.put_signature(0x1300,KEY_SIGNATURE,memory.base+0x3300)
        timer=[0.0];phase=[0]
        def read(address,size):
            if address==memory.base+0x3300 and size==8:return struct.pack('<Q',1|(64 if phase[0]==2 else 0))
            if address==memory.player+0x94 and size==24:return struct.pack('<6f',timer[0]*3000,0,0,0,.7,0)
            return memory.read(address,size)
        process=SimpleNamespace(base=memory.base,size=memory.size,k=object(),read=read,close=Mock())
        def beep(frequency,duration):
            if frequency==1100:phase[0]+=1
        def window(snapshot,seconds):
            return capture_window(snapshot,seconds,clock=lambda:timer[0],sleep=lambda dt:timer.__setitem__(0,timer[0]+dt))
        with tempfile.TemporaryDirectory() as folder,contextlib.ExitStack() as stack:
            stack.enter_context(patch.object(measure,'__file__',str(Path(folder)/'tools/script.py')))
            stack.enter_context(patch.object(measure,'find_pid',return_value=42))
            stack.enter_context(patch.object(measure,'WindowsProcess',return_value=process))
            stack.enter_context(patch.object(measure,'ReadOnlyIPC',side_effect=ReaderError('Absent')))
            stack.enter_context(patch.object(measure,'read_collision_status',return_value=None))
            stack.enter_context(patch.object(measure.ctypes,'WinDLL',return_value=Mock(),create=True))
            stack.enter_context(patch.object(measure,'wait_start'))
            stack.enter_context(patch.object(measure,'capture_window',side_effect=window))
            stack.enter_context(patch.object(measure.time,'sleep'))
            stack.enter_context(patch.dict('sys.modules',{'winsound':SimpleNamespace(Beep=beep)}))
            stack.enter_context(patch('sys.argv',['measure','--seconds','2']))
            stack.enter_context(contextlib.redirect_stdout(io.StringIO()))
            self.assertEqual(measure.main(),0)
            result=json.loads(next(Path(folder).glob('movement-measure-*.json')).read_text())
            self.assertEqual([p['name'] for p in result['phases']],['caminar','correr'])
            self.assertTrue(all(p['summary']['valid'] for p in result['phases']))
        process.close.assert_called_once()
    def rows(self,speed=3000,run=False):
        return [dict(time=i*.05,player=dict(state='player',position_raw=[i*.05*speed,0,0]),keys=1|(64 if run else 0),
                     control=dict(armed=True,arm_id=1,hook_ticks=i*3),collision=dict(contacts=5)) for i in range(81)]

    def test_real_distance_time_and_input_cadence_not_fps(self):
        rows=self.rows();before=deepcopy(rows);result=summarize(rows)
        self.assertTrue(result['valid']);self.assertAlmostEqual(result['units_per_second'],3000)
        self.assertAlmostEqual(result['blocks_per_second'],3);self.assertAlmostEqual(result['pad_callbacks_per_second'],60)
        self.assertEqual(rows,before)
        self.assertTrue(summarize(self.rows(run=True),sprint=True)['valid'])
        self.assertFalse(summarize(rows,sprint=True)['valid'])

    def test_contact_teleport_missing_player_and_dearm_reject_calibration(self):
        for failure in range(4):
            rows=self.rows()
            if failure==0:rows[-1]['collision']['contacts']+=1
            if failure==1:rows[10]['player']['position_raw'][0]+=10000
            if failure==2:rows[10]['player']['state']='transition'
            if failure==3:rows[-1]['control']['armed']=False
            self.assertFalse(summarize(rows)['valid'])

    def test_timed_sampling_uses_monotonic_boundary_without_writes(self):
        timer=[0.0]
        rows=capture_window(lambda:dict(player=dict(state='player',position_raw=[timer[0]*3000,0,0]),keys=1),2,
                            clock=lambda:timer[0],sleep=lambda dt:timer.__setitem__(0,timer[0]+dt))
        self.assertEqual(rows[-1]['time'],2);self.assertTrue(summarize(rows)['valid'])

    def test_real_pattern_operand_is_bounded_and_readonly(self):
        memory=Memory();memory.put_signature(0x1300,KEY_SIGNATURE,memory.base+0x3300)
        before=bytes(memory.data);self.assertEqual(key_address(memory.read,memory.base,memory.size),memory.base+0x3300)
        self.assertEqual(bytes(memory.data),before)
        memory.put_signature(0x1400,KEY_SIGNATURE,memory.base+0x3300)
        with self.assertRaises(ReaderError):key_address(memory.read,memory.base,memory.size)
