"""Bounded compressed Minecraft GUI/hand layer, separate from movement IPC."""
import base64,ctypes,functools,math,struct,zlib
from fake_re4_host import ProtocolError
from re4_reader import ReaderError
MAX_WIDTH,MAX_HEIGHT=1600,900
SIZE=128+MAX_WIDTH*MAX_HEIGHT*4
MAGIC=0x5245344f
@functools.lru_cache(maxsize=2)
def pixels(image,width,height):
    if type(width) is not int or type(height) is not int or not 32<=width<=MAX_WIDTH or not 32<=height<=MAX_HEIGHT or not isinstance(image,str) or len(image)>262144:raise ProtocolError('Invalid overlay dimensions')
    try:
        source=base64.b64decode(image,validate=True)
        if len(source)>196608:raise ValueError('compressed limit')
        expected=width*height*4;decoder=zlib.decompressobj();result=decoder.decompress(source,expected+1)
        if len(result)!=expected or not decoder.eof or decoder.unused_data or decoder.unconsumed_tail:raise ValueError('decompressed size')
        return result
    except (ValueError,zlib.error) as error:raise ProtocolError('Invalid overlay pixels') from error

def validate_overlay(value):
    if not isinstance(value,dict) or set(value)-{'image','width','height','menu','encoding','selection'} or not {'image','width','height','menu','encoding'}<=set(value) or value['encoding']!='zlib-argb' or type(value['menu']) is not bool:raise ProtocolError('Invalid overlay')
    data=pixels(value['image'],value['width'],value['height'])
    selection=value.get('selection')
    if selection is not None:
        if not isinstance(selection,list) or len(selection)!=10 or any(type(x) not in (int,float) or not math.isfinite(x) or abs(x)>20000000 for x in selection) or abs(selection[3])>math.pi or any(not 1<=selection[i+7]-selection[i+4]<=7500 for i in range(3)):raise ProtocolError('Invalid selection outline')
    return data

class NativeOverlay:
    supports_overlay=True
    def __init__(self,pid,kernel):
        self.k=kernel;self.address=0;self.handle=0;self.available=False;self.last=None;self.revision=0
        self.handle=self.k.OpenFileMappingW(6,False,f'Local\\RE4Bridge-OverlayCommand-{pid}')
        if not self.handle:return
        self.address=self.k.MapViewOfFile(self.handle,6,0,0,SIZE)
        if not self.address:self.close();raise ReaderError('No se pudo abrir overlay')
        if struct.unpack('<2I',ctypes.string_at(self.address,8))!=(MAGIC,1):self.close();raise ReaderError('Overlay incompatible')
        self.available=True
    def status(self):
        if not self.available:return {}
        w=struct.unpack('<4I',ctypes.string_at(self.address+40,16))
        return {'overlay_available':True,'overlay_viewport_width':w[0],'overlay_viewport_height':w[1],'native_hud_candidates':w[2],'native_hud_hidden':w[3]}
    def publish(self,enabled,generation=0,hud=None):
        data=validate_overlay(hud) if enabled else None
        if enabled and (not self.available or type(generation) is not int or not 1<=generation<=2147483647):raise ProtocolError('Overlay requires explicit arm and DLL0.4.33')
        if not self.available:return
        identity=(hud['image'],hud['width'],hud['height']) if enabled else None
        changed=identity!=self.last
        if changed:self.revision=(self.revision+1)%2147483647 or 1
        sequence=(ctypes.c_uint32.from_address(self.address+8).value+1)|1;ctypes.c_uint32.from_address(self.address+8).value=sequence&0xffffffff
        ctypes.memmove(self.address+12,struct.pack('<7I',self.k.GetTickCount(),int(bool(enabled)),generation if enabled else 0,hud['width'] if enabled else 0,hud['height'] if enabled else 0,int(hud['menu']) if enabled else 0,self.revision),28)
        sel=hud.get('selection') if enabled and not hud['menu'] else None
        ctypes.memmove(self.address+64,struct.pack('<I10f',int(sel is not None),*(sel or [0]*10)),44)
        if changed and enabled:ctypes.memmove(self.address+128,data,len(data))
        self.last=identity
        ctypes.c_uint32.from_address(self.address+8).value=(sequence+1)&0xffffffff
    def close(self):
        if self.available:self.publish(False)
        if self.address:self.k.UnmapViewOfFile(self.address);self.address=0
        if self.handle:self.k.CloseHandle(self.handle);self.handle=0
        self.available=False
