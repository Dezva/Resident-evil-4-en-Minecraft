"""Bounded native scene/activity telemetry and explicit contextual action requests."""
import ctypes
from ctypes import wintypes as w
from re4_reader import ReaderError
from fake_re4_host import ProtocolError
MAGIC=0x52453452

class NativeRuntime:
    def __init__(self,pid,k):
        self.k=k;self.available=False;self.address=None;self.handle=None
        k.OpenFileMappingW.argtypes=[w.DWORD,w.BOOL,w.LPCWSTR];k.OpenFileMappingW.restype=w.HANDLE
        k.MapViewOfFile.argtypes=[w.HANDLE,w.DWORD,w.DWORD,w.DWORD,ctypes.c_size_t];k.MapViewOfFile.restype=ctypes.c_void_p
        self.handle=k.OpenFileMappingW(6,False,f'Local\\RE4Bridge-Runtime-{pid}')
        if not self.handle:
            error=ctypes.get_last_error()
            if error==2:return
            raise ReaderError(f'Canal de escenas no disponible (Windows {error})')
        self.address=k.MapViewOfFile(self.handle,6,0,0,64)
        if not self.address:self.close();raise ReaderError('No se pudo mapear escenas')
        self.words=(ctypes.c_uint32*16).from_address(self.address)
        if tuple(self.words[:4])!=(MAGIC,1,442,pid):self.close();raise ReaderError('Canal de escenas incompatible')
        self.available=True;self.publish(0,0)
    def publish(self,generation,action_id):
        if type(action_id) is not int or not 0<=action_id<=2147483647:raise ProtocolError('Invalid action id')
        if not self.available:return
        seq=(self.words[12]+1)|1;self.words[12]=seq&0xffffffff
        self.words[13]=self.k.GetTickCount();self.words[14]=action_id;self.words[15]=generation
        self.words[12]=(seq+1)&0xffffffff
    def status(self):
        if not self.available:return dict(available=False,fresh=False)
        for _ in range(3):
            before=self.words[4]
            if before&1:continue
            stamp,scene,flags,caps,contacts,corrections,actions=list(self.words[5:12])
            if before!=self.words[4]:continue
            if scene>2 or caps>3:raise ReaderError('Estado de escenas no valido')
            return dict(available=True,fresh=((self.k.GetTickCount()-stamp)&0xffffffff)<500,scene=scene,flags=flags,
                enemies_available=bool(caps&1),action_available=bool(caps&2),enemy_contacts=contacts,enemy_corrections=corrections,actions=actions)
        return dict(available=True,fresh=False)
    def close(self):
        if self.available:self.publish(0,0)
        self.available=False
        if self.address:self.k.UnmapViewOfFile(self.address);self.address=None
        if self.handle:self.k.CloseHandle(self.handle);self.handle=None
