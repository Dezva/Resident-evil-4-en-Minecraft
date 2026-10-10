#!/usr/bin/env python3
"""Read-only RE4 UHD 1.1.0 diagnostic. No injection, writes, or Minecraft socket.

Interop signatures/layout documented by nipkownix/re4_tweaks, revision
92c0208bd09c29c9640e13a2493b8e6bd6edd700 (Game.cpp, SDK/cCoord.h).
This is an independent external reader, not a re4_tweaks plugin.
"""
import argparse
import ctypes
from ctypes import wintypes as w
import json
import math
import os
import struct
import time

DEFAULT_EXE = r"D:\Games\Resident Evil 4 - Ultimate HD Edition\Bin32\bio4.exe"
VERSION_SIGNATURE = "68 ? ? ? ? 8B C8 E8 ? ? ? ? DC 2D ? ? ? ? A1 ? ? ? ? DC 25"
PLAYER_SIGNATURE = "A1 ? ? ? ? D8 CC D8 C9 D8 CA D9 5D ? D9 45 ?"


class ReaderError(Exception):
    pass


def u32(data, offset=0):
    return struct.unpack_from("<I", data, offset)[0]


def matches(data, signature):
    pattern = [None if p == "?" else int(p, 16) for p in signature.split()]
    found = []
    start = 0
    first = bytes([pattern[0]])
    while True:
        start = data.find(first, start)
        if start < 0 or start + len(pattern) > len(data):
            return found
        if all(v is None or data[start + i] == v for i, v in enumerate(pattern)):
            found.append(start)
        start += 1


def code_sections(read, base, module_size):
    """Validate loaded PE32/i386 headers and return executable section ranges."""
    if not 4096 <= module_size <= 128 * 1024 * 1024:
        raise ReaderError("Tamano de modulo no compatible")
    dos = read(base, 64)
    if dos[:2] != b"MZ":
        raise ReaderError("El modulo no es PE")
    pe_offset = u32(dos, 0x3C)
    if not 64 <= pe_offset <= min(module_size - 24, 1024 * 1024):
        raise ReaderError("Cabecera PE fuera de rango")
    header = read(base + pe_offset, 24)
    machine, count = struct.unpack_from("<HH", header, 4)
    optional_size = struct.unpack_from("<H", header, 20)[0]
    if header[:4] != b"PE\0\0" or machine != 0x14C or not 1 <= count <= 96:
        raise ReaderError("Se requiere el ejecutable RE4 x86 (PE32)")
    if not 96 <= optional_size <= 4096:
        raise ReaderError("Cabecera opcional no compatible")
    table_offset = pe_offset + 24 + optional_size
    if table_offset + count * 40 > module_size:
        raise ReaderError("Tabla de secciones fuera de rango")
    optional = read(base + pe_offset + 24, optional_size)
    if struct.unpack_from("<H", optional)[0] != 0x10B or u32(optional, 56) != module_size:
        raise ReaderError("Imagen PE32 no compatible")
    table = read(base + table_offset, count * 40)
    sections = []
    for i in range(count):
        row = table[i * 40:(i + 1) * 40]
        size, rva = struct.unpack_from("<II", row, 8)
        flags = u32(row, 36)
        if flags & 0x20000000 and size:
            if rva < table_offset + count * 40 or rva + size > module_size or size > 32 * 1024 * 1024:
                raise ReaderError("Seccion ejecutable fuera de rango")
            sections.append((base + rva, size))
    if not sections:
        raise ReaderError("No hay secciones ejecutables")
    return sections


def locate_player_slot(read, base, size):
    sections = [(address, read(address, length)) for address, length in code_sections(read, base, size)]

    def operand(signature):
        hits = [(address + offset, data, offset) for address, data in sections
                for offset in matches(data, signature)]
        if len(hits) != 1:
            raise ReaderError(f"Firma incompatible o ambigua ({len(hits)} coincidencias). No se usaran direcciones alternativas.")
        _, data, offset = hits[0]
        pointer = u32(data, offset + 1)
        if not base <= pointer <= base + size - 6:
            raise ReaderError("Operando de firma fuera del modulo")
        return pointer

    version = read(operand(VERSION_SIGNATURE), 6)
    if version != b"1.1.0\0":
        raise ReaderError("Version no compatible: se requiere RE4 UHD 1.1.0")
    return operand(PLAYER_SIGNATURE)


def sample_player(read, slot, base, module_size):
    pointer = u32(read(slot, 4))
    if pointer == 0:
        return {"state": "waiting_player"}
    if pointer < 0x10000 or pointer > 0xFFFFFF00 or pointer % 4:
        raise ReaderError("Puntero de personaje no valido")
    vtable = u32(read(pointer, 4))
    if not base <= vtable < base + module_size:
        raise ReaderError("Objeto de personaje no compatible")
    # cCoord::pos_94 and ang_A0: two contiguous Vec (float x,y,z).
    values = struct.unpack("<6f", read(pointer + 0x94, 24))
    if u32(read(slot, 4)) != pointer:
        return {"state": "transition"}
    if not all(math.isfinite(v) for v in values):
        raise ReaderError("Posicion/rotacion no finita")
    return {"state": "player", "position_raw": list(values[:3]), "angles_raw": list(values[3:])}


class WindowsProcess:
    """Only PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION are requested."""
    def __init__(self, pid, expected_exe):
        if os.name != "nt":
            raise ReaderError("Este lector debe ejecutarse en Windows junto a RE4")
        self.k = ctypes.WinDLL("kernel32", use_last_error=True)
        self._bind()
        self.handle = self.k.OpenProcess(0x0010 | 0x1000, False, pid)
        if not self.handle:
            raise ReaderError(f"OpenProcess fallo (Windows {ctypes.get_last_error()})")
        try:
            path = ctypes.create_unicode_buffer(32768)
            count = w.DWORD(len(path))
            if not self.k.QueryFullProcessImageNameW(self.handle, 0, path, ctypes.byref(count)):
                raise ReaderError("No se pudo verificar la ruta del proceso")
            if os.path.normcase(os.path.abspath(path.value)) != os.path.normcase(os.path.abspath(expected_exe)):
                raise ReaderError("El proceso no pertenece a la ruta RE4 solicitada")
            self.base, self.size = self._module(pid)
        except Exception:
            self.close()
            raise

    def _bind(self):
        signatures = {
            "OpenProcess": ([w.DWORD, w.BOOL, w.DWORD], w.HANDLE),
            "CloseHandle": ([w.HANDLE], w.BOOL),
            "ReadProcessMemory": ([w.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t,
                                   ctypes.POINTER(ctypes.c_size_t)], w.BOOL),
            "QueryFullProcessImageNameW": ([w.HANDLE, w.DWORD, w.LPWSTR, ctypes.POINTER(w.DWORD)], w.BOOL),
            "CreateToolhelp32Snapshot": ([w.DWORD, w.DWORD], w.HANDLE),
            "Module32FirstW": ([w.HANDLE, ctypes.c_void_p], w.BOOL),
        }
        for name, (args, result) in signatures.items():
            fn = getattr(self.k, name)
            fn.argtypes, fn.restype = args, result

    def _module(self, pid):
        class Module(ctypes.Structure):
            _fields_ = [("dwSize", w.DWORD), ("th32ModuleID", w.DWORD), ("th32ProcessID", w.DWORD),
                        ("GlblcntUsage", w.DWORD), ("ProccntUsage", w.DWORD),
                        ("modBaseAddr", ctypes.c_void_p), ("modBaseSize", w.DWORD), ("hModule", w.HMODULE),
                        ("szModule", w.WCHAR * 256), ("szExePath", w.WCHAR * 260)]
        # A 64-bit Python can enumerate a 32-bit game with SNAPMODULE32.
        snapshot = self.k.CreateToolhelp32Snapshot(0x08 | 0x10, pid)
        if snapshot == ctypes.c_void_p(-1).value:
            raise ReaderError(f"No se pudieron enumerar modulos (Windows {ctypes.get_last_error()})")
        try:
            module = Module()
            module.dwSize = ctypes.sizeof(module)
            if not self.k.Module32FirstW(snapshot, ctypes.byref(module)) or module.szModule.lower() != "bio4.exe":
                raise ReaderError("No se encontro bio4.exe como modulo principal")
            return module.modBaseAddr, module.modBaseSize
        finally:
            self.k.CloseHandle(snapshot)

    def read(self, address, size):
        if not 0 < size <= 32 * 1024 * 1024:
            raise ReaderError("Lectura fuera de rango")
        buffer = ctypes.create_string_buffer(size)
        copied = ctypes.c_size_t()
        if not self.k.ReadProcessMemory(self.handle, address, buffer, size, ctypes.byref(copied)) or copied.value != size:
            raise ReaderError(f"Memoria no disponible (Windows {ctypes.get_last_error()}); posible cambio de escena o cierre del juego")
        return buffer.raw

    def close(self):
        if self.handle:
            self.k.CloseHandle(self.handle)
            self.handle = None


def find_pid():
    if os.name != "nt":
        raise ReaderError("Este lector debe ejecutarse en Windows junto a RE4")
    k = ctypes.WinDLL("kernel32", use_last_error=True)
    class Process(ctypes.Structure):
        _fields_ = [("dwSize", w.DWORD), ("cntUsage", w.DWORD), ("th32ProcessID", w.DWORD),
                    ("th32DefaultHeapID", ctypes.c_size_t), ("th32ModuleID", w.DWORD),
                    ("cntThreads", w.DWORD), ("th32ParentProcessID", w.DWORD),
                    ("pcPriClassBase", w.LONG), ("dwFlags", w.DWORD), ("szExeFile", w.WCHAR * 260)]
    k.CreateToolhelp32Snapshot.argtypes = [w.DWORD, w.DWORD]
    k.CreateToolhelp32Snapshot.restype = w.HANDLE
    k.CloseHandle.argtypes = [w.HANDLE]
    k.CloseHandle.restype = w.BOOL
    for name in ("Process32FirstW", "Process32NextW"):
        getattr(k, name).argtypes = [w.HANDLE, ctypes.POINTER(Process)]
        getattr(k, name).restype = w.BOOL
    snapshot = k.CreateToolhelp32Snapshot(2, 0)
    if snapshot == ctypes.c_void_p(-1).value:
        raise ReaderError("No se pudieron enumerar procesos")
    try:
        entry = Process()
        entry.dwSize = ctypes.sizeof(entry)
        found = []
        valid = k.Process32FirstW(snapshot, ctypes.byref(entry))
        while valid:
            if entry.szExeFile.lower() == "bio4.exe":
                found.append(entry.th32ProcessID)
            valid = k.Process32NextW(snapshot, ctypes.byref(entry))
        if len(found) != 1:
            raise ReaderError(f"Se necesita exactamente un bio4.exe abierto; encontrados: {len(found)}")
        return found[0]
    finally:
        k.CloseHandle(snapshot)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game-exe", default=DEFAULT_EXE)
    parser.add_argument("--pid", type=int, help="PID explicito; igualmente se comprueba la ruta")
    parser.add_argument("--once", action="store_true", help="Una sola muestra")
    args = parser.parse_args()
    process = None
    try:
        process = WindowsProcess(args.pid if args.pid is not None else find_pid(), args.game_exe)
        slot = locate_player_slot(process.read, process.base, process.size)
        print("RE4 UHD 1.1.0 detectado — LECTOR DE POSICION, sin conexion a Minecraft", flush=True)
        failures = 0
        while True:
            try:
                sample = sample_player(process.read, slot, process.base, process.size)
                failures = 0
            except ReaderError as error:
                failures += 1
                if args.once or failures >= 10:
                    raise
                sample = {"state": "unavailable", "message": str(error)}
            sample["monotonic_ns"] = time.monotonic_ns()
            print(json.dumps(sample, allow_nan=False), flush=True)
            if args.once:
                break
            time.sleep(0.25)
    except KeyboardInterrupt:
        return 0
    except (ReaderError, OSError, struct.error) as error:
        print("ERROR:", error, flush=True)
        return 1
    finally:
        if process is not None:
            process.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
