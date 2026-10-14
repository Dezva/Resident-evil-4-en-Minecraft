"""Read-only audit of native hook prerequisites; never installs or patches a hook."""
import argparse
import ctypes
import json
import struct
import time
from pathlib import Path
from re4_reader import (DEFAULT_EXE, VERSION_SIGNATURE, ReaderError, WindowsProcess,
                        code_sections, find_pid, matches, u32)

KEY_SIGNATURE = "A1 ? ? ? ? 33 C9 83 E0 ? 83 3D"
PAD_SIGNATURE = "E8 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? 6A 04 E8 ? ? ? ? 83 C4 18 E8 ? ? ? ? E8 ? ? ? ? A1"
ENEMY_SIGNATURE = "81 E1 01 02 00 00 83 F9 01 75 ? 50 B9 ? ? ? ? E8"
TRIGGER_SIGNATURE = "A1 ? ? ? ? 25 ? ? ? ? 0B C1 74 ? 88 0D"
CAMERA_SIGNATURE = "52 8D 85 ? ? ? ? 50 8D 4D ? 51 8B CB E8"
SMOOTH_SIGNATURE = "E8 ? ? ? ? 80 BB B6 01 00 00 00"
CAMERA_CTRL_SIGNATURE = "D9 EE B9 ? ? ? ? D9 9E A4 00 00 00 E8 ? ? ? ? D9 EE"
GLOBAL_SIGNATURE = "A1 ? ? ? ? B9 FF FF FF 7F 21 48 ? A1"
PROJECTION_SIGNATURE = "E8 ? ? ? ? 8A 46 30 3C FD"
OBSERVED_PROJECTION_CALLS = {0x1e7110,0x1e8697,0x1e8f4a,0x1ebb47,0x1f43d3,0x1f7bad,0x205dad}


def inspect_enemy_manager(read, base, size, address):
    row=dict(manager_rva=hex(address-base))
    try:
        vt,array,count,stride=struct.unpack('<4I',read(address,16))
        row.update(vtable_in_module=base<=vt<base+size,capacity=count,stride=stride,
                   array_aligned=array>=0x10000 and array%4==0)
        row['cem_layout_possible']=(row['vtable_in_module'] and row['array_aligned'] and 0<count<=4096
            and 0x408<=stride<=65536 and stride%4==0 and count*stride<=64*1024*1024 and array+count*stride<=0xffffffff)
        if row['cem_layout_possible']:
            ids={};routines={};flags={};eligible=0;scanned=0
            for index in range(count):
                data=read(array+index*stride,0x408);scanned+=1
                actor_vt=u32(data);bits=u32(data,4)
                if not base<=actor_vt<base+size or not bits&0x601:continue
                kind=data[0x100];routine=data[0xfc];hp=struct.unpack_from('<h',data,0x324)[0]
                ids[hex(kind)]=ids.get(hex(kind),0)+1;routines[str(routine)]=routines.get(str(routine),0)+1
                flags[hex(bits&0x601)]=flags.get(hex(bits&0x601),0)+1
                if bits&0x601==1 and routine in (1,2) and hp>0:eligible+=1
            row.update(scanned=scanned,ids=ids,routines=routines,validity_flags=flags,live_cem_fields=eligible)
    except (ReaderError,OSError,ValueError,struct.error) as error:
        row['read_error']=str(error)
    return row


def sample_native_input(read, base, pointers, seconds, clock=time.monotonic, wait=time.sleep):
    if not 0<seconds<=20:raise ValueError('Input capture duration must be between 0 and 20 seconds')
    start=clock();rows=[];previous=None
    while clock()-start<seconds:
        values=tuple(struct.unpack('<Q',read(base+int(pointers[key],16),8))[0] for key in ('on_rva','trigger_rva'))
        if values!=previous and len(rows)<256:
            rows.append(dict(elapsed_ms=round((clock()-start)*1000),native_on=hex(values[0]),native_trigger=hex(values[1])))
            previous=values
        wait(.005)
    return rows


def observed_projection_profile(rows):
    """Exact read-only UHD layout, not a majority or arbitrary fallback."""
    if len(rows)!=7 or {int(row['call_rva'],16) for row in rows}!=OBSERVED_PROJECTION_CALLS:
        return False
    for row in rows:
        other = int(row['call_rva'],16)==0x1f7bad
        if row['target_rva']!=hex(0x344f if other else 0xe395): return False
        if row.get('resolved_target_rva')!=hex(0x202f60 if other else 0x19f3a0): return False
        if row.get('resolved_target_executable') is False or row.get('target_executable') is False: return False
    return True


def inspect(read, base, size, query=None):
    report = {"mode": "READ ONLY", "checks": {}, "sections": [],
              "note": "Con DLL activa, las ramas ya instaladas pueden apuntar a dinput8 fuera de bio4; no implica fallo."}
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
    projection_calls = []
    for name, signature in [("version", VERSION_SIGNATURE), ("keys", KEY_SIGNATURE), ("pad", PAD_SIGNATURE),
                            ("camera", CAMERA_SIGNATURE), ("camera_ctrl", CAMERA_CTRL_SIGNATURE), ("camera_smoothing", SMOOTH_SIGNATURE),
                            ("global", GLOBAL_SIGNATURE), ("projection", PROJECTION_SIGNATURE),
                            ("enemy_manager", ENEMY_SIGNATURE), ("action_trigger", TRIGGER_SIGNATURE)]:
        found = [address + hit for address, data in blobs for hit in matches(data, signature)]
        checks[name + "_matches"] = len(found)
        if name == 'enemy_manager':
            pointers = [u32(read(site+13,4)) for site in found]
            checks['enemy_manager_unique_targets'] = len(set(pointers))
            checks['enemy_manager_shared_target'] = 0 < len(found) <= 64 and len(set(pointers)) == 1
            checks['enemy_manager_in_module'] = bool(pointers) and all(base <= p <= base+size-32 and p % 4 == 0 for p in pointers)
            if checks['enemy_manager_in_module'] and len(found)<=64:
                report['enemy_manager_candidates']=[inspect_enemy_manager(read,base,size,p) for p in sorted(set(pointers))]
            if checks['enemy_manager_shared_target'] and checks['enemy_manager_in_module']:
                vt,array,capacity,stride=struct.unpack('<4I',read(pointers[0],16))
                report['enemy_manager'] = dict(manager_rva=hex(pointers[0]-base),vtable_in_module=base<=vt<base+size,
                    capacity=capacity,stride=stride,array_aligned=array>=0x10000 and array%4==0)
        if name == 'projection': projection_calls = found
        if len(found) == 1:
            hits[name] = found[0]
    if 'global' in hits:
        pointer = u32(read(hits['global']+1,4))
        checks['global_slot_in_module'] = base <= pointer <= base+size-4 and pointer % 4 == 0
    if 'keys' in hits and 'action_trigger' in hits:
        on=u32(read(hits['keys']+1,4));trigger=u32(read(hits['action_trigger']+1,4))
        if all(base<=p<=base+size-8 and p%4==0 for p in (on,trigger)) and on!=trigger:
            report['input_pointers']=dict(on_rva=hex(on-base),trigger_rva=hex(trigger-base))
    if projection_calls:
        rows = []
        targets = []
        for call in projection_calls:
            target = call+5+struct.unpack('<i',read(call+1,4))[0]
            targets.append(target)
            inside = base <= target < base+size
            row = {'call_rva':hex(call-base),'target_rva':hex(target-base), 'target_in_module':inside}
            if inside:
                row['target_bytes'] = read(target,min(16,base+size-target)).hex(' ')
                if query: row['target_executable'] = bool(query(target)['protection'] & (0x20|0x40|0x80))
                if target <= base+size-5 and read(target,1)==b'\xe9':
                    resolved = target+5+struct.unpack('<i',read(target+1,4))[0]
                    if base <= resolved < base+size:
                        row['resolved_target_rva'] = hex(resolved-base)
                        row['resolved_target_bytes'] = read(resolved,min(16,base+size-resolved)).hex(' ')
                        if query: row['resolved_target_executable'] = bool(query(resolved)['protection'] & (0x20|0x40|0x80))
            rows.append(row)
        report['projection_calls'] = rows
        checks['projection_unique_targets'] = len(set(targets))
        checks['projection_target_in_module'] = all(row['target_in_module'] for row in rows)
        checks['projection_shared_target'] = len(set(targets)) == 1 and checks['projection_target_in_module'] and len(rows) <= 32
        checks['projection_observed_profile'] = observed_projection_profile(rows)
        if query and checks['projection_target_in_module']:
            checks['projection_target_executable'] = all(row['target_executable'] for row in rows)
    if 'camera_ctrl' in hits:
        pointer = u32(read(hits['camera_ctrl'] + 3, 4))
        checks['camera_ctrl_in_module'] = base <= pointer <= base + size - 0x788
        if checks['camera_ctrl_in_module']:
            read(pointer, 0x788)
            checks['camera_ctrl_readable'] = True
    if 'camera' in hits:
        call = hits['camera'] + 14
        wrapper = call + 5 + struct.unpack('<i', read(call+1,4))[0]
        checks['camera_wrapper_in_module'] = base <= wrapper <= base + size - 5
        if checks['camera_wrapper_in_module']:
            data = read(wrapper,5)
            report['camera_branch_details'] = {'wrapper_rva':hex(wrapper-base), 'wrapper_bytes':data.hex(' ')}
            checks['camera_wrapper_branch'] = data[0] in (0xe8,0xe9)
            if checks['camera_wrapper_branch']:
                target = wrapper + 5 + struct.unpack_from('<i',data,1)[0]
                checks['camera_target_in_module'] = base <= target < base + size
                if checks['camera_target_in_module'] and query:
                    checks['camera_target_executable'] = bool(query(target)['protection'] & (0x20|0x40|0x80))
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
            report["branch_details"] = {
                "call_rva": hex(call - base),
                "wrapper_rva": hex(wrapper - base),
                "wrapper_bytes": read(wrapper, min(24, base + size - wrapper)).hex(" "),
            }
            checks["wrapper_starts_call"] = data[0] == 0xe8
            checks["wrapper_starts_jump"] = data[0] == 0xe9
            if checks["wrapper_starts_call"] or checks["wrapper_starts_jump"]:
                target = wrapper + 5 + struct.unpack_from("<i", data, 1)[0]
                checks["target_in_module"] = base <= target < base + size
                if checks["target_in_module"]:
                    read(target, 1)
                    report["branch_details"]["target_rva"] = hex(target - base)
                    report["branch_details"]["target_bytes"] = read(target, min(24, base + size - target)).hex(" ")
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
    parser.add_argument("--input-seconds",type=float,default=0)
    parser.add_argument("--report",type=Path)
    args = parser.parse_args()
    process = None
    query_handle = None
    try:
        pid = find_pid()
        process = WindowsProcess(pid, args.exe)
        query_handle, query = memory_query(process, pid)
        report=inspect(process.read, process.base, process.size, query)
        if args.input_seconds:
            if 'input_pointers' not in report:raise ReaderError('No se identificaron ambos canales nativos de entrada')
            print('Pulsa F8 para detener el puente; enfoca RE4 y pulsa E una vez ante una accion. Lectura de botones en 3 segundos.',flush=True)
            time.sleep(3)
            report['native_input_samples']=sample_native_input(process.read,process.base,report['input_pointers'],args.input_seconds)
        if args.report:args.report.write_text(json.dumps(report,indent=2),encoding='utf-8')
        print(json.dumps(report, indent=2))
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
