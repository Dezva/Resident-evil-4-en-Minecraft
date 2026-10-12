"""Bounded HUD pixels over a separate lease; no desktop capture/input or game assets."""
import base64
import ctypes
import struct
from fake_re4_host import ProtocolError
from re4_reader import ReaderError

SIZE=32792

def validate_hud(value):
    if value is None:return None
    if not isinstance(value,dict) or set(value)!={'image'} or not isinstance(value['image'],str) or len(value['image'])!=43692:raise ProtocolError('Invalid HUD')
    try:data=base64.b64decode(value['image'],validate=True)
    except ValueError as error:raise ProtocolError('Invalid HUD encoding') from error
    if len(data)!=32768:raise ProtocolError('Invalid HUD dimensions')
    return data

class NativeHud:
    def __init__(self,pid,kernel):
        self.k=kernel;self.address=0;self.handle=0;self.available=False
        self.handle=self.k.OpenFileMappingW(6,False,f'Local\\RE4Bridge-HudCommand-{pid}')
        if not self.handle:return
        self.address=self.k.MapViewOfFile(self.handle,6,0,0,SIZE)
        if not self.address:self.close();raise ReaderError('No se pudo abrir HUD')
        if struct.unpack('<2I',ctypes.string_at(self.address,8))!=(0x52453448,1):self.close();raise ReaderError('HUD incompatible')
        self.available=True
    def publish(self,enabled,generation=0,hud=None):
        data=validate_hud(hud)
        if enabled and (data is None or type(generation) is not int or not 1<=generation<=2147483647):raise ProtocolError('HUD requires explicit arm')
        if not self.available:
            if enabled:raise ReaderError('HUD requiere DLL 0.4.26')
            return
        sequence=(ctypes.c_uint32.from_address(self.address+8).value+1)|1
        ctypes.c_uint32.from_address(self.address+8).value=sequence&0xffffffff
        header=struct.pack('<3I',self.k.GetTickCount(),int(bool(enabled)),generation if enabled else 0)
        ctypes.memmove(self.address+12,header,12)
        ctypes.memmove(self.address+24,data if enabled else bytes(32768),32768)
        ctypes.c_uint32.from_address(self.address+8).value=(sequence+1)&0xffffffff
    def close(self):
        if self.available:self.publish(False)
        if self.address:self.k.UnmapViewOfFile(self.address);self.address=0
        if self.handle:self.k.CloseHandle(self.handle);self.handle=0
        self.available=False
