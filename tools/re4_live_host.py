#!/usr/bin/env python3
"""Authenticated loopback RE4 host; optional local native-input controller."""
import argparse
import os
from pathlib import Path
import socket
import struct
from fake_re4_host import ProtocolError, load_config, serve_peer
from re4_reader import DEFAULT_EXE, ReaderError, WindowsProcess, find_pid, locate_player_slot, sample_player
from re4_control import NativeControl, ControlPolicy
from re4_health import NativeHealth
from re4_runtime import NativeRuntime
from re4_combat import NativeCombat
from re4_graphics_capture import read_capture
from re4_block_preview import NativePreview
from re4_hud import NativeHud
from re4_overlay import NativeOverlay
from re4_terrain_feed import TerrainFeed,CombatTerrainFeed


def main():
    terrain=None;combat_terrain=None
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game-exe", default=DEFAULT_EXE)
    parser.add_argument("--game-dir", type=Path, default=Path(os.environ.get("APPDATA", ".")) / ".minecraft")
    parser.add_argument("--control", action="store_true", help="Require native module; explicit /re4bridge arm needed")
    args = parser.parse_args()
    process = None
    native = None
    preview = None
    hud = None
    health = None
    runtime = None
    combat = None
    try:
        config = load_config(args.game_dir / "config" / "re4bridge.json")
        pid = find_pid()
        process = WindowsProcess(pid, args.game_exe)
        slot = locate_player_slot(process.read, process.base, process.size)
        print("RE4 UHD 1.1.0 detectado. Lectura externa conectada.", flush=True)
        if args.control:
            terrain=TerrainFeed(process.read,process.base,process.size)
            combat_terrain=CombatTerrainFeed(process.read,process.base,process.size)
            native = NativeControl(pid)
            health = NativeHealth(pid,native.k)
            runtime = NativeRuntime(pid,native.k)
            combat = NativeCombat(pid,native.k)
            preview = NativePreview(pid,process.k)
            hud=NativeOverlay(pid,process.k)
            if not hud.available:hud.close();hud=NativeHud(pid,process.k)
            print('Host 0.4.51: Preparar Minecraft en VIEW y F9 para jugar. F8 detiene.',flush=True)
            try:
                captured=read_capture(process.k,pid)
                print(f"Captura grafica: {captured['hook_description']}; creaciones correctas={captured['device_success']}",flush=True)
                if 'installation' in captured:
                    journal=captured['installation']
                    print(f"Captura al inicio: {journal['initial']['reason']}; ultimo intento: {journal['latest']['reason']}; fase={journal['phase']}",flush=True)
            except ReaderError as error:print('Captura grafica:',error,flush=True)
            print("Modulo de controles nativo conectado. DESARMADO. F8 = parada de emergencia.", flush=True)
            capabilities = native.status()['capabilities']
            print(f"Capacidades: raton={bool(capabilities & 1)}, primera_persona={bool(capabilities & 2)}, correr={bool(capabilities & 4)}", flush=True)

        def provider():
            try:
                return sample_player(process.read, slot, process.base, process.size)
            except (ReaderError, struct.error):
                return {"state": "unavailable"}

        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
            server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server.bind(("127.0.0.1", config["port"]))
            server.listen(1)
            command = "controller" if args.control else "re4"
            print(f"READY 127.0.0.1:{config['port']} — RE4 REAL; "+("pulsa Preparar Minecraft en VIEW" if args.control else f"usa /re4bridge {command}"), flush=True)
            while True:
                peer, _ = server.accept()
                with peer:
                    policy = ControlPolicy(native,preview,hud,health,runtime,combat) if native is not None else None
                    try:
                        def apply(frame,sample):
                            result=policy.apply(frame,sample)
                            result.update(terrain.observe(frame,sample,result))
                            result.update(combat_terrain.observe(frame,sample,result))
                            return result
                        serve_peer(peer, config["token"], {}, provider, apply if policy else None)
                    except (ProtocolError, OSError,ReaderError) as error:
                        print("Sesion cerrada:", error, flush=True)
                    finally:
                        if policy:
                            policy.stop()
                            terrain.observe({'armed':False}, {'state':'unavailable'}, {'armed':False})
                            combat_terrain.observe({'armed':False}, {'state':'unavailable'}, {'armed':False})
    except KeyboardInterrupt:
        return 0
    except (OSError, ValueError, ProtocolError, ReaderError) as error:
        print("ERROR:", error, flush=True)
        print("Comprueba RE4 abierto, config de Minecraft y que el simulador/otro host este cerrado.")
        return 1
    finally:
        if combat is not None:combat.close()
        if runtime is not None:runtime.close()
        if health is not None:health.close()
        if terrain is not None:terrain.close()
        if combat_terrain is not None:combat_terrain.close()
        if hud is not None:hud.close()
        if preview is not None:
            preview.close()
        if native is not None:
            native.close()
        if process is not None:
            process.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
