"""Read-only audit of the conditions used by the 0.4.15 creation hook.
No COM calls, device discovery, import patching or process writes.
"""
import argparse
import ctypes
from ctypes import wintypes as w
import datetime
import json
import ntpath
from pathlib import Path
import struct
import time
from re4_reader import DEFAULT_EXE,ReaderError,WindowsProcess,find_pid
from re4_graphics_probe import loaded_modules
from re4_graphics_capture import read_capture


class AuditFailure(ReaderError):
    def __init__(self,reason):super().__init__(reason);self.reason=reason


class Budget:
    def __init__(self,read,max_bytes=2*1024*1024,max_reads=20000,seconds=10):
        self.read=read;self.bytes=0;self.reads=0;self.max_bytes=max_bytes;self.max_reads=max_reads
        self.deadline=time.monotonic()+seconds

    def get(self,address,length):
        if self.reads>=self.max_reads or length>self.max_bytes-self.bytes or time.monotonic()>=self.deadline:
            raise AuditFailure('audit_resource_limit')
        self.reads+=1;self.bytes+=length
        return self.read(address,length)


class Image:
    def __init__(self,budget,base,size,label):
        self.budget=budget;self.base=base;self.size=size;self.label=label
        dos=self.get(0,64)
        if dos[:2]!=b'MZ':raise AuditFailure(label+'_dos_header')
        pe=struct.unpack_from('<I',dos,0x3c)[0]
        if not 64<=pe<=1024*1024:raise AuditFailure(label+'_pe_offset')
        header=self.get(pe,248)
        if header[:4]!=b'PE\0\0' or struct.unpack_from('<H',header,4)[0]!=0x14c:
            raise AuditFailure(label+'_not_pe32_i386')
        if struct.unpack_from('<H',header,20)[0]!=224 or struct.unpack_from('<H',header,24)[0]!=0x10b:
            raise AuditFailure(label+'_optional_header_not_224_pe32')
        if struct.unpack_from('<I',header,80)[0]!=size:raise AuditFailure(label+'_image_size')
        if struct.unpack_from('<I',header,116)[0]<2:raise AuditFailure(label+'_directories_missing')
        self.export_rva,self.export_size,self.import_rva,self.import_size=struct.unpack_from('<4I',header,120)

    def get(self,rva,length):
        if rva<0 or length<=0 or rva>self.size or length>self.size-rva:
            raise AuditFailure(self.label+'_rva_out_of_image')
        try:return self.budget.get(self.base+rva,length)
        except AuditFailure:raise
        except ReaderError as error:raise AuditFailure(self.label+'_memory_unreadable') from error

    def u32(self,rva):return struct.unpack('<I',self.get(rva,4))[0]

    def string(self,rva,limit=128):
        out=bytearray()
        # Byte reads stop at NUL, so a short name at the end of a region works.
        for offset in range(limit):
            value=self.get(rva+offset,1)[0]
            if not value:return out.decode('ascii',errors='replace')
            out.append(value)
        raise AuditFailure(self.label+'_string_unterminated')


def export_create9(image):
    rva,length=image.export_rva,image.export_size
    if not rva or length<40:raise AuditFailure('d3d9_export_directory_missing')
    image.get(rva,length)
    row=image.get(rva,40)
    ordinal_base,functions,names,func_table,name_table,ordinal_table=struct.unpack_from('<6I',row,16)
    if not 0<functions<=8192 or names>8192:raise AuditFailure('d3d9_export_count_invalid')
    function_rows=image.get(func_table,functions*4)
    name_rows=image.get(name_table,names*4) if names else b''
    ordinals=image.get(ordinal_table,names*2) if names else b''
    hits=[]
    for i in range(names):
        name_rva=struct.unpack_from('<I',name_rows,i*4)[0]
        if image.string(name_rva)!='Direct3DCreate9':continue
        ordinal=struct.unpack_from('<H',ordinals,i*2)[0]
        if ordinal>=functions:raise AuditFailure('d3d9_export_ordinal_invalid')
        target=struct.unpack_from('<I',function_rows,ordinal*4)[0]
        if target>=image.size:raise AuditFailure('d3d9_export_target_outside_image')
        if rva<=target<rva+length:raise AuditFailure('d3d9_create9_export_forwarded')
        hits.append((image.base+target,ordinal_base+ordinal))
    if len(hits)!=1:raise AuditFailure('d3d9_create9_export_not_unique')
    return hits[0]


def location(address,modules):
    for module in modules:
        if module['base']<=address<module['base']+module['size']:
            return dict(module=module['name'],rva=hex(address-module['base']))
    return dict(module=None,rva=None)


def inspect(read,base,size,modules,expected_directory,**limits):
    budget=Budget(read,**limits)
    report={'mode':'READ ONLY IMPORT AUDIT','diagnostic':'0.4.15',
            'modules':[{key:m[key] for key in ('name','path')} for m in modules if m['name'].lower() in {'bio4.exe','d3d9.dll','dinput8.dll'}],
            'd3d9_descriptors':[], 'current_native_lookup_status':None,
            'scope':'Current loaded image; does not reconstruct DLL startup conditions.'}
    try:
        main=Image(budget,base,size,'main')
        report['import_directory']={'rva':hex(main.import_rva),'size':main.import_size}
        libraries=[m for m in modules if m['name'].lower()=='d3d9.dll']
        if not libraries:
            report['current_native_lookup_status']=2;raise AuditFailure('d3d9_module_missing')
        if len(libraries)!=1:raise AuditFailure('d3d9_module_ambiguous')
        library=libraries[0]
        expected=ntpath.join(expected_directory,'d3d9.dll')
        report['system_library_path']={'expected':expected,'loaded':library['path'],
            'matches':ntpath.normcase(library['path'])==ntpath.normcase(expected)}
        if not report['system_library_path']['matches']:
            report['current_native_lookup_status']=3;raise AuditFailure('d3d9_system_path_mismatch')
        target,ordinal=export_create9(Image(budget,library['base'],library['size'],'d3d9'))
        report['direct3dcreate9_export']={'address':hex(target),'ordinal':ordinal}
        imports,length=main.import_rva,main.import_size
        if not imports and not length:
            report['current_native_lookup_status']=2;raise AuditFailure('main_no_import_directory')
        if not imports or length<20 or length>131072 or imports>size or length>size-imports:
            raise AuditFailure('main_import_directory_bounds')
        terminated=False;matches=[];failures=[]
        for i in range(min(length//20,256)):
            row=main.get(imports+i*20,20)
            names,stamp,chain,name,iat=struct.unpack('<5I',row)
            if not any((names,stamp,chain,name,iat)):terminated=True;break
            if main.string(name,64).lower()!='d3d9.dll':continue
            descriptor={'index':i,'original_first_thunk':hex(names),'first_thunk':hex(iat),'entries':[], 'omitted_entries':0}
            report['d3d9_descriptors'].append(descriptor)
            if not iat or iat&3 or iat>=size:raise AuditFailure('d3d9_iat_invalid')
            if not names:
                # Describe resolved IAT values only; never use them as names or hook targets.
                for j in range(min(256,(size-iat)//4)):
                    value=main.u32(iat+j*4)
                    if not value:break
                    if value==target:descriptor.setdefault('export_found_in_bound_iat',[]).append(hex(iat+j*4))
                failures.append('d3d9_original_first_thunk_missing');continue
            if names&3 or names>=size:raise AuditFailure('d3d9_original_thunk_invalid')
            end=False
            for j in range(4096):
                entry=main.u32(names+j*4);value=main.u32(iat+j*4)
                if not entry:end=True;break
                detail={'slot_rva':hex(iat+j*4),'value':hex(value),'target_location':location(value,modules)}
                if entry&0x80000000:
                    detail['ordinal']=entry&0xffff
                    if detail['ordinal']==ordinal:detail['is_create9_ordinal']=True
                elif entry>size or size-entry<2:
                    detail['invalid_name_rva']=hex(entry)
                else:
                    detail['name']=main.string(entry+2,64)
                    if detail['name']=='Direct3DCreate9':
                        detail['matches_system_export']=value==target
                        matches.append(detail)
                        if value!=target:failures.append('create9_iat_target_differs_from_system_export')
                if len(descriptor['entries'])<64:descriptor['entries'].append(detail)
                else:descriptor['omitted_entries']+=1
            if not end:failures.append('d3d9_thunk_table_unterminated')
        report['matching_named_imports']=matches
        if not terminated:failures.append('main_import_descriptors_unterminated')
        if len(matches)>1:failures.append('create9_named_import_ambiguous')
        if failures:
            report['current_native_lookup_status']=3;report['reasons']=list(dict.fromkeys(failures))
        elif not matches:
            report['current_native_lookup_status']=2;report['reasons']=['create9_named_import_not_found']
        else:
            report['current_native_lookup_status']=1;report['reasons']=['current_import_matches_system_export']
    except (AuditFailure,ReaderError,struct.error) as error:
        report['reasons']=[getattr(error,'reason',str(error))]
        report['inspection_incomplete']=True
    report['reads']=budget.reads;report['bytes']=budget.bytes
    return report


def system_directory(k):
    for name in ('GetSystemWow64DirectoryW','GetSystemDirectoryW'):
        function=getattr(k,name);function.argtypes=[w.LPWSTR,w.UINT];function.restype=w.UINT
        buffer=ctypes.create_unicode_buffer(260);length=function(buffer,260)
        if 0<length<260:return buffer.value
    raise ReaderError('No se pudo leer el directorio de sistema.')


def main(argv=None):
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-exe',default=DEFAULT_EXE)
    parser.add_argument('--output-dir',type=Path,default=Path(__file__).resolve().parents[1])
    args=parser.parse_args(argv);process=None;report={'mode':'READ ONLY IMPORT AUDIT','diagnostic':'0.4.15'};code=0
    try:
        pid=find_pid();process=WindowsProcess(pid,args.game_exe)
        modules=loaded_modules(process,pid)
        report=inspect(process.read,process.base,process.size,modules,system_directory(process.k))
        try:report['native_capture']=read_capture(process.k,pid)
        except ReaderError as error:report['capture_read_error']=str(error)
        if report['current_native_lookup_status']==1 and report.get('native_capture',{}).get('hook_status')==3:
            report['startup_note']='Current import passes; the reason for the earlier startup rejection remains unknown.'
        print('Diagnostico de imports:',', '.join(report['reasons']))
    except (ReaderError,OSError,ValueError,struct.error) as error:report['error']=str(error);print('ERROR:',error);code=1
    finally:
        if process is not None:process.close()
    report['generated_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat()
    path=args.output_dir/('imports-audit-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S')+'.json')
    try:path.write_text(json.dumps(report,indent=2,allow_nan=False)+'\n',encoding='utf-8')
    except OSError as error:print('ERROR al guardar informe:',error);return 1
    print('Informe:',path);return code


if __name__=='__main__':raise SystemExit(main())
