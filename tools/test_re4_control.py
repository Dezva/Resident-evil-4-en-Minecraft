import unittest
from fake_re4_host import ProtocolError
from re4_control import ControlPolicy, validate_control, NativeControl
import ctypes


class Writer:
    def __init__(self):
        self.stop_code = 0
        self.ticking = True
        self.writes = []
    def publish(self, armed, buttons, **kwargs): self.writes.append((armed, buttons))
    def reset_stop(self): self.stop_code = 0
    def status(self): return {"stop": self.stop_code, "ticking": self.ticking, "hook_ticks": 10}


class ControlTests(unittest.TestCase):
    def frame(self, generation=1, armed=True, buttons=1):
        return {"type": "re4_control", "armed": armed, "buttons": buttons, "arm_id": generation}

    def test_emergency_stop_requires_new_arm_generation(self):
        writer = Writer()
        policy = ControlPolicy(writer)
        player = {"state": "player"}
        self.assertTrue(policy.apply(self.frame(), player)["armed"])
        self.assertEqual(writer.writes[-1], (True, 1))
        writer.stop_code = 1
        self.assertFalse(policy.apply(self.frame(), player)["armed"])
        self.assertEqual(writer.writes[-1], (False, 0))
        writer.stop_code = 0
        self.assertFalse(policy.apply(self.frame(), player)["armed"])
        self.assertTrue(policy.apply(self.frame(generation=2), player)["armed"])
        with self.assertRaises(ProtocolError): policy.apply(self.frame(), player)
        policy.stop()
        self.assertEqual(writer.writes[-1], (False, 0))

    def test_missing_player_or_stalled_hook_disarms_without_auto_resume(self):
        for reason in ("player", "hook"):
            writer = Writer()
            policy = ControlPolicy(writer)
            policy.apply(self.frame(), {"state": "player"})
            if reason == "hook": writer.ticking = False
            state = "unavailable" if reason == "player" else "player"
            self.assertFalse(policy.apply(self.frame(), {"state": state})["armed"])
            writer.ticking = True
            self.assertFalse(policy.apply(self.frame(), {"state": "player"})["armed"])
            self.assertTrue(policy.apply(self.frame(generation=2), {"state": "player"})["armed"])

    def test_invalid_frames_never_write_input(self):
        for field, value in [("armed",1),("buttons",True),("buttons",16),("arm_id",0),("arm_id",-1),("type","pose")]:
            writer = Writer()
            frame = self.frame()
            frame[field] = value
            with self.assertRaises(ProtocolError): ControlPolicy(writer).apply(frame,{"state":"player"})
            self.assertEqual(writer.writes, [])
        validate_control(self.frame(generation=0, armed=False, buttons=0))

    def test_run_capability_and_stop_require_explicit_rearm(self):
        writer=Writer()
        policy=ControlPolicy(writer)
        player={'state':'player'}
        status=policy.apply(self.frame(buttons=65),player)
        self.assertFalse(status['armed'])
        self.assertIn('correr',status['reason'])
        old_status=writer.status
        writer.status=lambda: dict(old_status(),capabilities=7)
        self.assertFalse(policy.apply(self.frame(buttons=65),player)['armed'])
        self.assertTrue(policy.apply(self.frame(generation=2,buttons=69),player)['armed'])
        self.assertEqual(writer.writes[-1],(True,69))
        writer.stop_code=1
        self.assertFalse(policy.apply(self.frame(generation=2,buttons=69),player)['armed'])
        self.assertEqual(writer.writes[-1],(False,0))
        for buttons in (16,32,80,128,-1):
            with self.assertRaises(ProtocolError): validate_control(self.frame(buttons=buttons))

    def test_diagnostics_distinguish_request_stop_and_applied_native_input(self):
        writer = Writer()
        original_status = writer.status
        writer.status = lambda: dict(original_status(), native_buttons=8)
        policy = ControlPolicy(writer)
        sample = {"state": "player"}
        status = policy.apply(self.frame(buttons=1), sample)
        self.assertEqual(status['reason'], 'activo')
        self.assertEqual(status['requested_buttons'], 1)
        self.assertEqual(status['native_buttons'], 8)
        status = policy.apply(self.frame(armed=False), sample)
        self.assertEqual(status['reason'], 'cliente desarmado')
        writer.ticking = False
        status = policy.apply(self.frame(generation=2), sample)
        self.assertEqual(status['reason'], 'modulo sin ticks recientes')
        self.assertFalse(status['armed'])

    def test_look_validation_capability_and_explicit_rearm(self):
        look = {'enabled': True, 'first_person': True, 'total_yaw': 22.0, 'pitch': 10.0, 'eye_height': 160.0}
        writer = Writer()
        policy = ControlPolicy(writer)
        frame = dict(self.frame(), look=look)
        self.assertFalse(policy.apply(frame, {'state':'player'})['armed'])
        for field, value in [('total_yaw',float('nan')), ('pitch',86), ('eye_height',0),
                             ('eye_height',2501), ('eye_height',-2501), ('enabled',1)]:
            invalid = dict(frame, look=dict(look, **{field:value}))
            with self.assertRaises(ProtocolError): validate_control(invalid)
        original_status = writer.status
        writer.status = lambda: dict(original_status(), capabilities=3)
        for height in (1600, 2000, -2000, 2500):
            validate_control(dict(frame, look=dict(look, eye_height=height)))
        # Rejected generation is not allowed to arm automatically when capabilities appear.
        self.assertFalse(policy.apply(frame, {'state':'player'})['armed'])
        self.assertTrue(policy.apply(dict(frame, arm_id=2), {'state':'player'})['armed'])

    def test_ipc_v2_publication_preserves_native_status_and_disarms_look(self):
        native = NativeControl.__new__(NativeControl)
        backing = (ctypes.c_uint32 * 20)()
        native.version = 2
        native.words = backing
        native.address = ctypes.addressof(backing)
        class Kernel:
            def GetTickCount(self): return 123
        native.k = Kernel()
        backing[15] = 3; backing[17] = 50
        look = {'enabled': True, 'first_person': True, 'total_yaw': 22.0, 'pitch': 10.0, 'eye_height': 1600.0}
        native.publish(True, 1, look, 7)
        self.assertEqual(backing[2] % 2, 0)
        self.assertEqual((backing[4],backing[5],backing[13],backing[14]), (1,1,3,7))
        self.assertEqual(ctypes.c_float.from_address(native.address+40).value, 22)
        self.assertEqual(ctypes.c_float.from_address(native.address+48).value, 1600)
        self.assertEqual((backing[15],backing[17]), (3,50))
        native.publish(False,0)
        self.assertEqual((backing[4],backing[5],backing[13]), (0,0,0))


if __name__ == "__main__": unittest.main()
