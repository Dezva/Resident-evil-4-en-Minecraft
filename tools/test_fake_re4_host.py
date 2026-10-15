import io
import json
from pathlib import Path
import socket
import tempfile
import threading
import unittest
import uuid

import fake_re4_host as host


class SimulatorTests(unittest.TestCase):
    def test_rejects_bad_frames(self):
        for data in [b'{"protocol":true}\n', b'{"protocol":2}\n', b'{"protocol":1,"x":NaN}\n',
                     b'{"protocol":1}', b'\xff\n', b'x' * (host.MAX_FRAME_BYTES + 1) + b'\n']:
            with self.subTest(data=data[:50]), self.assertRaises(host.ProtocolError):
                host.read_frame(io.BytesIO(data))

    def test_slab_bounds_and_removed_blocks(self):
        frame = self.collision()
        host.validate_sample(frame)
        frame["blocks"][0]["boxes"][0][4] = 0
        with self.assertRaises(host.ProtocolError):
            host.validate_sample(frame)

    @staticmethod
    def collision():
        return {"type": "collision_snapshot", "sample_ns": 1, "dimension": "minecraft:overworld",
                "center": [0, 64, 0], "complete": True, "blocks": [
                    {"pos": [0, 63, 0], "state": "minecraft:stone_slab[type=bottom]",
                     "collision_complete": True, "boxes": [[0, 0, 0, 1, 0.5, 1]]}]}

    def test_authenticated_exchange_and_snapshot_replacement(self):
        client, peer = socket.socketpair()
        report, errors = {}, []
        token = "a" * 64
        def serve():
            try:
                with peer:
                    host.serve_peer(peer, token, report)
            except Exception as exc:
                errors.append(exc)
        thread = threading.Thread(target=serve)
        thread.start()
        session = str(uuid.uuid4())
        with client, client.makefile("rwb") as stream:
            host.write_frame(stream, {"protocol": 1, "type": "hello", "session": session, "token": token})
            self.assertEqual(host.read_frame(stream)["mode"], "simulator")
            for seq, frame in enumerate([self.collision(), {**self.collision(), "blocks": []}], 1):
                frame.update(protocol=1, session=session, seq=seq)
                host.write_frame(stream, frame)
                self.assertEqual(host.read_frame(stream)["seq"], seq)
        thread.join(3)
        self.assertFalse(thread.is_alive())
        self.assertFalse(errors)
        self.assertEqual(report["snapshots"], 2)
        self.assertEqual(report["last_collision"]["blocks"], [])

    def test_wrong_token_and_replayed_sequence_are_rejected(self):
        for valid_token in (False, True):
            client, peer = socket.socketpair()
            errors = []
            def serve():
                try:
                    with peer:
                        host.serve_peer(peer, "a" * 64, {})
                except host.ProtocolError as exc:
                    errors.append(str(exc))
            thread = threading.Thread(target=serve)
            thread.start()
            with client, client.makefile("rwb") as stream:
                session = str(uuid.uuid4())
                host.write_frame(stream, {"protocol": 1, "type": "hello", "session": session,
                                          "token": ("a" if valid_token else "b") * 64})
                if valid_token:
                    host.read_frame(stream)
                    frame = self.collision()
                    frame.update(protocol=1, session=session, seq=0)
                    host.write_frame(stream, frame)
                self.assertIsNone(host.read_frame(stream))
            thread.join(3)
            self.assertTrue(errors)
            self.assertNotIn("a" * 64, errors[0])

    def test_config_cannot_request_external_listener(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "config.json"
            path.write_text(json.dumps({"protocol": 1, "port": True, "token": "a" * 64}))
            with self.assertRaises(host.ProtocolError):
                host.load_config(path)


if __name__ == "__main__":
    unittest.main()

class TelemetryBudgetTests(unittest.TestCase):
    def test_console_summary_does_not_copy_meshes_actor_catalogs_or_tokens(self):
        mesh={'revision':1,'complete':True,'triangles':[[123456.789]*9 for _ in range(1024)]}
        state={'armed':True,'terrain':mesh,'combat_terrain':mesh,'combat_state':{'hits':12,'kills':4,'enemies':[{'position':[1,2,3]}]*128},'token':'a'*64}
        summary=host.control_summary(state)
        self.assertLess(len(json.dumps(summary)),600)
        self.assertEqual(summary['terrain']['triangles'],1024);self.assertEqual(summary['combat']['enemies'],128)
        self.assertNotIn('123456',str(summary));self.assertNotIn('token',summary)
        self.assertEqual(len(state['terrain']['triangles']),1024)

    def test_oversized_outbound_frame_is_rejected_before_any_partial_write(self):
        stream=io.BytesIO()
        with self.assertRaises(host.ProtocolError):host.write_frame(stream,{'payload':'x'*host.MAX_FRAME_BYTES})
        self.assertEqual(stream.getvalue(),b'')
