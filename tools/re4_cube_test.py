"""Explicit, time-limited cube test; writes only the cube command mapping.
Native rendering runs on RE4's own scene thread. No Minecraft or controller needed.
"""
import argparse
import ctypes
from ctypes import wintypes as w
import datetime
import json
from pathlib import Path
import secrets
import struct
import time
from re4_reader import DEFAULT_EXE,ReaderError,WindowsProcess,find_pid

FIELDS=['build','pid','live_devices','wrap_failures','escaped_interfaces','begin_calls','frames','draws','failures',
        'restore_failures','last_hresult','last_restore_hresult','last_scene_active','stop_reason','reset_calls',
        'reset_failures','present_failures','lease_rejections','camera_rejections','skipped_passes']
REASONS={0:'activo',1:'desactivado',2:'permiso caducado',3:'F8',4:'dispositivo perdido o requiere nueva prueba',
         5:'fallo al restaurar estados; reinicia RE4',6:'interfaz nativa externa; dibujo rechazado',7:'camara no disponible',8:'otro destino de dibujo',9:'fallo grafico'}

def configure(k):
    for name,args,result in [
        ('OpenFileMappingW',[w.DWORD,w.BOOL,w.LPCWSTR],w.HANDLE),
        ('MapViewOfFile',[w.HANDLE,w.DWORD,w.DWORD,w.DWORD,ctypes.c_size_t],ctypes.c_void_p),
        ('UnmapViewOfFile',[ctypes.c_void_p],w.BOOL),('CloseHandle',[w.HANDLE],w.BOOL),
        ('CreateMutexW',[ctypes.c_void_p,w.BOOL,w.LPCWSTR],w.HANDLE),
        ('WaitForSingleObject',[w.HANDLE,w.DWORD],w.DWORD),('ReleaseMutex',[w.HANDLE],w.BOOL),
        ('GetTickCount',[],w.DWORD)]:
        fn=getattr(k,name);fn.argtypes=args;fn.restype=result

def read_cube_status(k,pid):
    configure(k)
    handle=k.OpenFileMappingW(4,False,f'Local\\RE4Bridge-CubeTelemetry-{pid}')
    if not handle:raise ReaderError('Canal de cubo ausente; requiere DLL0.4.18/0.4.19 y creacion grafica correcta.')
    view=None
    try:
        view=k.MapViewOfFile(handle,4,0,0,92)
        if not view:raise ReaderError('No se pudo leer el estado del cubo.')
        for _ in range(8):
            before=struct.unpack('<I',ctypes.string_at(view+8,4))[0]
            if before&1:continue
            data=ctypes.string_at(view,92);words=struct.unpack('<23I',data)
            after=struct.unpack('<I',ctypes.string_at(view+8,4))[0]
            if before!=after or before!=words[2]:continue
            if words[:2]!=(0x52453455,1) or words[3] not in (418,419) or words[4]!=pid:
                raise ReaderError('Version/build/PID de cubo incompatible.')
            report=dict(zip(FIELDS,words[3:]))
            if report['stop_reason'] not in REASONS or report['last_scene_active'] not in (0,1):
                raise ReaderError('Estado del cubo desconocido.')
            report['reason']=REASONS[report['stop_reason']]
            for field in ['last_hresult','last_restore_hresult']:report[field]=f"0x{report[field]:08x}"
            report['last_scene_active']=bool(report['last_scene_active'])
            report['semantics']='Snapshot of last observed scene; active flag is not a live command lease.'
            return report
        raise ReaderError('Estado de cubo cambiando; repite el diagnostico.')
    finally:
        if view:k.UnmapViewOfFile(view)
        k.CloseHandle(handle)

class CubeSession:
    def __init__(self,k,pid,user=None):
        configure(k);self.k=k;self.pid=pid;self.mutex=self.handle=self.address=None
        self.user=user if user is not None else ctypes.WinDLL('user32',use_last_error=True)
        self.user.GetAsyncKeyState.argtypes=[ctypes.c_int];self.user.GetAsyncKeyState.restype=ctypes.c_short
        self.locked=False;self.started=False;self.epoch=secrets.randbits(32) or 1
        try:
            self.mutex=k.CreateMutexW(None,False,f'Local\\RE4Bridge-CubeWriter-{pid}')
            if not self.mutex or k.WaitForSingleObject(self.mutex,0) not in (0,0x80):
                raise ReaderError('Ya hay otra prueba del cubo abierta o no se pudo obtener el permiso.')
            self.locked=True
            self.handle=k.OpenFileMappingW(6,False,f'Local\\RE4Bridge-CubeCommand-{pid}')
            if not self.handle:raise ReaderError('Canal de activacion del cubo ausente.')
            self.address=k.MapViewOfFile(self.handle,6,0,0,32)
            if not self.address:raise ReaderError('No se pudo abrir activacion del cubo.')
            self.words=(ctypes.c_uint32*8).from_address(self.address)
            if self.words[0]!=0x52453454 or self.words[1] not in (1,2):
                raise ReaderError('Canal de activacion del cubo incompatible.')
            self.version=self.words[1]
            if self.version==2:
                k.UnmapViewOfFile(self.address);self.address=None
                self.address=k.MapViewOfFile(self.handle,6,0,0,56)
                if not self.address:raise ReaderError('Canal de cubo v2 incompleto.')
                self.words=(ctypes.c_uint32*14).from_address(self.address)
                if self.words[0]!=0x52453454 or self.words[1]!=2:
                    raise ReaderError('Cabecera del cubo cambio durante apertura.')
        except Exception:
            self.close();raise

    def publish(self,enabled,bounds=None):
        if bounds is not None:
            from re4_block_preview import validate_preview
            validate_preview({'bounds':bounds})
            if self.version!=2:raise ReaderError('Bloques requieren DLL0.4.19.')
        seq=(self.words[2]+1)|1;self.words[2]=seq&0xffffffff
        self.words[3]=self.k.GetTickCount();self.words[4]=int(enabled);self.words[5]=self.epoch
        self.words[6]=int(enabled and bounds is not None);self.words[7]=0
        if self.version==2:
            for i,value in enumerate(bounds if enabled and bounds is not None else [0]*6):
                ctypes.c_float.from_address(self.address+32+4*i).value=value
        self.words[2]=(seq+1)&0xffffffff;self.started=True

    def close(self):
        if self.address:
            if self.started and self.words[5]==self.epoch:self.publish(False)
            self.k.UnmapViewOfFile(self.address);self.address=None
        if self.handle:self.k.CloseHandle(self.handle);self.handle=None
        if self.locked:self.k.ReleaseMutex(self.mutex);self.locked=False
        if self.mutex:self.k.CloseHandle(self.mutex);self.mutex=None

def run_test(session,seconds):
    k=session.k;before=read_cube_status(k,session.pid)
    if not before['live_devices']:raise ReaderError('No hay dispositivo envuelto; envia CAPTURE-RE4-GRAPHICS.')
    if before['restore_failures']:raise ReaderError('Fallo previo de restauracion. Reinicia RE4 antes de otra prueba.')
    for remaining in range(5,0,-1):
        print(f'Inicio en {remaining}s. Vuelve a RE4 con Alt+Tab; F8 cancela.',flush=True)
        for _ in range(20):
            if session.user.GetAsyncKeyState(119)&0x8000:return {'before':before,'after':read_cube_status(k,session.pid),'stop':'F8 before activation'}
            time.sleep(.05)
    print(f'CUBO ACTIVADO durante {seconds}s. F8 detiene.',flush=True)
    started=time.monotonic();next_report=0;stop='duration';after=before
    while time.monotonic()-started<seconds:
        if session.user.GetAsyncKeyState(119)&0x8000:stop='F8';break
        session.publish(True)
        after=read_cube_status(k,session.pid)
        if after['restore_failures'] or after['escaped_interfaces'] or after['failures']>before['failures'] or after['present_failures']>before['present_failures'] or after['reset_calls']>before['reset_calls']:
            stop='native safety stop';break
        elapsed=time.monotonic()-started
        if elapsed>=next_report:
            print(f"{elapsed:.0f}/{seconds}s: frames={after['frames']-before['frames']} dibujos={after['draws']-before['draws']} fallos={after['failures']-before['failures']} ({after['reason']})",flush=True)
            next_report=elapsed+1
        time.sleep(.05)
    session.publish(False)
    return {'before':before,'after':after,'stop':stop,'elapsed_seconds':time.monotonic()-started,
            'drawing_observed':after['draws']>before['draws']}

def main(argv=None):
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seconds',type=int,default=30)
    parser.add_argument('--game-exe',default=DEFAULT_EXE)
    parser.add_argument('--output-dir',type=Path,default=Path(__file__).resolve().parents[1])
    args=parser.parse_args(argv)
    if not 1<=args.seconds<=120:parser.error('--seconds debe estar entre1 y120')
    process=session=None;result=0
    report={'diagnostic':'0.4.19','mode':'EXPLICIT CUBE LEASE; no process-memory writes',
            'generated_utc':datetime.datetime.now(datetime.timezone.utc).isoformat()}
    try:
        pid=find_pid();process=WindowsProcess(pid,args.game_exe);session=CubeSession(process.k,pid)
        report.update(run_test(session,args.seconds))
        if report['stop']=='native safety stop':result=1
        print('Prueba terminada. Cubo desactivado.',flush=True)
    except KeyboardInterrupt:report['stop']='interrupted'
    except (ReaderError,OSError,ValueError,struct.error) as error:
        report['error']=str(error);print('ERROR:',error);result=1
    finally:
        if session is not None:session.close()
        if process is not None:process.close()
    output=args.output_dir/('cube-test-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S')+'.json')
    try:output.write_text(json.dumps(report,indent=2,allow_nan=False)+'\n',encoding='utf-8')
    except OSError as error:print('ERROR al guardar:',error);return 1
    print('Informe:',output);return result

if __name__=='__main__':raise SystemExit(main())
