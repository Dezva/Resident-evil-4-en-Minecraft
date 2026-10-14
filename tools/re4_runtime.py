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
        self.address=k.MapViewOfFile(self.handle,6,0,0,112)
        if not self.address:self.close();raise ReaderError('No se pudo mapear escenas')
        self.words=(ctypes.c_uint32*28).from_address(self.address)
        if tuple(self.words[:4])!=(MAGIC,3,445,pid):self.close();raise ReaderError('Canal de escenas incompatible: instala la DLL0.4.45')
        self.available=True;self.publish(0,0)
    def publish(self,generation,action_id,vertical=None,action_kind=0):
        if type(action_id) is not int or not 0<=action_id<=2147483647:raise ProtocolError('Invalid action id')
        if type(action_kind) is not int or action_kind not in (0,1) or action_kind and not action_id:raise ProtocolError('Invalid action kind')
        if not self.available:return
        seq=(self.words[12]+1)|1;self.words[12]=seq&0xffffffff
        self.words[13]=self.k.GetTickCount();self.words[14]=action_id;self.words[15]=generation
        ctypes.c_float.from_address(self.address+64).value=vertical['lift'] if vertical else 0
        self.words[17]=vertical['token'] if vertical else 0
        ctypes.c_float.from_address(self.address+72).value=vertical['scale'] if vertical else 0
        self.words[19]=int(vertical is not None);self.words[20]=action_kind
        self.words[12]=(seq+1)&0xffffffff
    def status(self):
        if not self.available:return dict(available=False,fresh=False)
        for _ in range(3):
            before=self.words[4]
            if before&1:continue
            stamp,scene,flags,caps,contacts,corrections,actions=list(self.words[5:12])
            enemy_reason,scanned,eligible,skips,_,capacity,stride=list(self.words[21:28])
            view_lift=ctypes.c_float.from_address(self.address+100).value
            if before!=self.words[4]:continue
            if scene>2 or caps>3 or enemy_reason>6 or scanned>4096 or eligible>4096:raise ReaderError('Estado de escenas no valido')
            return dict(available=True,fresh=((self.k.GetTickCount()-stamp)&0xffffffff)<500,scene=scene,flags=flags,
                enemies_available=bool(caps&1),action_available=bool(caps&2),enemy_contacts=contacts,enemy_corrections=corrections,actions=actions,enemy_reason=enemy_reason,enemy_scanned=scanned,enemy_eligible=eligible,
                skips=skips,view_lift=view_lift,enemy_capacity=capacity,enemy_stride=stride)
        return dict(available=True,fresh=False)
    def close(self):
        if self.available:self.publish(0,0)
        self.available=False
        if self.address:self.k.UnmapViewOfFile(self.address);self.address=None
        if self.handle:self.k.CloseHandle(self.handle);self.handle=None
