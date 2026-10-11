"""Read-only camera/character observations; never publishes control or modifies RE4."""
import argparse
import ctypes
from ctypes import wintypes as w
import datetime
import hashlib
import json
import math
from pathlib import Path
import struct
import time

from re4_reader import (DEFAULT_EXE, ReaderError, WindowsProcess, code_sections,
                        find_pid, locate_player_slot, matches, sample_player, u32)
from re4_native_probe import CAMERA_CTRL_SIGNATURE, GLOBAL_SIGNATURE

SMOOTH_CTRL_SIGNATURE = "56 B9 ? ? ? ? E8 ? ? ? ? 8D 53 60"
KNOWN_DLLS = {
    "a4f36adf075e66d4e2796417ffc35dd8c6822f8ad72e800fedbf2d5d35352af0": "0.4.18 (explicit cube test)",
    "f431b22d52cf04eb7da4ef72bda81c0b2da16f59946bef1a34f1cc3021857f20": "0.4.17 (system paths capture, drawing disabled)",
    "2e6f7c21d30ed62e09eff9e6e5b17cd5418e2b59b87cfd6739fb3a834a16cfb8": "0.4.16 (creation retry, drawing disabled)",
    "0a79c56b94aa83ddcdc40e78998956c43b27cc8f63eb81fc3fa9a6032e8b3052": "0.4.13 (withdrawn)",
    "a88a40e7ab0df1aaf126a1614943f9cf50b2f6083ed59a0f63ff600026a56ace": "0.4.14 (graphics disabled)",
    "fa2773e941e0fc92717f493c34a2575fa3cd8514420f897959050fab6ea2f492": "0.4.15 (creation capture, drawing disabled)",
    "9258ba9fc395273e12a6e5412e7e68fcfb1f6ee5bc25c0a36f74080ba421400d": "0.4.12",
    "45b675b3f2f50d0ee46225b72dcec607343f1c12a13e3b22e5d75bbc6ac2e8a9": "0.4.11",
    "29696fd38b7085582b1647eaaf3f0c5945b566534de98296d89549c2811279cd": "0.4.10",
    "1e578b891f83811a36d9624077554e2d7ae67d96647ef03ca00a4073d76752c8": "0.4.9",
    "4e929e2dcd6bd0034d1ee101eb75840aeb732a28710b55f02ba6967484b2a0ed": "0.4.8",
    "69118533f1d2b21f5f7bd9d3c5f6154e81b97aa2b1ab871a362a1b77416015e5": "0.4.7",
    "fd58c15d645215a6ba841a84b3422ab9967e881ca44291bdfc9f3652a21af48b": "0.4.6",
    "2e8165188db6a22f4d03f4e2ca0297d1e2346187956952de98e214a9939f995d": "0.4.5",
    "a01d582a0548b052a7e77c5a61b9ba342b4941132a1a2c07eb3f18d44cd8f8db": "0.4.4",
    "a66e6c7c01adb212b30f8918593a7d9b0cdc85ca37ba22e0f65648eddb0e7012": "0.4.0",
    "1b4d42141885c6ef7fbdf3312fcd8ea861cab199fddf7bd3718e3e8c542eba96": "0.4.1",
    "fa452d6fad8c8ddc931c51b76325124920c2c16aef11db2a6c1c7713dded35f2": "0.4.2",
    "c4e277dd9a4b5524972478232d1bbba48347ad6d46a58daae80c88de30dd7070": "0.4.3",
}


def camera_addresses(read, base, size):
    blobs = [(address, read(address, length)) for address, length in code_sections(read, base, size)]
    result = {}
    for name, signature, offset, length in [("ctrl", CAMERA_CTRL_SIGNATURE, 3, 0x788),
                                            ("smooth", SMOOTH_CTRL_SIGNATURE, 2, 0x150)]:
        hits = [address + index for address, data in blobs for index in matches(data, signature)]
        if len(hits) != 1:
            if name == "ctrl": raise ReaderError(f"Firma de camara ambigua/ausente ({len(hits)}).")
            continue
        pointer = u32(read(hits[0] + offset, 4))
        if not base <= pointer <= base + size - length:
            raise ReaderError("Camara fuera del modulo; no se usara una direccion alternativa.")
        read(pointer, length)
        result[name] = pointer
    hits = [address + index for address,data in blobs for index in matches(data,GLOBAL_SIGNATURE)]
    if len(hits) == 1:
        pointer = u32(read(hits[0]+1,4))
        if base <= pointer <= base+size-4 and pointer % 4 == 0:
            result['global_slot'] = pointer
    return result


def floats(read, address, count):
    values = struct.unpack('<' + 'f' * count, read(address, count * 4))
    if not all(math.isfinite(value) for value in values):
        raise ReaderError("Datos de camara no finitos.")
    return list(values)


def point(read, address):
    data = floats(read, address, 8)
    direction = [data[i+3] - data[i] for i in range(3)]
    length = math.sqrt(sum(value*value for value in direction))
    yaw = math.atan2(direction[0], direction[2]) if length > 1e-6 else None
    return {"position": data[:3], "target": data[3:6], "roll": data[6], "fovy": data[7],
            "yaw_from_points": yaw}


def observation(read, base, size, slot, addresses):
    player = sample_player(read, slot, base, size)
    ctrl = addresses['ctrl']
    row = {"player": player, "ctrl_point": point(read, ctrl+0x38),
           "ctrl_camera_point": point(read, ctrl+0x60+0xA4),
           "ctrl_camera_matrix": floats(read, ctrl+0x60, 12),
           "ctrl_camera_view_matrix": floats(read, ctrl+0x60+0x30, 12),
           "ctrl_camera_look": floats(read, ctrl+0x60+0xD0, 3),
           "quasi_point": point(read, ctrl+0x278+0xA4),
           "quasi_matrix": floats(read, ctrl+0x278+0x178, 12),
           "quasi_player_direction": floats(read, ctrl+0x278+0x16C, 3),
           "quasi_ratios": floats(read, ctrl+0x278+0x204, 3)}
    if 'smooth' in addresses:
        row['smooth_point'] = point(read, addresses['smooth']+0xA4)
        row['smooth_ratio'] = floats(read, addresses['smooth']+0xFC, 1)[0]
    if 'global_slot' in addresses:
        global_pointer = u32(read(addresses['global_slot'],4))
        if global_pointer >= 0x10000 and global_pointer <= 0xFFFFFE00 and global_pointer % 4 == 0:
            camera = global_pointer+0x74
            row['global_camera_point'] = point(read,camera+0xA4)
            row['global_camera_view_matrix'] = floats(read,camera+0x30,12)
            row['global_camera_look'] = floats(read,camera+0xD0,3)
    return row


def decode_ipc(data):
    if len(data) < 40: raise ReaderError("IPC demasiado corto.")
    words = struct.unpack_from('<10I', data)
    if words[0] != 0x52453443 or words[1] not in (1,2): raise ReaderError("IPC desconocido.")
    result = dict(zip(['sequence','stamp','armed','buttons','stop','status','hook_ticks','native_buttons'], words[2:]))
    if words[1] == 2:
        if len(data) < 80: raise ReaderError("IPC v2 demasiado corto.")
        more = struct.unpack_from('<10I', data, 40)
        total, pitch, eye = struct.unpack_from('<3f', data, 40)
        yaw = struct.unpack_from('<f', data, 64)[0]
        if not all(math.isfinite(value) for value in (total,pitch,eye,yaw)):
            raise ReaderError("IPC de camara no finito.")
        result.update(total_yaw=total,pitch=pitch,eye=eye,look_flags=more[3],arm_id=more[4],
                      capabilities=more[5],applied_yaw=yaw,camera_ticks=more[7],
                      look_packets=more[8],camera_active=bool(more[9]))
    return result


def decode_diagnostics(data):
    if len(data) != 64:
        raise ReaderError('Diagnostico de proyeccion incompleto.')
    words = struct.unpack_from('<12I',data)
    if words[0] != 0x52453444 or words[1] != 1 or words[2] & 1:
        raise ReaderError('Diagnostico de proyeccion desconocido/incompleto.')
    angles = struct.unpack_from('<4f',data,48)
    if not all(math.isfinite(value) for value in angles):
        raise ReaderError('Diagnostico de proyeccion no finito.')
    result = dict(zip(['sequence','build','pid','calls','writes','rejected','stamp',
                       'pad_ticks','camera_ticks','applied'],words[2:]))
    result.update(zip(['yaw_before','yaw_written','yaw_after','view_yaw'],angles))
    return result


class ReadOnlyIPC:
    def __init__(self, pid, diagnostic=False):
        self.diagnostic=diagnostic
        self.k = ctypes.WinDLL('kernel32', use_last_error=True)
        for name, args, result in [('OpenFileMappingW',[w.DWORD,w.BOOL,w.LPCWSTR],w.HANDLE),
                                   ('MapViewOfFile',[w.HANDLE,w.DWORD,w.DWORD,w.DWORD,ctypes.c_size_t],ctypes.c_void_p),
                                   ('UnmapViewOfFile',[ctypes.c_void_p],w.BOOL),('CloseHandle',[w.HANDLE],w.BOOL)]:
            fn=getattr(self.k,name); fn.argtypes=args; fn.restype=result
        mapping='CameraDiagnostics' if diagnostic else 'Control'
        self.handle=self.k.OpenFileMappingW(4,False,f'Local\\RE4Bridge-{mapping}-{pid}')
        if not self.handle: raise ReaderError(f"IPC de lectura no disponible (Windows {ctypes.get_last_error()}).")
        self.view=0
        try:
            self.length=64 if diagnostic else 40
            self.view=self.k.MapViewOfFile(self.handle,4,0,0,self.length)
            if not self.view: raise ReaderError("No se pudo leer IPC.")
            header=ctypes.string_at(self.view,8)
            if diagnostic:
                if u32(header)!=0x52453444 or u32(header,4)!=1:
                    raise ReaderError('Diagnostico desconocido.')
            elif u32(header)!=0x52453443 or u32(header,4) not in (1,2):
                raise ReaderError("IPC desconocido.")
            if not diagnostic and u32(header,4)==2:
                self.k.UnmapViewOfFile(self.view); self.view=0; self.length=80
                self.view=self.k.MapViewOfFile(self.handle,4,0,0,self.length)
                if not self.view: raise ReaderError("No se pudo leer IPC v2.")
            self.sample()
        except Exception:
            self.close(); raise

    def sample(self):
        # Host seqlock covers control fields; native counters update independently.
        before=u32(ctypes.string_at(self.view+8,4))
        data=ctypes.string_at(self.view,self.length)
        after=u32(ctypes.string_at(self.view+8,4))
        if before!=after or before & 1 or u32(data,8)!=before:
            return {"state":"torn_sample"}
        return decode_diagnostics(data) if self.diagnostic else decode_ipc(data)

    def close(self):
        if self.view: self.k.UnmapViewOfFile(self.view); self.view=0
        if self.handle: self.k.CloseHandle(self.handle); self.handle=0


def metadata(exe):
    dll=Path(exe).parent/'dinput8.dll'
    digest=hashlib.sha256(dll.read_bytes()).hexdigest() if dll.is_file() else None
    return {"type":"header", "mode":"READ ONLY", "dll_on_disk_sha256":digest,
            "dll_on_disk_version":KNOWN_DLLS.get(digest,'unknown'),
            "note":"El hash identifica el archivo instalado, no prueba que se reinicio RE4 ni que el frame muestreado sea atomico."}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe',default=DEFAULT_EXE)
    parser.add_argument('--seconds',type=int,default=45,choices=range(5,121),metavar='5..120')
    parser.add_argument('--output',type=Path)
    args=parser.parse_args()
    output=args.output or Path(__file__).resolve().parents[1]/('camera-trace-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S')+'.jsonl')
    process=ipc=diagnostic=None
    try:
        pid=find_pid(); process=WindowsProcess(pid,args.exe)
        slot=locate_player_slot(process.read,process.base,process.size)
        addresses=camera_addresses(process.read,process.base,process.size)
        try: ipc=ReadOnlyIPC(pid); ipc_error=None
        except ReaderError as error: ipc_error=str(error)
        try: diagnostic=ReadOnlyIPC(pid,diagnostic=True); diagnostic_error=None
        except ReaderError as error: diagnostic_error=str(error)
        with output.open('x',encoding='utf-8') as log:
            header=metadata(args.exe); header['ipc_error']=ipc_error
            header['projection_diagnostic_error']=diagnostic_error
            if diagnostic: header['loaded_diagnostic']=diagnostic.sample()
            log.write(json.dumps(header,allow_nan=False)+'\n'); log.flush()
            print('LECTURA SOLAMENTE. No se modifica RE4 ni se activa/desactiva el puente.')
            print('Vuelve a Minecraft, arm, camina y luego suelta W. Registro de',args.seconds,'segundos.')
            print('DLL en disco:',header['dll_on_disk_version'])
            print('Archivo:',output)
            start=time.monotonic()
            while time.monotonic()-start<args.seconds:
                row=observation(process.read,process.base,process.size,slot,addresses)
                row['elapsed']=round(time.monotonic()-start,3)
                if ipc: row['ipc']=ipc.sample()
                if diagnostic: row['projection_diagnostic']=diagnostic.sample()
                log.write(json.dumps(row,allow_nan=False)+'\n'); log.flush()
                elapsed=int(time.monotonic()-start)
                print(f'REGISTRANDO {elapsed:02d}/{args.seconds:02d} s | quedan {max(0,args.seconds-elapsed):02d} s',end='\r',flush=True)
                time.sleep(0.2)
            print()
            print('Registro terminado:',output)
    except (ReaderError,OSError,ValueError) as error:
        print('ERROR:',error); return 1
    finally:
        if ipc: ipc.close()
        if diagnostic: diagnostic.close()
        if process: process.close()
    return 0


if __name__=='__main__': raise SystemExit(main())
