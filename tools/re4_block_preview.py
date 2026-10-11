"""One visual-only box, through a separate named mapping; no process-memory writes."""
import ctypes
from ctypes import wintypes as w
import math
from fake_re4_host import ProtocolError
from re4_reader import ReaderError


def validate_preview(preview):
    if preview is None: return
    if not isinstance(preview,dict) or set(preview)!={'bounds'}:
        raise ProtocolError('Invalid block preview')
    bounds=preview['bounds']
    if not isinstance(bounds,list) or len(bounds)!=6 or any(type(x) not in (int,float)
            or not math.isfinite(x) or abs(x)>20000000 for x in bounds):
        raise ProtocolError('Invalid preview coordinates')
    if any(not 40<=bounds[i+3]-bounds[i]<=2500 for i in range(3)):
        raise ProtocolError('Invalid preview size')


class NativePreview:
    def __init__(self,pid):
        self.k=ctypes.WinDLL('kernel32',use_last_error=True)
        for name,args,result in [('OpenFileMappingW',[w.DWORD,w.BOOL,w.LPCWSTR],w.HANDLE),
                ('MapViewOfFile',[w.HANDLE,w.DWORD,w.DWORD,w.DWORD,ctypes.c_size_t],ctypes.c_void_p),
                ('UnmapViewOfFile',[ctypes.c_void_p],w.BOOL),('CloseHandle',[w.HANDLE],w.BOOL),('GetTickCount',[],w.DWORD)]:
            fn=getattr(self.k,name);fn.argtypes=args;fn.restype=result
        self.handle=self.k.OpenFileMappingW(0xF001F,False,f'Local\\RE4Bridge-BlockPreview-{pid}')
        if not self.handle: raise ReaderError('DLL sin canal de bloque visual (requiere0.4.11).')
        self.address=self.k.MapViewOfFile(self.handle,0xF001F,0,0,68)
        if not self.address:
            self.k.CloseHandle(self.handle);raise ReaderError('No se pudo abrir bloque visual.')
        self.words=(ctypes.c_uint32*17).from_address(self.address)
        if self.words[0]!=0x52453442 or self.words[1] not in (1,2,3):
            self.k.UnmapViewOfFile(self.address);self.k.CloseHandle(self.handle)
            raise ReaderError('Canal de bloque desconocido.')
        if self.words[1] in (2,3):
            version=self.words[1];size=92 if version==2 else 100
            self.k.UnmapViewOfFile(self.address)
            self.address=self.k.MapViewOfFile(self.handle,0xF001F,0,0,size)
            if not self.address:
                self.k.CloseHandle(self.handle);raise ReaderError('Canal de diagnostico de bloque incompleto.')
            self.words=(ctypes.c_uint32*(size//4)).from_address(self.address)
            if self.words[0]!=0x52453442 or self.words[1]!=version:
                self.k.UnmapViewOfFile(self.address);self.k.CloseHandle(self.handle)
                raise ReaderError('El canal de bloque cambio durante la apertura.')

    def publish(self,enabled,generation,bounds=None):
        if enabled: validate_preview({'bounds':bounds})
        seq=(self.words[2]+1)|1
        self.words[2]=seq&0xFFFFFFFF
        self.words[3]=self.k.GetTickCount()
        self.words[4]=int(enabled)
        self.words[5]=generation
        for i,x in enumerate(bounds if enabled else [0]*6):
            ctypes.c_float.from_address(self.address+24+i*4).value=x
        self.words[2]=(seq+1)&0xFFFFFFFF

    def status(self):
        result=dict(zip(['status','frames','draws','failures','last_hresult'],self.words[12:17]))
        if len(self.words)>=23:
            result['discovery']=dict(zip(['attempts','slots','tables','devices','window_rejected','roots'],self.words[17:23]))
        if len(self.words)==25:
            result['discovery'].update(factory_rejected=self.words[23],probe_valid=self.words[24])
        return result

    def close(self):
        self.publish(False,0)
        self.k.UnmapViewOfFile(self.address);self.k.CloseHandle(self.handle)
