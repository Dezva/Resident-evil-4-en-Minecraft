"""Read creation-only native telemetry. No device calls or process writes."""
import argparse
import ctypes
from ctypes import wintypes as w
import datetime
import json
from pathlib import Path
import struct
from re4_reader import DEFAULT_EXE,ReaderError,WindowsProcess,find_pid

MAGIC=0x52453447
SIZE=96
HOOK_STATES={0:'no intentado',1:'importacion interceptada',2:'importacion no disponible',
             3:'incompatible o modificada',4:'fallo de proteccion; captura rechazada'}


def decode_capture(data,pid):
    if len(data)!=SIZE:raise ReaderError('Tamano de captura incompatible.')
    words=struct.unpack('<24I',data)
    if words[0]!=MAGIC or words[1]!=1 or words[3]!=415 or words[4]!=pid:
        raise ReaderError('Cabecera/build/PID de captura incompatible; se requiere DLL 0.4.15.')
    if words[2]&1:raise ReaderError('Instantanea de captura incompleta.')
    if words[5] not in HOOK_STATES:raise ReaderError('Estado de captura desconocido.')
    return dict(build='0.4.15',hook_status=words[5],hook_description=HOOK_STATES[words[5]],
                factory_calls=words[6],factory_success=words[7],wrappers_live=words[8],wrap_failures=words[9],
                device_calls=words[10],device_success=words[11],device_failures=words[12],
                last_hresult=f'0x{words[13]:08x}',last_factory_address=f'0x{words[14]:08x}',
                last_device_address=f'0x{words[15]:08x}',adapter=words[16],device_type=words[17],
                behavior_flags=f'0x{words[18]:08x}',windowed=bool(words[19]),
                width=words[20],height=words[21],focus_window=f'0x{words[22]:08x}',
                escaped_interfaces=words[23],creation_observed=words[11]>0,
                address_semantics='Historical API return values; no current device lifetime is asserted.',
                rendering_enabled=False)


def read_capture(k,pid):
    k.OpenFileMappingW.argtypes=[w.DWORD,w.BOOL,w.LPCWSTR];k.OpenFileMappingW.restype=w.HANDLE
    k.MapViewOfFile.argtypes=[w.HANDLE,w.DWORD,w.DWORD,w.DWORD,ctypes.c_size_t];k.MapViewOfFile.restype=ctypes.c_void_p
    k.UnmapViewOfFile.argtypes=[ctypes.c_void_p];k.UnmapViewOfFile.restype=w.BOOL
    k.CloseHandle.argtypes=[w.HANDLE];k.CloseHandle.restype=w.BOOL
    handle=k.OpenFileMappingW(4,False,f'Local\\RE4Bridge-GraphicsCapture-{pid}')
    if not handle:raise ReaderError('Canal de captura ausente. Abre RE4 con la DLL 0.4.15; usa los mismos permisos que el juego.')
    view=None
    try:
        view=k.MapViewOfFile(handle,4,0,0,SIZE)
        if not view:raise ReaderError('No se pudo leer el canal de captura.')
        for _ in range(8):
            before=struct.unpack('<I',ctypes.string_at(view+8,4))[0]
            if before&1:continue
            data=ctypes.string_at(view,SIZE)
            after=struct.unpack('<I',ctypes.string_at(view+8,4))[0]
            if before==after==struct.unpack_from('<I',data,8)[0]:return decode_capture(data,pid)
        raise ReaderError('Captura cambiando; repite el diagnostico dentro de unos segundos.')
    finally:
        if view:k.UnmapViewOfFile(view)
        k.CloseHandle(handle)


def main(argv=None):
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-exe',default=DEFAULT_EXE)
    parser.add_argument('--output-dir',type=Path,default=Path(__file__).resolve().parents[1])
    args=parser.parse_args(argv)
    process=None;report={'mode':'READ ONLY CAPTURE IPC','diagnostic':'0.4.15',
                         'generated_utc':datetime.datetime.now(datetime.timezone.utc).isoformat()}
    result=0
    try:
        pid=find_pid();process=WindowsProcess(pid,args.game_exe)
        report['capture']=read_capture(process.k,pid)
        state=report['capture']
        print(f"Captura grafica: {state['hook_description']}; creaciones correctas={state['device_success']}; dibujo DESACTIVADO.")
    except (ReaderError,OSError,ValueError,struct.error) as error:
        report['error']=str(error);print('ERROR:',error);result=1
    finally:
        if process is not None:process.close()
    output=args.output_dir/('graphics-capture-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S')+'.json')
    try:
        output.write_text(json.dumps(report,indent=2,allow_nan=False)+'\n',encoding='utf-8')
        print('Informe:',output)
    except OSError as error:print('ERROR al guardar informe:',error);return 1
    return result


if __name__=='__main__':raise SystemExit(main())
