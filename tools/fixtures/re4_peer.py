"""Synthetic RE4 peer for cross-language tests ONLY; never included in Windows ZIP."""
import argparse
from pathlib import Path
import socket
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from fake_re4_host import load_config, serve_peer

parser = argparse.ArgumentParser()
parser.add_argument("--config", type=Path, required=True)
args = parser.parse_args()
config = load_config(args.config)
count = 0


def sample():
    global count
    count += 1
    if count >= 3:
        return {"state": "waiting_player"}
    return {"state": "player", "position_raw": [100, -200, 300], "angles_raw": [0, 1.5, 0]}


with socket.socket() as server:
    server.bind(("127.0.0.1", config["port"]))
    server.listen(1)
    print("READY synthetic fixture", flush=True)
    peer, _ = server.accept()
    with peer:
        serve_peer(peer, config["token"], {}, sample)
