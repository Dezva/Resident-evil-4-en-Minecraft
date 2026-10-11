"""Read-only audit of native hook prerequisites; never installs or patches a hook."""
import argparse
import ctypes
import json
import struct
from re4_reader import (DEFAULT_EXE, VERSION_SIGNATURE, ReaderError, WindowsProcess,
                        code_sections, find_pid, matches, u32)

KEY_SIGNATURE = "A1 ? ? ? ? 33 C9 83 E0 ? 83 3D"
PAD_SIGNATURE = "E8 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? 6A 04 E8 ? ? ? ? 83 C4 18 E8 ? ? ? ? E8 ? ? ? ? A1"


def inspect(read, base, size, query=None):
    report = {"mode": "READ ONLY", "checks": {}, "sections": []}
    checks = report["checks"]
    offset = u32(read(base, 64), 0x3c)
    header = read(base + offset, 24)
    checks["native_optional_header_224"] = struct.unpack_from("<H", header, 20)[0] == 224
    sections = code_sections(read, base, size)
    blobs = []
    for address, length in sections:
        row = {"bytes": length}
        if query:
            region = query(address)
            row.update(region)
            row["fits_single_region"] = length <= region["remaining"]
        report["sections"].append(row)
        blobs.append((address, read(address, length)))
    hits = {}
    for name, signature in [("version", VERSION_SIGNATURE), ("keys", KEY_SIGNATURE), ("pad", PAD_SIGNATURE)]:
        found = [address + hit for address, data in blobs for hit in matches(data, signature)]
        checks[name + "_matches"] = len(found)
        if len(found) == 1:
            hits[name] = found[0]
    if "version" in hits:
        pointer = u32(read(hits["version"] + 1, 4))
        checks["version_pointer_in_module"] = base <= pointer <= base + size - 6
        if checks["version_pointer_in_module"]:
            checks["version_1_1_0"] = read(pointer, 6) == b"1.1.0\0"
    if "keys" in hits:
        pointer = u32(read(hits["keys"] + 1, 4))
        checks["keys_pointer_in_module"] = base <= pointer <= base + size - 8
        if checks["keys_pointer_in_module"]:
            read(pointer, 8)
            checks["keys_readable"] = True
            if query:
                info = query(pointer)
                checks["keys_writable"] = bool(info["protection"] & (4 | 0x40))
    if "pad" in hits:
        call = hits["pad"]
        wrapper = call + 5 + struct.unpack("<i", read(call + 1, 4))[0]
        checks["wrapper_in_module"] = base <= wrapper <= base + size - 5
        if checks["wrapper_in_module"]:
            data = read(wrapper, 5)
            checks["wrapper_starts_call"] = data[0] == 0xe8
            if checks["wrapper_starts_call"]:
                target = wrapper + 5 + struct.unpack_from("<i", data, 1)[0]
                checks["target_in_module"] = base <= target < base + size
                if checks["target_in_module"]:
                    read(target, 1)
                    if query:
                        checks["target_executable"] = bool(query(target)["protection"] & (0x20 | 0x40 | 0x80))
    return report


def memory_query(process, pid):
    class MemoryInfo(ctypes.Structure):
        _fields_ = [("base", ctypes.c_void_p), ("allocation", ctypes.c_void_p),
                    ("allocation_protect", ctypes.c_uint32)]
        if ctypes.sizeof(ctypes.c_void_p) == 8:
            _fields_ += [("partition", ctypes.c_uint16)]
        _fields_ += [("size", ctypes.c_size_t), ("state", ctypes.c_uint32),
                    ("protect", ctypes.c_uint32), ("kind", ctypes.c_uint32)]
    k = process.k
    # VirtualQueryEx requires QUERY_INFORMATION, beyond the normal reader handle.
    handle = k.OpenProcess(0x410, False, pid)
    if not handle:
        raise ReaderError(f"OpenProcess de diagnostico fallo (Windows {ctypes.get_last_error()})")
    k.VirtualQueryEx.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.POINTER(MemoryInfo), ctypes.c_size_t]
    k.VirtualQueryEx.restype = ctypes.c_size_t

    def query(address):
        info = MemoryInfo()
        if not k.VirtualQueryEx(handle, address, ctypes.byref(info), ctypes.sizeof(info)):
            raise ReaderError(f"VirtualQueryEx fallo (Windows {ctypes.get_last_error()})")
        return {"remaining": info.base + info.size - address, "protection": info.protect,
                "committed": info.state == 0x1000}
    return handle, query


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", default=DEFAULT_EXE)
    args = parser.parse_args()
    process = None
    query_handle = None
    try:
        pid = find_pid()
        process = WindowsProcess(pid, args.exe)
        query_handle, query = memory_query(process, pid)
        print(json.dumps(inspect(process.read, process.base, process.size, query), indent=2))
        print("Diagnostico terminado. No se modifico RE4 ni se activaron controles.")
    except (ReaderError, OSError, ValueError) as error:
        print("ERROR:", error)
        return 1
    finally:
        if query_handle:
            process.k.CloseHandle(query_handle)
        if process:
            process.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
