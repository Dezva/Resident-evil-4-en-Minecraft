import socket
import threading
import unittest
import uuid
from fake_re4_host import ProtocolError, read_frame, serve_peer, write_frame


class LiveProtocolTests(unittest.TestCase):
    def test_reader_samples_follow_poll_and_reject_outbound_world_state(self):
        client, peer = socket.socketpair()
        errors, calls = [], []
        def provider():
            calls.append(1)
            return {"state": "player", "position_raw": [1, 2, 3], "angles_raw": [0, 0.5, 0]}
        def run():
            try:
                with peer:
                    serve_peer(peer, "a" * 64, {}, provider)
            except ProtocolError as error:
                errors.append(str(error))
        thread = threading.Thread(target=run)
        thread.start()
        session = str(uuid.uuid4())
        with client, client.makefile("rwb") as stream:
            write_frame(stream, {"protocol": 1, "type": "hello", "session": session, "token": "a" * 64})
            self.assertEqual(read_frame(stream)["mode"], "re4_reader")
            write_frame(stream, {"protocol": 1, "type": "re4_poll", "session": session, "seq": 1})
            ack = read_frame(stream)
            self.assertEqual(ack["re4_sample"]["position_raw"], [1, 2, 3])
            self.assertEqual(ack["seq"], 1)
            write_frame(stream, {"protocol": 1, "type": "pose", "session": session, "seq": 2})
            self.assertIsNone(read_frame(stream))
        thread.join(3)
        self.assertFalse(thread.is_alive())
        self.assertEqual(len(calls), 1)
        self.assertEqual(errors, ["Reader accepts only re4_poll"])


if __name__ == "__main__":
    unittest.main()
