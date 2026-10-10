#!/usr/bin/env python3
"""A diagnostic peer, NOT a RE4 loader. Uses only Python's standard library."""
from __future__ import annotations

import argparse
import hmac
import json
import math
from pathlib import Path
import socket
import time
import uuid

PROTOCOL = 1
MAX_FRAME_BYTES = 524288


class ProtocolError(Exception):
    pass


def reject_constant(value):
    raise ProtocolError("Non-finite JSON number")


def read_frame(stream):
    data = stream.readline(MAX_FRAME_BYTES + 2)
    if not data:
        return None
    if not data.endswith(b"\n") or len(data) - 1 > MAX_FRAME_BYTES:
        raise ProtocolError("Oversized or incomplete frame")
    try:
        frame = json.loads(data.decode("utf-8"), parse_constant=reject_constant)
    except (ValueError, UnicodeError) as exc:
        raise ProtocolError("Invalid JSON") from exc
    if not isinstance(frame, dict) or type(frame.get("protocol")) is not int or frame["protocol"] != PROTOCOL:
        raise ProtocolError("Incompatible protocol")
    return frame


def write_frame(stream, frame):
    stream.write((json.dumps(frame, allow_nan=False, separators=(",", ":")) + "\n").encode())
    stream.flush()


def number(value):
    return type(value) in (int, float) and math.isfinite(value) and abs(value) <= 100_000_000


def vector(value, size=3):
    return isinstance(value, list) and len(value) == size and all(number(v) for v in value)


def validate_sample(frame):
    if type(frame.get("sample_ns")) is not int or frame["sample_ns"] < 0:
        raise ProtocolError("Missing sample time")
    if not isinstance(frame.get("dimension"), str) or len(frame["dimension"]) > 128:
        raise ProtocolError("Invalid dimension")
    if frame["type"] == "pose":
        if not vector(frame.get("feet")) or not vector(frame.get("eye")):
            raise ProtocolError("Invalid position")
        if not number(frame.get("yaw_deg")) or not number(frame.get("pitch_deg")):
            raise ProtocolError("Invalid orientation")
        return
    if frame["type"] != "collision_snapshot":
        raise ProtocolError("Unknown sample type")
    if not vector(frame.get("center")) or any(type(v) is not int and v != int(v) for v in frame["center"]):
        raise ProtocolError("Invalid sample center")
    if type(frame.get("complete")) is not bool:
        raise ProtocolError("Missing completeness flag")
    blocks = frame.get("blocks")
    if not isinstance(blocks, list) or len(blocks) > 405:
        raise ProtocolError("Invalid region size")
    seen = set()
    for block in blocks:
        if not isinstance(block, dict) or not vector(block.get("pos")):
            raise ProtocolError("Invalid block")
        pos = block["pos"]
        if any(v != int(v) for v in pos) or tuple(pos) in seen:
            raise ProtocolError("Duplicate or fractional block position")
        seen.add(tuple(pos))
        if any(abs(pos[i] - frame["center"][i]) > (2 if i == 1 else 4) for i in range(3)):
            raise ProtocolError("Block outside sampled region")
        if not isinstance(block.get("state"), str) or len(block["state"]) > 1024:
            raise ProtocolError("Invalid block state")
        if type(block.get("collision_complete")) is not bool:
            raise ProtocolError("Missing shape completeness")
        boxes = block.get("boxes")
        if not isinstance(boxes, list) or len(boxes) > 256:
            raise ProtocolError("Invalid collision boxes")
        for box in boxes:
            if not vector(box, 6) or any(abs(v) > 16 for v in box):
                raise ProtocolError("Invalid box bounds")
            if any(box[i] >= box[i + 3] for i in range(3)):
                raise ProtocolError("Empty or inverted box")
        if not block["collision_complete"] and boxes:
            raise ProtocolError("Incomplete shapes must not pretend to be complete")
    if frame["complete"] and any(not b["collision_complete"] for b in blocks):
        raise ProtocolError("Contradictory completeness")


def serve_peer(peer, token, report):
    # Timeout prevents an unauthenticated or dead peer occupying the only session indefinitely.
    peer.settimeout(3.0)
    with peer.makefile("rwb") as stream:
        hello = read_frame(stream)
        if not hello or hello.get("type") != "hello" or not isinstance(hello.get("token"), str):
            raise ProtocolError("Missing handshake")
        if not hmac.compare_digest(hello["token"], token):
            raise ProtocolError("Authentication failed")
        try:
            session = str(uuid.UUID(hello["session"]))
        except (KeyError, ValueError, TypeError, AttributeError) as exc:
            raise ProtocolError("Invalid session") from exc
        if session != hello["session"]:
            raise ProtocolError("Noncanonical session")
        write_frame(stream, {"protocol": 1, "type": "welcome", "session": session, "mode": "simulator"})
        sequence = 0
        report.clear()
        report.update(poses=0, snapshots=0, last_sequence=0, mode="simulator", disconnected=False)
        print("Conectado a Minecraft. SIMULADOR: RE4 no está conectado.", flush=True)
        last_print = 0.0
        while True:
            try:
                frame = read_frame(stream)
            except TimeoutError:
                raise ProtocolError("Peer stale; reconnect")
            if frame is None:
                report["disconnected"] = True
                return
            if frame.get("session") != session or type(frame.get("seq")) is not int or frame["seq"] != sequence + 1:
                raise ProtocolError("Stale session or nonmonotonic sequence")
            if frame.get("type") != "heartbeat":
                validate_sample(frame)
            sequence = frame["seq"]
            report["last_sequence"] = sequence
            if frame["type"] == "pose":
                report["poses"] += 1
                report["last_pose"] = {k: frame[k] for k in ("feet", "eye", "yaw_deg", "pitch_deg", "dimension")}
            elif frame["type"] == "collision_snapshot":
                report["snapshots"] += 1
                # Replace the entire sampled region. Omitted/removed blocks do not persist.
                report["last_collision"] = {k: frame[k] for k in ("center", "complete", "blocks", "dimension")}
            write_frame(stream, {"protocol": 1, "type": "ack", "session": session, "seq": sequence})
            if time.monotonic() - last_print > 1:
                last_print = time.monotonic()
                collision = report.get("last_collision", {})
                print(f"seq={sequence} poses={report['poses']} snapshots={report['snapshots']} "
                      f"feet={report.get('last_pose', {}).get('feet')} "
                      f"blocks={len(collision.get('blocks', []))} complete={collision.get('complete')}", flush=True)


def load_config(path):
    if path.stat().st_size > 4096:
        raise ProtocolError("Config too large")
    config = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(config, dict) or type(config.get("protocol")) is not int or config["protocol"] != 1:
        raise ProtocolError("Invalid config protocol")
    if type(config.get("port")) is not int or not 1024 <= config["port"] <= 65535:
        raise ProtocolError("Invalid local port")
    token = config.get("token")
    if not isinstance(token, str) or len(token) != 64 or any(c not in "0123456789abcdef" for c in token):
        raise ProtocolError("Invalid config token")
    return config


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--game-dir", type=Path, help="Minecraft instance folder containing config/re4bridge.json")
    source.add_argument("--config", type=Path, help="Explicit config file, never a token argument")
    parser.add_argument("--once", action="store_true", help="Stop after one connection (automated tests)")
    parser.add_argument("--report", type=Path, help="Write final diagnostic JSON without the auth token")
    args = parser.parse_args()
    config_path = args.config or args.game_dir / "config" / "re4bridge.json"
    try:
        config = load_config(config_path)
    except (OSError, ValueError, ProtocolError):
        parser.exit(2, "No se pudo leer la configuración. Inicia Minecraft con el mod y revisa --game-dir.\n")
    report = {"mode": "simulator"}
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
            server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server.bind(("127.0.0.1", config["port"]))
            server.listen(1)
            print(f"READY 127.0.0.1:{config['port']} — SIMULADOR, no RE4", flush=True)
            while True:
                peer, _ = server.accept()
                with peer:
                    try:
                        serve_peer(peer, config["token"], report)
                    except (ProtocolError, OSError) as exc:
                        report["error"] = str(exc)
                        print(f"Sesión cerrada: {exc}", flush=True)
                if args.once:
                    break
    except KeyboardInterrupt:
        pass
    except OSError:
        parser.exit(2, "No se pudo abrir el puerto local. Cierra otro simulador o revisa la configuración.\n")
    finally:
        if args.report:
            args.report.write_text(json.dumps(report, allow_nan=False, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
