"""External, read-only graphics audit. Never calls COM or writes in bio4.exe.
Reports possible method tables, not proof of a live IDirect3DDevice9 object.
"""
import argparse
import ctypes
from ctypes import wintypes as w
import datetime
import json
from pathlib import Path
import struct
import time
from re4_reader import DEFAULT_EXE,ReaderError,WindowsProcess,find_pid,code_sections,locate_player_slot,matches,u32

FACTORY_SIGNATURE='E8 ? ? ? ? A3 ? ? ? ? 85 C0 75 17'
GRAPHICS_NAMES={'d3d9.dll','d3d11.dll','dxgi.dll','vulkan-1.dll','opengl32.dll','dinput8.dll'}


def is_graphics(name):
    name=name.lower()
    return name in GRAPHICS_NAMES or name.startswith(('nv','amd','ati','igd','igc','reshade','dxvk'))


def loaded_modules(process,pid):
    class Module(ctypes.Structure):
        _fields_=[('size',w.DWORD),('id',w.DWORD),('pid',w.DWORD),('global_count',w.DWORD),('count',w.DWORD),
                  ('base',ctypes.c_void_p),('length',w.DWORD),('module',w.HMODULE),('name',w.WCHAR*256),('path',w.WCHAR*260)]
    k=process.k
    k.Module32NextW.argtypes=[w.HANDLE,ctypes.c_void_p];k.Module32NextW.restype=w.BOOL
    snapshot=k.CreateToolhelp32Snapshot(0x18,pid)
    if snapshot==ctypes.c_void_p(-1).value:raise ReaderError('No se pudieron enumerar bibliotecas graficas.')
    try:
        row=Module();row.size=ctypes.sizeof(row);result=[]
        valid=k.Module32FirstW(snapshot,ctypes.byref(row))
        if not valid:raise ReaderError('No se pudo leer la lista de modulos.')
        while valid:
            result.append(dict(name=row.name.lower(),base=row.base,size=row.length,path=row.path))
            if len(result)>2048:raise ReaderError('Lista de modulos demasiado grande.')
            valid=k.Module32NextW(snapshot,ctypes.byref(row))
        if ctypes.get_last_error()!=18:raise ReaderError('La enumeracion de modulos no termino correctamente.')
        return result
    finally:k.CloseHandle(snapshot)


def query_memory(process,pid):
    class Memory(ctypes.Structure):
        _fields_=[('base',ctypes.c_void_p),('allocation',ctypes.c_void_p),('allocation_protect',w.DWORD)]
        if ctypes.sizeof(ctypes.c_void_p)==8:_fields_+=[('partition',ctypes.c_uint16)]
        _fields_+=[('size',ctypes.c_size_t),('state',w.DWORD),('protect',w.DWORD),('kind',w.DWORD)]
    k=process.k;handle=k.OpenProcess(0x410,False,pid)
    if not handle:raise ReaderError(f'OpenProcess de diagnostico fallo (Windows {ctypes.get_last_error()}).')
    k.VirtualQueryEx.argtypes=[w.HANDLE,ctypes.c_void_p,ctypes.POINTER(Memory),ctypes.c_size_t]
    k.VirtualQueryEx.restype=ctypes.c_size_t
    def query(address):
        m=Memory()
        if not k.VirtualQueryEx(handle,address,ctypes.byref(m),ctypes.sizeof(m)):
            raise ReaderError('Region de memoria no disponible.')
        return dict(base=m.base or 0,end=(m.base or 0)+m.size,protect=m.protect,kind=m.kind,committed=m.state==0x1000)
    return handle,query


def data_sections(read,base,size):
    code_sections(read,base,size) # Reuse complete PE32/header/range validation.
    pe=base+u32(read(base,64),0x3c);header=read(pe,24)
    count=struct.unpack_from('<H',header,6)[0];optional=struct.unpack_from('<H',header,20)[0]
    rows=read(pe+24+optional,count*40);result=[]
    for i in range(count):
        row=rows[i*40:(i+1)*40];length,rva=struct.unpack_from('<II',row,8);flags=u32(row,36)
        if flags&0x80000000 and not flags&0x20000000 and length:
            if rva>=size or length>size-rva:raise ReaderError('Seccion de datos fuera de rango.')
            result.append((base+rva,length))
    return result


class Audit:
    def __init__(self,read,query,modules,seconds=20,root_limit=4096,byte_limit=32*1024*1024):
        self.read=read;self.query=query;self.modules=modules;self.deadline=time.monotonic()+seconds
        self.root_limit=root_limit;self.byte_limit=byte_limit;self.bytes=0;self.memory_cache={};self.objects=set()
        self.roots=set();self.samples=[];self.table_info={};self.truncated=False
        self.counts=dict(object_pointers=0,readable_tables=0,graphics_tables=0,method_table_candidates=0,read_failures=0)

    def region(self,address):
        page=address&~4095
        if page not in self.memory_cache:self.memory_cache[page]=self.query(address)
        return self.memory_cache[page]

    def owner(self,address):
        return next((m for m in self.modules if m['base']<=address<m['base']+m['size']),None)

    def location(self,address):
        owner=self.owner(address)
        return {'module':owner['name'] if owner else None,'rva':hex(address-owner['base']) if owner else None}

    def readable(self,address,length,writable=False):
        if not 0x10000<=address<=0xffffffff-length:return False
        m=self.region(address);p=m['protect']
        if not m['committed'] or p&0x101 or address+length>m['end']:return False
        return (p&255) in ((4,8) if writable else (2,4,8,0x20,0x40,0x80))

    def executable(self,address):
        try:
            m=self.region(address)
            return bool(m['committed'] and not m['protect']&0x101 and m['protect']&0xf0)
        except ReaderError:return False

    def fetch(self,address,length):
        if time.monotonic()>=self.deadline or self.bytes+length>self.byte_limit:
            self.truncated=True;raise TimeoutError('Limite del diagnostico alcanzado.')
        data=self.read(address,length);self.bytes+=length;return data

    def describe(self,table):
        if table in self.table_info:return self.table_info[table]
        info=None;self.table_info[table]=None
        if table%4 or not self.readable(table,119*4):return None
        self.counts['readable_tables']+=1
        owner=self.owner(table)
        if owner and owner['name']!='bio4.exe' and not is_graphics(owner['name']):return None
        methods=struct.unpack('<119I',self.fetch(table,119*4))
        # Diagnostic filter only. It does NOT validate an interface for hooking.
        if not all(self.executable(methods[i]) for i in (0,1,2,6,7,9,42)):return None
        graphics=sum(bool(self.owner(m) and is_graphics(self.owner(m)['name'])) for m in methods)
        if graphics<7:return None
        self.counts['graphics_tables']+=1
        executable=sum(self.executable(m) for m in methods)
        if executable<100:return None
        self.counts['method_table_candidates']+=1
        named={'get_direct3d':6,'get_device_caps':7,'get_creation_parameters':9,'reset':16,'present':17,'end_scene':42}
        info={'table_address':hex(table),'table_location':self.location(table),'executable_methods':executable,
              'graphics_methods':graphics,'methods_outside_table_module':sum(self.owner(m)!=owner for m in methods),
              'method_slots':{name:self.location(methods[index]) for name,index in named.items()}}
        self.table_info[table]=info
        return info

    def candidate(self,address,source):
        if address in self.objects or address%4 or not self.readable(address,4,True):return
        self.objects.add(address);self.counts['object_pointers']+=1
        table=u32(self.fetch(address,4))
        info=self.describe(table)
        if info and len(self.samples)<24:
            self.samples.append(dict(info,object_address=hex(address),source=source))

    def scan(self,start,length,source,indirect=False):
        end=start+length;cursor=start
        while cursor<end:
            if time.monotonic()>=self.deadline:self.truncated=True;return
            try:
                m=self.region(cursor);stop=min(end,m['end'])
                if stop<=cursor:raise ReaderError('Region vacia.')
                if m['committed'] and not m['protect']&0x101 and (m['protect']&255) in (4,8):
                    first=(cursor+3)&~3
                    for chunk in range(first,stop,65536):
                        size=min(stop-chunk,65536)//4*4
                        if not size:continue
                        data=self.fetch(chunk,size)
                        for offset in range(0,size,4):
                            pointer=u32(data,offset)
                            try:
                                self.candidate(pointer,dict(source,offset=hex(chunk+offset-start)))
                                if indirect and pointer>=0x10000 and not pointer%4 and pointer not in self.roots:
                                    root=self.region(pointer)
                                    if root['committed'] and root['kind']==0x20000 and self.readable(pointer,4,True):
                                        if len(self.roots)>=self.root_limit:self.truncated=True;continue
                                        self.roots.add(pointer)
                                        self.scan(pointer,min(4096,root['end']-pointer),
                                                  dict(kind='referenced_structure',root_address=hex(pointer)),False)
                            except ReaderError:self.counts['read_failures']+=1
            except ReaderError:
                self.counts['read_failures']+=1;cursor=(cursor&~4095)+4096;continue
            except TimeoutError:self.truncated=True;return
            cursor=stop


def inspect(read,query,base,size,modules,**limits):
    report={'mode':'READ ONLY','diagnostic':'graphics-0.4.12','possible_device_tables':[],
            'note':'Candidates are diagnostic hints, not validated COM interfaces. No remote methods or controls executed.'}
    report['graphics_modules']=[dict(m,base=hex(m['base'])) for m in modules if is_graphics(m['name'])]
    audit=Audit(read,query,modules,**limits)
    factory=[]
    for address,length in code_sections(read,base,size):
        data=audit.fetch(address,length)
        for hit in matches(data,FACTORY_SIGNATURE):
            slot=u32(data,hit+6)
            row={'call_rva':hex(address+hit-base),'slot_in_game':base<=slot<=base+size-4}
            if row['slot_in_game']:
                row['slot_rva']=hex(slot-base)
                try:
                    obj=u32(audit.fetch(slot,4));table=u32(audit.fetch(obj,4)) if audit.readable(obj,4) else 0
                    row.update(object_address=hex(obj),table_location=audit.location(table))
                except ReaderError:row['read_failed']=True
            factory.append(row)
    report['factory_pattern_matches']=len(factory);report['factory_calls']=factory[:16]
    for start,length in data_sections(read,base,size):
        audit.scan(start,length,dict(kind='game_data',rva=hex(start-base)),True)
    report['counts']=dict(audit.counts,roots=len(audit.roots),bytes_read=audit.bytes)
    report['truncated']=audit.truncated
    report['possible_device_tables']=audit.samples
    return report


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--exe',default=DEFAULT_EXE)
    args=parser.parse_args();process=None;query_handle=None
    try:
        pid=find_pid();process=WindowsProcess(pid,args.exe)
        locate_player_slot(process.read,process.base,process.size) # Exact RE4 UHD1.1.0 prerequisite.
        modules=loaded_modules(process,pid);query_handle,query=query_memory(process,pid)
        print('Revisando graficos, solo lectura. Puede tardar unos20 segundos.',flush=True)
        report=inspect(process.read,query,process.base,process.size,modules)
        filename=Path(__file__).resolve().parents[1]/('graphics-probe-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%d-%H%M%S')+'.json')
        filename.write_text(json.dumps(report,indent=2,ensure_ascii=True)+'\n',encoding='utf-8')
        print(json.dumps(report['counts']),flush=True)
        print('Informe guardado:',filename,flush=True)
        print('No se modifico RE4 ni se activaron controles.',flush=True)
    except (ReaderError,OSError,ValueError,TimeoutError,struct.error) as error:
        print('ERROR:',error,flush=True);return 1
    finally:
        if query_handle:process.k.CloseHandle(query_handle)
        if process:process.close()
    return 0


if __name__=='__main__':raise SystemExit(main())
