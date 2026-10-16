"""Minecraft/Leon health requests through a separate game-thread IPC channel."""
import ctypes
from ctypes import wintypes as w
import math
from fake_re4_host import ProtocolError
from re4_reader import ReaderError

MAGIC=0x52453448
SIZE=100

def validate_health(value):
    if value is None:return
    if not isinstance(value,dict) or set(value)!={'mode','ratio','damage_ack','healing_ack'}:raise ProtocolError('Invalid health request')
    if type(value['mode']) is not int or value['mode'] not in (1,2):raise ProtocolError('Invalid health mode')
    if type(value['ratio']) not in (int,float) or not math.isfinite(value['ratio']) or not 0<=value['ratio']<=1:raise ProtocolError('Invalid health ratio')
    for key in ('damage_ack','healing_ack'):
        if type(value[key]) is not int or not 0<=value[key]<=0xffffffff:raise ProtocolError('Invalid health acknowledgement')

class NativeHealth:
    def __init__(self,pid,k):
        self.k=k;self.handle=None;self.address=None;self.available=False
        k.OpenFileMappingW.argtypes=[w.DWORD,w.BOOL,w.LPCWSTR];k.OpenFileMappingW.restype=w.HANDLE
        k.MapViewOfFile.argtypes=[w.HANDLE,w.DWORD,w.DWORD,w.DWORD,ctypes.c_size_t];k.MapViewOfFile.restype=ctypes.c_void_p
        k.UnmapViewOfFile.argtypes=[ctypes.c_void_p];k.UnmapViewOfFile.restype=w.BOOL
        k.CloseHandle.argtypes=[w.HANDLE];k.CloseHandle.restype=w.BOOL
        k.GetTickCount.argtypes=[];k.GetTickCount.restype=w.DWORD
        self.handle=k.OpenFileMappingW(6,False,f'Local\\RE4Bridge-Health-{pid}')
        if not self.handle:
            error=ctypes.get_last_error()
            if error==2:return
            raise ReaderError(f'Canal de vida no disponible (Windows {error})')
        self.address=k.MapViewOfFile(self.handle,6,0,0,SIZE)
        if not self.address:self.close();raise ReaderError('No se pudo mapear vida')
        self.words=(ctypes.c_uint32*25).from_address(self.address)
        if tuple(self.words[:2])!=(MAGIC,1) or self.words[2] not in (441,442,443,444,445,446,447,448,449,450,451,452,453,455,456) or self.words[3]!=pid:self.close();raise ReaderError('Canal de vida incompatible')
        self.available=True;self.publish(False,0)

    def publish(self,enabled,generation,request=None):
        if not self.available:return
        validate_health(request)
        seq=(self.words[4]+1)|1;self.words[4]=seq&0xffffffff
        self.words[5]=self.k.GetTickCount();self.words[6]=int(enabled and request is not None);self.words[7]=generation
        self.words[8]=request['mode'] if request else 0
        ctypes.c_float.from_address(self.address+36).value=request['ratio'] if request else 0
        self.words[10]=request['damage_ack'] if request else 0;self.words[11]=request['healing_ack'] if request else 0
        self.words[4]=(seq+1)&0xffffffff

    def status(self):
        if not self.available:return {'available':False}
        for _ in range(3):
            before=self.words[12]
            if before&1:continue
            values=list(self.words[13:25])
            if before!=self.words[12]:continue
            stamp,reason,active,generation,player,hp,maximum,damage,healing,dead,writes,mode=values
            if reason>5 or active>1 or dead>1 or hp>maximum or maximum>32767 or mode>2:raise ReaderError('Estado de vida no valido')
            return dict(available=True,active=bool(active),fresh=((self.k.GetTickCount()-stamp)&0xffffffff)<500,
                reason=reason,generation=generation,player=player,hp=hp,maximum=maximum,damage=damage,healing=healing,dead=bool(dead),writes=writes,mode=mode)
        return dict(available=True,active=False,fresh=False)

    def close(self):
        if self.available:self.publish(False,0)
        self.available=False
        if self.address:self.k.UnmapViewOfFile(self.address);self.address=None
        if self.handle:self.k.CloseHandle(self.handle);self.handle=None
