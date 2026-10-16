"""Leased native world-space visuals; never mixed into the always-visible HUD."""
import base64,ctypes,math,struct,zlib
from fake_re4_host import ProtocolError
from re4_reader import ReaderError

MAGIC=0x52453457
MAX_QUADS=128
PIXEL_BYTES=128*128*4
SIZE=64+MAX_QUADS*4*24+PIXEL_BYTES

def decode_world_mesh(value):
 if value is None:return None
 if not isinstance(value,dict) or set(value)!={'encoding','vertices','texture'} or value['encoding']!='world-quads-v1':raise ProtocolError('Invalid world mesh')
 try:
  if type(value['vertices']) is not str or len(value['vertices'])>16384 or type(value['texture']) is not str or len(value['texture'])>43692:raise ValueError()
  vertices=base64.b64decode(value['vertices'],validate=True);compressed=base64.b64decode(value['texture'],validate=True)
  if len(compressed)>32768:raise ValueError()
  if not vertices or len(vertices)%96 or len(vertices)>MAX_QUADS*96:raise ValueError()
  for x,y,z,u,v,color in struct.iter_unpack('<5fI',vertices):
   if not all(math.isfinite(n) for n in (x,y,z,u,v)) or max(abs(x),abs(y),abs(z))>20000000 or not 0<=u<=1 or not 0<=v<=1:raise ValueError()
  decoder=zlib.decompressobj();pixels=decoder.decompress(compressed,PIXEL_BYTES+1)
  if len(pixels)!=PIXEL_BYTES or not decoder.eof or decoder.unused_data or decoder.unconsumed_tail:raise ValueError()
  return vertices,pixels
 except (ValueError,TypeError,zlib.error,struct.error):raise ProtocolError('Invalid world mesh data')

class NativeWorldMesh:
 def __init__(self,k,pid):
  self.k=k;self.handle=self.address=0;self.available=False
  self.handle=k.OpenFileMappingW(6,False,f'Local\\RE4Bridge-WorldMesh-{pid}')
  if not self.handle:raise ReaderError('Flechas 3D requieren DLL0.4.57; actualiza el paquete completo')
  self.address=k.MapViewOfFile(self.handle,6,0,0,SIZE)
  if not self.address:self.close();raise ReaderError('No se pudo mapear el canal de flechas 3D')
  if struct.unpack('<4I',ctypes.string_at(self.address,16))!=(MAGIC,1,457,pid):self.close();raise ReaderError('Canal de flechas 3D incompatible; requiere DLL0.4.57')
  self.available=True;self.publish(False,0,None)
 def publish(self,enabled,generation,decoded):
  if not self.available:return
  sequence=ctypes.c_uint32.from_address(self.address+16);seq=(sequence.value+1)|1;sequence.value=seq&0xffffffff
  vertices,pixels=decoded if decoded is not None else (b'',b'')
  enabled=bool(enabled and decoded is not None)
  ctypes.memmove(self.address+20,struct.pack('<4I',self.k.GetTickCount(),int(enabled),generation if enabled else 0,len(vertices)//96 if enabled else 0),16)
  if enabled:
   ctypes.memmove(self.address+64,vertices,len(vertices));ctypes.memmove(self.address+64+MAX_QUADS*96,pixels,PIXEL_BYTES)
  sequence.value=(seq+1)&0xffffffff
 def close(self):
  if self.available:self.publish(False,0,None)
  self.available=False
  if self.address:self.k.UnmapViewOfFile(self.address);self.address=0
  if self.handle:self.k.CloseHandle(self.handle);self.handle=0
