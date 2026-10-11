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


class ReadOnlyIPC:
    def __init__(self, pid):
        self.k = ctypes.WinDLL('kernel32', use_last_error=True)
        for name, args, result in [('OpenFileMappingW',[w.DWORD,w.BOOL,w.LPCWSTR],w.HANDLE),
                                   ('MapViewOfFile',[w.HANDLE,w.DWORD,w.DWORD,w.DWORD,ctypes.c_size_t],ctypes.c_void_p),
                                   ('UnmapViewOfFile',[ctypes.c_void_p],w.BOOL),('CloseHandle',[w.HANDLE],w.BOOL)]:
            fn=getattr(self.k,name); fn.argtypes=args; fn.restype=result
        self.handle=self.k.OpenFileMappingW(4,False,f'Local\\RE4Bridge-Control-{pid}')
        if not self.handle: raise ReaderError(f"IPC de lectura no disponible (Windows {ctypes.get_last_error()}).")
        self.view=0
        try:
            self.length=40
            self.view=self.k.MapViewOfFile(self.handle,4,0,0,self.length)
            if not self.view: raise ReaderError("No se pudo leer IPC.")
            header=ctypes.string_at(self.view,8)
            if u32(header)!=0x52453443 or u32(header,4) not in (1,2):
                raise ReaderError("IPC desconocido.")
            if u32(header,4)==2:
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
        return decode_ipc(data)

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
    process=ipc=None
    try:
        pid=find_pid(); process=WindowsProcess(pid,args.exe)
        slot=locate_player_slot(process.read,process.base,process.size)
        addresses=camera_addresses(process.read,process.base,process.size)
        try: ipc=ReadOnlyIPC(pid); ipc_error=None
        except ReaderError as error: ipc_error=str(error)
        with output.open('x',encoding='utf-8') as log:
            header=metadata(args.exe); header['ipc_error']=ipc_error
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
                log.write(json.dumps(row,allow_nan=False)+'\n'); log.flush()
                time.sleep(0.2)
            print('Registro terminado:',output)
    except (ReaderError,OSError,ValueError) as error:
        print('ERROR:',error); return 1
    finally:
        if ipc: ipc.close()
        if process: process.close()
    return 0


if __name__=='__main__': raise SystemExit(main())
