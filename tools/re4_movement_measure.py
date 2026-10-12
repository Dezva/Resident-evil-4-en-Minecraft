"""Timed, read-only movement measurement. Never injects input or writes game/IPC memory."""
import argparse
import ctypes
import datetime
import json
import math
from pathlib import Path
import struct
import time

from re4_reader import DEFAULT_EXE, ReaderError, WindowsProcess, code_sections, find_pid, locate_player_slot, matches, sample_player, u32
from re4_native_probe import KEY_SIGNATURE
from re4_camera_trace import ReadOnlyIPC
from re4_cube_test import read_collision_status


def key_address(read, base, size):
    hits=[]
    for address,length in code_sections(read,base,size):
        hits.extend(address+i for i in matches(read(address,length),KEY_SIGNATURE))
    if len(hits)!=1:raise ReaderError('Firma de teclas ausente/ambigua; no se usara otra direccion.')
    pointer=u32(read(hits[0]+1,4))
    if pointer%4 or not base<=pointer<=base+size-8:raise ReaderError('Teclas fuera del modulo.')
    read(pointer,8)
    return pointer


def summarize(rows, units_per_block=1000, sprint=False):
    reasons=[]
    valid=[r for r in rows if r['player'].get('state')=='player']
    if len(valid)<2 or len(valid)!=len(rows):return dict(valid=False,reasons=['Jugador ausente o cambio de zona durante la prueba.'])
    duration=valid[-1]['time']-valid[0]['time']
    if duration<=0:return dict(valid=False,reasons=['Tiempo de captura insuficiente.'])
    distance=0.0;moving_time=0.0
    for a,b in zip(valid,valid[1:]):
        dt=b['time']-a['time']
        if not 0<dt<=.25:reasons.append('Captura interrumpida.');continue
        pa,pb=a['player']['position_raw'],b['player']['position_raw']
        step=math.hypot(pb[0]-pa[0],pb[2]-pa[2])
        if not math.isfinite(step) or step>500:reasons.append('Salto de posicion/cambio de zona.');continue
        distance+=step
        if step>2:moving_time+=dt
    forward=sum(bool(r['keys']&1) for r in valid)/len(valid)
    running=sum(bool(r['keys']&64) for r in valid)/len(valid)
    if forward<.8:reasons.append('W no estuvo activo durante suficiente tiempo.')
    if sprint and running<.8:reasons.append('El motor no recibio correr durante suficiente tiempo.')
    if not sprint and running>.2:reasons.append('Correr estuvo activo durante la prueba de caminar.')
    if moving_time/duration<.7:reasons.append('Demasiado tiempo sin avanzar: comprueba obstaculos o controles.')
    modes={r['control']['armed'] for r in valid if r.get('control') and 'armed' in r['control']}
    if len(modes)>1:reasons.append('El puente cambio entre armado y desarmado.')
    ids={r['control']['arm_id'] for r in valid if r.get('control') and 'arm_id' in r['control']}
    if True in modes and len(ids)>1:reasons.append('Cambio de generacion de control.')
    barriers=[r.get('collision') for r in valid if r.get('collision') is not None]
    if barriers and barriers[-1]['contacts']!=barriers[0]['contacts']:reasons.append('Hubo contactos con bloques del puente.')
    controls=[r.get('control') for r in valid if r.get('control') and 'hook_ticks' in r['control']]
    rate=None
    if len(controls)>1:rate=((controls[-1]['hook_ticks']-controls[0]['hook_ticks'])&0xffffffff)/duration
    speed=distance/duration
    return dict(valid=not reasons,reasons=list(dict.fromkeys(reasons)),seconds=duration,distance_native=distance,
                units_per_second=speed,blocks_per_second=speed/units_per_block,
                moving_units_per_second=distance/moving_time if moving_time else 0,
                forward_fraction=forward,run_fraction=running,bridge_armed=True in modes,
                pad_callbacks_per_second=rate,
                note='PadRead por segundo es cadencia de input, no una medicion de FPS de GPU.')


def capture_window(snapshot, seconds, clock=time.perf_counter, sleep=time.sleep):
    start=clock();rows=[]
    while True:
        current=clock();row=snapshot();row['time']=current-start;rows.append(row)
        if current-start>=seconds:return rows
        sleep(min(.05,seconds-(current-start)))


def wait_start(user):
    print('Vuelve a la ventana del juego. Pulsa F10 para iniciar; no abras el chat durante la captura.',flush=True)
    deadline=time.monotonic()+180
    # Require release before a new edge, so a held F10 cannot start both phases.
    previous=bool(user.GetAsyncKeyState(0x79)&0x8000)
    while time.monotonic()<deadline:
        pressed=bool(user.GetAsyncKeyState(0x79)&0x8000)
        if pressed and not previous:return
        previous=pressed;time.sleep(.05)
    raise ReaderError('No se pulso F10 durante tres minutos.')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-exe',default=DEFAULT_EXE)
    parser.add_argument('--seconds',type=float,default=4)
    parser.add_argument('--units-per-block',type=float,default=1000)
    args=parser.parse_args()
    if not 2<=args.seconds<=10 or not 40<=args.units_per_block<=2500:parser.error('Tiempo2..10s; escala40..2500.')
    process=ipc=None
    try:
        import winsound
        def tone(frequency,duration):
            try:winsound.Beep(frequency,duration)
            except RuntimeError:print('Tono no disponible; usa las marcas INICIO/FIN de consola.',flush=True)
        pid=find_pid();process=WindowsProcess(pid,args.game_exe)
        slot=locate_player_slot(process.read,process.base,process.size)
        key=key_address(process.read,process.base,process.size)
        try:ipc=ReadOnlyIPC(pid)
        except ReaderError:ipc=None
        user=ctypes.WinDLL('user32',use_last_error=True)
        user.GetAsyncKeyState.argtypes=[ctypes.c_int];user.GetAsyncKeyState.restype=ctypes.c_short
        def snapshot():
            row=dict(player=sample_player(process.read,slot,process.base,process.size),keys=struct.unpack('<Q',process.read(key,8))[0])
            if ipc is not None:row['control']=ipc.sample()
            row['collision']=read_collision_status(process.k,pid)
            return row
        report=dict(mode='READ ONLY MOVEMENT',units_per_block=args.units_per_block,phases=[],
                    generated_utc=datetime.datetime.now(datetime.timezone.utc).isoformat())
        print('Medicion de movimiento, sin enviar teclas ni modificar RE4. Elige una zona despejada.',flush=True)
        for name,sprint in [('caminar',False),('correr',True)]:
            print(f'Prueba {name}: al tono largo mantén W'+(' y tu tecla de correr/sprint.' if sprint else ' sin correr.'),flush=True)
            print(f'Mantenlas {args.seconds:g}s hasta el tono de fin. Si usas el puente, preparalo y armalo antes de pulsar F10.',flush=True)
            wait_start(user)
            for count in (3,2,1):
                print(count,flush=True);tone(700,100);time.sleep(.9)
            tone(1100,150);print('INICIO',flush=True)
            rows=capture_window(snapshot,args.seconds)
            print('FIN: suelta las teclas.',flush=True);tone(400,300)
            result=summarize(rows,args.units_per_block,sprint)
            report['phases'].append(dict(name=name,summary=result,samples=rows))
            print(json.dumps(result,ensure_ascii=False,indent=2),flush=True)
        output=Path(__file__).resolve().parents[1]/('movement-measure-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S')+'.json')
        output.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
        print('Informe:',output,flush=True)
        return 0
    except (OSError,ReaderError,ValueError,struct.error) as error:
        print('ERROR:',error,flush=True);return 1
    except KeyboardInterrupt:return 0
    finally:
        if ipc is not None:ipc.close()
        if process is not None:process.close()


if __name__=='__main__':raise SystemExit(main())
