"""Read creation-only native telemetry. No device calls or process writes."""
import argparse
import ctypes
from ctypes import wintypes as w
import datetime
import json
import os
from pathlib import Path
import struct
from re4_reader import DEFAULT_EXE,ReaderError,find_pid

MAGIC=0x52453447
SIZE=96
SIZES={1:96,2:168,3:3296}
HOOK_STATES={0:'no intentado',1:'importacion interceptada',2:'importacion no disponible',
             3:'incompatible o modificada',4:'fallo de proteccion; captura rechazada'}
INSTALL_REASONS={0:'sin intento',1:'importacion instalada',2:'cabecera DOS no valida',
    3:'cabecera PE no legible',4:'tamano de imagen no valido',5:'d3d9 no cargada',
    6:'ruta de sistema no disponible',7:'ruta de d3d9 distinta del sistema',8:'exportacion no disponible',
    9:'cabecera PE32 incompatible',10:'directorio de imports ausente',11:'limites del directorio de imports',
    12:'descriptor de importacion no legible',13:'tabla de nombres ausente o no valida',14:'tabla de imports no legible',
    15:'destino de importacion distinto de la exportacion',16:'tabla de imports sin terminador',
    17:'descriptores sin terminador',18:'importacion ambigua',19:'importacion por nombre no encontrada',
    20:'consulta de memoria fallo',21:'proteccion de escritura fallo',22:'importacion cambio al instalar',
    23:'restauracion de proteccion fallo'}


def layout(data,pid):
    if len(data)<24:raise ReaderError('Cabecera de captura incompleta.')
    magic,version,_,build,owner=struct.unpack_from('<5I',data)
    if magic!=MAGIC or (version,build) not in {(1,415),(2,416),(3,417),(3,418),(3,419),(3,421),(3,422),(3,423),(3,424),(3,426),(3,427),(3,428),(3,429),(3,430),(3,432),(3,433),(3,434),(3,435),(3,436),(3,437),(3,438),(3,439),(3,441),(3,442)} or owner!=pid:
        raise ReaderError('Cabecera/build/PID de captura incompatible; se requiere una DLL de captura compatible.')
    return version,SIZES[version],f'0.4.{build-400}'


def decode_capture(data,pid):
    version,size,build=layout(data,pid)
    if len(data)!=size:raise ReaderError('Tamano de captura incompatible.')
    words=struct.unpack('<'+str(size//4)+'I',data)
    if words[2]&1:raise ReaderError('Instantanea de captura incompleta.')
    if words[5] not in HOOK_STATES:raise ReaderError('Estado de captura desconocido.')
    report=dict(build=build,hook_status=words[5],hook_description=HOOK_STATES[words[5]],
                factory_calls=words[6],factory_success=words[7],wrappers_live=words[8],wrap_failures=words[9],
                device_calls=words[10],device_success=words[11],device_failures=words[12],
                last_hresult=f'0x{words[13]:08x}',last_factory_address=f'0x{words[14]:08x}',
                last_device_address=f'0x{words[15]:08x}',adapter=words[16],device_type=words[17],
                behavior_flags=f'0x{words[18]:08x}',windowed=bool(words[19]),
                width=words[20],height=words[21],focus_window=f'0x{words[22]:08x}',
                escaped_interfaces=words[23],creation_observed=words[11]>0,
                address_semantics='Historical API return values; no current device lifetime is asserted.',
                rendering_enabled=None if build in ('0.4.18','0.4.19','0.4.21','0.4.22','0.4.23','0.4.24','0.4.26','0.4.27','0.4.28','0.4.29','0.4.30','0.4.32','0.4.33','0.4.34','0.4.35','0.4.36','0.4.37','0.4.38','0.4.39','0.4.41','0.4.42') else False)
    if version>=2:
        phases={0:'sin intento',1:'DLL_PROCESS_ATTACH',2:'DirectInput8Create'}
        if words[24] not in phases or words[25]>2:raise ReaderError('Fase/intentos de captura no validos.')
        report['installation']={'phase':phases[words[24]],'attempts':words[25]}
        for name,start in [('initial',26),('latest',34)]:
            status,reason,error,slot,observed,expected,pe,image=words[start:start+8]
            if status not in HOOK_STATES or reason not in INSTALL_REASONS:raise ReaderError('Motivo de instalacion desconocido.')
            report['installation'][name]={'status':status,'reason_code':reason,'reason':INSTALL_REASONS[reason],
                'windows_error':error,'slot_rva':hex(slot),'observed':hex(observed),'expected':hex(expected),
                'pe_rva':hex(pe),'image_size':image}
    if version==3:
        matches={0:'none',1:'SysWOW64',2:'SystemDirectory'}
        for index,name in enumerate(['initial','latest']):
            start=168+index*1564
            matched=struct.unpack_from('<I',data,start)[0]
            if matched not in matches:raise ReaderError('Identidad de ruta desconocida.')
            paths={'matched':matches[matched]}
            for field_index,field in enumerate(['loaded','wow64','system']):
                offset=start+4+field_index*520
                try:value=data[offset:offset+520].decode('utf-16-le')
                except UnicodeDecodeError as error:raise ReaderError('Ruta UTF-16 no valida.') from error
                if '\0' not in value:raise ReaderError('Ruta sin terminador.')
                paths[field]=value.split('\0',1)[0]
            report['installation'][name]['paths']=paths
    return report


def read_capture(k,pid):
    k.OpenFileMappingW.argtypes=[w.DWORD,w.BOOL,w.LPCWSTR];k.OpenFileMappingW.restype=w.HANDLE
    k.MapViewOfFile.argtypes=[w.HANDLE,w.DWORD,w.DWORD,w.DWORD,ctypes.c_size_t];k.MapViewOfFile.restype=ctypes.c_void_p
    k.UnmapViewOfFile.argtypes=[ctypes.c_void_p];k.UnmapViewOfFile.restype=w.BOOL
    k.CloseHandle.argtypes=[w.HANDLE];k.CloseHandle.restype=w.BOOL
    handle=k.OpenFileMappingW(4,False,f'Local\\RE4Bridge-GraphicsCapture-{pid}')
    if not handle:raise ReaderError('Canal de captura ausente. Abre RE4 con la DLL de captura; usa los mismos permisos que el juego.')
    view=None
    try:
        view=k.MapViewOfFile(handle,4,0,0,SIZE)
        if not view:raise ReaderError('No se pudo leer el canal de captura.')
        _,size,_=layout(ctypes.string_at(view,24),pid)
        if size!=SIZE:
            k.UnmapViewOfFile(view);view=None
            view=k.MapViewOfFile(handle,4,0,0,size)
            if not view:raise ReaderError('No se pudo leer el canal de captura extendido.')
        for _ in range(8):
            before=struct.unpack('<I',ctypes.string_at(view+8,4))[0]
            if before&1:continue
            data=ctypes.string_at(view,size)
            after=struct.unpack('<I',ctypes.string_at(view+8,4))[0]
            if before==after==struct.unpack_from('<I',data,8)[0]:return decode_capture(data,pid)
        raise ReaderError('Captura cambiando; repite el diagnostico dentro de unos segundos.')
    finally:
        if view:k.UnmapViewOfFile(view)
        k.CloseHandle(handle)


def capture_kernel():
    if os.name!='nt':raise ReaderError('Este diagnostico debe ejecutarse en Windows junto a RE4')
    return ctypes.WinDLL('kernel32',use_last_error=True)


def main(argv=None):
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-exe',default=DEFAULT_EXE)
    parser.add_argument('--output-dir',type=Path,default=Path(__file__).resolve().parents[1])
    args=parser.parse_args(argv)
    report={'mode':'READ ONLY CAPTURE IPC','diagnostic':'0.4.42',
                         'generated_utc':datetime.datetime.now(datetime.timezone.utc).isoformat()}
    result=0
    try:
        pid=find_pid();kernel=capture_kernel()
        report['capture']=read_capture(kernel,pid)
        state=report['capture']
        print(f"Captura grafica: {state['hook_description']}; creaciones correctas={state['device_success']}.")
        if state['build'] in ('0.4.18','0.4.19','0.4.21','0.4.22','0.4.23','0.4.24','0.4.26','0.4.27','0.4.28','0.4.29','0.4.30','0.4.32','0.4.33','0.4.34','0.4.35','0.4.36','0.4.37','0.4.38','0.4.39','0.4.41','0.4.42'):
            from re4_cube_test import read_cube_status
            try:
                state['cube']=read_cube_status(kernel,pid)
                print(f"Prueba de cubo: dispositivos={state['cube']['live_devices']}; frames={state['cube']['frames']}; dibujos={state['cube']['draws']}; {state['cube']['reason']}")
            except ReaderError as error:state['cube_error']=str(error)
            state['activation']='TEST-RE4-CUBE or the armed block/scene host publishes an explicit lease; this reader does not enable drawing.'
        if state['build'] in ('0.4.35','0.4.36','0.4.37','0.4.38','0.4.39','0.4.41','0.4.42'):
            from re4_depth import NativeDepth
            depth=NativeDepth(kernel,pid,readonly=True)
            try:state['depth']=depth.status()
            finally:depth.close()
        if 'installation' in state:
            journal=state['installation']
            print(f"Inicio: {journal['initial']['reason']}; ultimo intento: {journal['latest']['reason']}; fase={journal['phase']}")
    except (ReaderError,OSError,ValueError,struct.error) as error:
        report['error']=str(error);print('ERROR:',error);result=1
    output=args.output_dir/('graphics-capture-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S')+'.json')
    try:
        output.write_text(json.dumps(report,indent=2,allow_nan=False)+'\n',encoding='utf-8')
        print('Informe:',output)
    except OSError as error:print('ERROR al guardar informe:',error);return 1
    return result


if __name__=='__main__':raise SystemExit(main())
