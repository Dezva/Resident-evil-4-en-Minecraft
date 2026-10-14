"""Separate leased depth-mode command and readonly rendering telemetry."""
import ctypes,math,struct
from re4_reader import ReaderError

REASONS={0:'profundidad activa',1:'desactivada',2:'proyeccion no validada',3:'depth buffer ausente',8:'generacion distinta'}
class NativeDepth:
    def __init__(self,k,pid,readonly=False):
        self.k=k;self.pid=pid;self.readonly=readonly;self.handle=self.address=0;self.available=False;self.last={}
        self.handle=k.OpenFileMappingW(4 if readonly else 6,False,f'Local\\RE4Bridge-Depth-{pid}')
        if not self.handle:return
        self.address=k.MapViewOfFile(self.handle,4 if readonly else 6,0,0,256)
        if not self.address:self.close();raise ReaderError('Canal de profundidad ausente')
        header=struct.unpack('<5I',ctypes.string_at(self.address,20))
        if header[:2]!=(0x52453444,1) or header[3] not in (435,436,437,438,439,441,442,443,444,445,446,447,448,449,450) or header[4]!=pid:self.close();raise ReaderError('Canal de profundidad incompatible')
        self.available=True
    def publish(self,enabled,generation):
        if not self.available or self.readonly:return
        seq=(ctypes.c_uint32.from_address(self.address+208).value+1)|1;ctypes.c_uint32.from_address(self.address+208).value=seq&0xffffffff
        ctypes.memmove(self.address+212,struct.pack('<3I',self.k.GetTickCount(),int(bool(enabled)),generation if enabled else 0),12)
        ctypes.c_uint32.from_address(self.address+208).value=(seq+1)&0xffffffff
    def status(self):
        if not self.available:return {'depth_available':False}
        for _ in range(8):
            seq=ctypes.c_uint32.from_address(self.address+8).value
            if seq&1:continue
            data=ctypes.string_at(self.address,208)
            if seq!=ctypes.c_uint32.from_address(self.address+8).value:continue
            words=struct.unpack_from('<16I',data);floats=struct.unpack_from('<2f',data,44)
            self.last={'depth_available':True,'depth_reason':REASONS.get(words[5],'sin muestra de profundidad'), 'depth_reason_code':words[5],'depth_attempts':words[6],'depth_draws':words[7],'depth_stage':words[8],'depth_width':words[9],'depth_height':words[10], 'depth_near':floats[0], 'depth_far':floats[1],'depth_reverse':bool(words[13]),'depth_source':words[14],'depth_fallback_draws':words[15]}
            self.last.update(depth_projection=list(struct.unpack_from('<16f',data,64)),depth_gx_projection=list(struct.unpack_from('<16f',data,128)),depth_fov=struct.unpack_from('<f',data,192)[0],depth_compare=struct.unpack_from('<I',data,196)[0],depth_world_draws=struct.unpack_from('<I',data,200)[0])
            if any(not math.isfinite(v) for key in ('depth_projection','depth_gx_projection') for v in self.last[key]) or not math.isfinite(self.last['depth_fov']) or not all(map(math.isfinite,floats)):raise ReaderError('Telemetria de profundidad no finita')
            return self.last
        return dict(self.last or {'depth_available':True,'depth_reason':'telemetria cambiando'})
    def close(self):
        if self.available:self.publish(False,0)
        if self.address:self.k.UnmapViewOfFile(self.address);self.address=0
        if self.handle:self.k.CloseHandle(self.handle);self.handle=0
        self.available=False
