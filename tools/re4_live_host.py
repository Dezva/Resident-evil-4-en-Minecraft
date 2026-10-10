#!/usr/bin/env python3
"""Authenticated loopback RE4 read-only host. Never writes game memory."""
import argparse
import os
from pathlib import Path
import socket
import struct
from fake_re4_host import ProtocolError, load_config, serve_peer
from re4_reader import DEFAULT_EXE, ReaderError, WindowsProcess, find_pid, locate_player_slot, sample_player


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game-exe", default=DEFAULT_EXE)
    parser.add_argument("--game-dir", type=Path, default=Path(os.environ.get("APPDATA", ".")) / ".minecraft")
    args = parser.parse_args()
    process = None
    try:
        config = load_config(args.game_dir / "config" / "re4bridge.json")
        process = WindowsProcess(find_pid(), args.game_exe)
        slot = locate_player_slot(process.read, process.base, process.size)
        print("RE4 UHD 1.1.0 detectado. No se escribe memoria del juego.", flush=True)

        def provider():
            try:
                return sample_player(process.read, slot, process.base, process.size)
            except (ReaderError, struct.error):
                return {"state": "unavailable"}

        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
            server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server.bind(("127.0.0.1", config["port"]))
            server.listen(1)
            print(f"READY 127.0.0.1:{config['port']} — LECTOR RE4 REAL; usa /re4bridge re4", flush=True)
            while True:
                peer, _ = server.accept()
                with peer:
                    try:
                        serve_peer(peer, config["token"], {}, provider)
                    except (ProtocolError, OSError) as error:
                        print("Sesion cerrada:", error, flush=True)
    except KeyboardInterrupt:
        return 0
    except (OSError, ValueError, ProtocolError, ReaderError) as error:
        print("ERROR:", error, flush=True)
        print("Comprueba RE4 abierto, config de Minecraft y que el simulador/otro host este cerrado.")
        return 1
    finally:
        if process is not None:
            process.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
