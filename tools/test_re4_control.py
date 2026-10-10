import unittest
from fake_re4_host import ProtocolError
from re4_control import ControlPolicy, validate_control


class Writer:
    def __init__(self):
        self.stop_code = 0
        self.ticking = True
        self.writes = []
    def publish(self, armed, buttons): self.writes.append((armed, buttons))
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


if __name__ == "__main__": unittest.main()
