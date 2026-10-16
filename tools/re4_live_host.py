#!/usr/bin/env python3
"""Authenticated loopback RE4 host; optional local native-input controller."""
import argparse
import logging
from logging.handlers import RotatingFileHandler
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
from re4_world_mesh import NativeWorldMesh
from re4_combat_probe import describe_profile
from re4_graphics_capture import read_capture
from re4_block_preview import NativePreview
from re4_hud import NativeHud
from re4_overlay import NativeOverlay
from re4_terrain_feed import TerrainFeed
from re4_console import ConsoleReporter
from fake_re4_host import control_summary


def setup_logging():
    folder=Path(__file__).resolve().parents[1]/'logs'
    folder.mkdir(exist_ok=True)
    log=logging.getLogger('re4bridge.host');log.setLevel(logging.INFO)
    handler=RotatingFileHandler(folder/'re4bridge-host.log',maxBytes=2*1024*1024,backupCount=2,encoding='utf-8')
    handler.setFormatter(logging.Formatter('%(asctime)s %(levelname)s %(message)s'))
    log.addHandler(handler)
    return log,handler


def main():
    terrain=None
    log,log_handler=setup_logging()
    log.info('Host 0.4.57 iniciado')
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game-exe", default=DEFAULT_EXE)
    parser.add_argument("--game-dir", type=Path, default=Path(os.environ.get("APPDATA", ".")) / ".minecraft")
    parser.add_argument("--control", action="store_true", help="Require native module; explicit /re4bridge arm needed")
    args = parser.parse_args()
    console=ConsoleReporter()
    say=console.print
    process = None
    native = None
    preview = None
    hud = None
    health = None
    runtime = None
    combat = None
    world_mesh = None
    try:
        config = load_config(args.game_dir / "config" / "re4bridge.json")
        pid = find_pid()
        process = WindowsProcess(pid, args.game_exe)
        slot = locate_player_slot(process.read, process.base, process.size)
        say("RE4 UHD 1.1.0 detectado. Lectura externa conectada.", flush=True)
        if args.control:
            terrain=TerrainFeed(process.read,process.base,process.size)
            native = NativeControl(pid)
            health = NativeHealth(pid,native.k)
            runtime = NativeRuntime(pid,native.k)
            combat = NativeCombat(pid,native.k)
            world_mesh = NativeWorldMesh(native.k,pid)
            engine=combat.status()
            log.info('Perfil de reacciones nativas: %s',engine)
            if not engine.get('engine_supported'):log.warning('Perfil del ejecutable (solo lectura): %s',describe_profile(process.read,process.base,process.size))
            say('Reacciones nativas: '+('listas' if engine.get('engine_supported') else 'perfil no reconocido; revisa logs/re4bridge-host.log'),flush=True)
            preview = NativePreview(pid,process.k)
            hud=NativeOverlay(pid,process.k)
            if not hud.available:hud.close();hud=NativeHud(pid,process.k)
            say('Host 0.4.57: Preparar Minecraft en VIEW y F9 para jugar. F8 detiene.',flush=True)
            try:
                captured=read_capture(process.k,pid)
                say(f"Captura grafica: {captured['hook_description']}; creaciones correctas={captured['device_success']}",flush=True)
                if 'installation' in captured:
                    journal=captured['installation']
                    say(f"Captura al inicio: {journal['initial']['reason']}; ultimo intento: {journal['latest']['reason']}; fase={journal['phase']}",flush=True)
            except ReaderError as error:say('Captura grafica:',error,flush=True)
            say("Modulo de controles nativo conectado. DESARMADO. F8 = parada de emergencia.", flush=True)
            capabilities = native.status()['capabilities']
            say(f"Capacidades: raton={bool(capabilities & 1)}, primera_persona={bool(capabilities & 2)}, correr={bool(capabilities & 4)}", flush=True)

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
            say(f"READY 127.0.0.1:{config['port']} — RE4 REAL; "+("pulsa Preparar Minecraft en VIEW" if args.control else f"usa /re4bridge {command}"), flush=True)
            while True:
                peer, _ = server.accept()
                with peer:
                    policy = ControlPolicy(native,preview,hud,health,runtime,combat,world_mesh) if native is not None else None
                    try:
                        last_state=None
                        def apply(frame,sample):
                            nonlocal last_state
                            result=policy.apply(frame,sample)
                            result.update(terrain.observe(frame,sample,result))
                            result['combat_terrain_state']='consulta nativa por impacto'
                            client=result.get('client',{})
                            activity=result.get('runtime_state',{});impact=result.get('combat_state',{})
                            key=(frame.get('arm_id'),result['armed'],result.get('reason'),result.get('stop'),client.get('reason'),client.get('focused'),client.get('screen'),activity.get('scene'),bool(activity.get('native_flags2',0)&0x40000000),impact.get('ack'),impact.get('reason'))
                            if key!=last_state:
                                log.info('Cambio de control: %s',control_summary(result));last_state=key
                            return result
                        serve_peer(peer, config["token"], {}, provider, apply if policy else None, console.emit)
                    except (ProtocolError, OSError,ReaderError) as error:
                        log.warning('Sesion interrumpida (%s): %s',type(error).__name__,error)
                        say("Sesion cerrada:", error, flush=True)
                    except Exception:
                        log.exception('Fallo inesperado de sesion; se desarma y se permite reconectar')
                        say('Sesion interrumpida. Detalles en logs/re4bridge-host.log; puedes volver a preparar Minecraft.',flush=True)
                    finally:
                        if policy:
                            policy.stop()
                            terrain.observe({'armed':False}, {'state':'unavailable'}, {'armed':False})
    except KeyboardInterrupt:
        return 0
    except (OSError, ValueError, ProtocolError, ReaderError) as error:
        log.exception('Host detenido')
        say("ERROR:", error, flush=True)
        say("Comprueba RE4 abierto, config de Minecraft y que el simulador/otro host este cerrado.")
        return 1
    except Exception:
        log.exception('Host detenido por un fallo inesperado')
        say('ERROR: revisa logs/re4bridge-host.log.',flush=True)
        return 1
    finally:
        if world_mesh is not None:world_mesh.close()
        if combat is not None:combat.close()
        if runtime is not None:runtime.close()
        if health is not None:health.close()
        if terrain is not None:terrain.close()
        if hud is not None:hud.close()
        if preview is not None:
            preview.close()
        if native is not None:
            native.close()
        if process is not None:
            process.close()
        log.info('Host terminado')
        log.removeHandler(log_handler);log_handler.close()
        console.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
