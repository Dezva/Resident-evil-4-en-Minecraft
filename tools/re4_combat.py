"""Bounded melee commands and native actor snapshots. All game writes run in the DLL."""
import ctypes,math,struct
from ctypes import wintypes as w
from fake_re4_host import ProtocolError
from re4_reader import ReaderError
# Stable native ABI identifier (RE4F), independent of the release number.
MAGIC=0x52453446
SIZE=5332

def validate_combat(frame):
    enabled=frame.get('combat_enabled',False)
    if type(enabled) is not bool:raise ProtocolError('Invalid combat mode')
    scale=frame.get('combat_scale',1000)
    if type(scale) not in (int,float) or not math.isfinite(scale) or not 40<=scale<=2500:raise ProtocolError('Invalid combat scale')
    reach=frame.get('combat_reach',3)
    if type(reach) not in (int,float) or not math.isfinite(reach) or not 0<reach<=6:raise ProtocolError('Invalid combat reach')
    attack=frame.get('combat')
    if attack is None:return
    if not enabled or not frame.get('armed') or frame.get('menu',False) or not frame.get('look',{}).get('first_person'):raise ProtocolError('Melee requires armed first person gameplay')
    keys={'id','slot','guid','damage','scale','reach','start','end'}
    if not isinstance(attack,dict) or set(attack) not in (keys,keys|{'kind','projectile'}):raise ProtocolError('Invalid combat request')
    kind=attack.get('kind',0);projectile=attack.get('projectile',0)
    if type(kind) is not int or kind not in (0,1) or type(projectile) is not int or not 0<=projectile<=2147483647 or (kind==0 and projectile!=0) or (kind==1 and projectile==0):raise ProtocolError('Invalid projectile identity')
    for key,low,high in [('id',1,2147483647),('slot',0,4095),('guid',0,0xffffffff),('damage',1,50000)]:
        if type(attack[key]) is not int or not low<=attack[key]<=high:raise ProtocolError('Invalid melee '+key)
    for key,low,high in [('scale',40,2500),('reach',.000001,6)]:
        if type(attack[key]) not in (int,float) or not math.isfinite(attack[key]) or not low<=attack[key]<=high:raise ProtocolError('Invalid melee '+key)
    if abs(attack['scale']-scale)>.01:raise ProtocolError('Melee grid mismatch')
    for key in ('start','end'):
        value=attack[key]
        if not isinstance(value,list) or len(value)!=3 or any(type(n) not in (int,float) or not math.isfinite(n) or abs(n)>20000000 for n in value):raise ProtocolError('Invalid melee ray')
    distance=sum((attack['end'][i]-attack['start'][i])**2 for i in range(3))
    if not .000001<distance<=(attack['scale']*attack['reach']+1)**2:raise ProtocolError('Invalid melee reach')

class NativeCombat:
    def __init__(self,pid,k):
        self.k=k;self.handle=None;self.address=None;self.available=False
        k.OpenFileMappingW.argtypes=[w.DWORD,w.BOOL,w.LPCWSTR];k.OpenFileMappingW.restype=w.HANDLE
        k.MapViewOfFile.argtypes=[w.HANDLE,w.DWORD,w.DWORD,w.DWORD,ctypes.c_size_t];k.MapViewOfFile.restype=ctypes.c_void_p
        self.handle=k.OpenFileMappingW(6,False,f'Local\\RE4Bridge-Combat-{pid}')
        if not self.handle:
            error=ctypes.get_last_error()
            if error==2:return
            raise ReaderError(f'Canal de combate no disponible (Windows {error})')
        self.address=k.MapViewOfFile(self.handle,6,0,0,SIZE)
        if not self.address:self.close();raise ReaderError('No se pudo mapear combate')
        self.words=(ctypes.c_uint32*(SIZE//4)).from_address(self.address)
        header=tuple(self.words[:4])
        if header not in ((MAGIC,4,456,pid),(MAGIC,4,457,pid)):
            self.close()
            raise ReaderError(f'Combate requiere DLL0.4.56/0.4.57/IPC v4 para PID {pid}; recibido magic=0x{header[0]:08x}, version={header[1]}, build={header[2]}, PID={header[3]}')
        self.available=True;self.publish(False,0)
    def publish(self,enabled,generation,scale=1000,request=None):
        if not self.available:return
        seq=(self.words[4]+1)|1;self.words[4]=seq&0xffffffff
        self.words[5]=self.k.GetTickCount();self.words[6]=int(enabled);self.words[7]=generation
        self.words[8]=request['id'] if request else 0;self.words[9]=request['damage'] if request else 0
        ctypes.c_float.from_address(self.address+40).value=scale
        ctypes.c_float.from_address(self.address+44).value=request['reach'] if request else 0
        self.words[12]=request['slot'] if request else 0;self.words[13]=request['guid'] if request else 0
        for offset,key in [(56,'start'),(68,'end')]:
            for i in range(3):ctypes.c_float.from_address(self.address+offset+4*i).value=request[key][i] if request else 0
        self.words[1331]=request.get('kind',0) if request else 0
        self.words[1332]=request.get('projectile',0) if request else 0
        self.words[4]=(seq+1)&0xffffffff
    def status(self):
        if not self.available:return dict(available=False)
        for _ in range(3):
            before=self.words[20]
            if before&1:continue
            data=ctypes.string_at(self.address+84,SIZE-84)
            if before!=self.words[20]:continue
            stamp,generation,ack,reason,guid,slot,hp_before,hp_after,hits,kills,attempts,active,count=struct.unpack_from('<13I',data)
            if generation>2147483647 or ack>2147483647 or reason>9 or active>1 or count>128 or hp_after>hp_before or hp_before>32767:raise ReaderError('Estado de combate no valido')
            engine,queued,expired,part,profile,pending,xh,yh,zh,target_kind=struct.unpack_from('<6I3fI',data,5256-84)
            if engine>1 or part>255 or target_kind>255 or profile>9 or pending>2147483647 or any(not math.isfinite(n) or abs(n)>20000000 for n in (xh,yh,zh)):raise ReaderError('Reaccion de combate no valida')
            seen_callbacks,owned_callbacks,thread_changes,native_flags,message,foreign_callbacks,boxes_broken=struct.unpack_from('<7I',data,5296-84)
            actors=[];seen=set()
            for i in range(count if active else 0):
                slot_id,identity,kind,hp,maximum,x,y,z,radius,height=struct.unpack_from('<5I5f',data,52+40*i)
                if slot_id>4095 or slot_id in seen or kind>255 or not 0<hp<=maximum<=32767 or not 100<=radius<=1000 or not 300<=height<=3000 or any(not math.isfinite(n) or abs(n)>20000000 for n in (x,y,z)):raise ReaderError('Ganado de combate no valido')
                seen.add(slot_id);actors.append(dict(slot=slot_id,guid=identity,kind=kind,hp=hp,maximum=maximum,position=[x,y,z],radius=radius,height=height))
            return dict(available=True,active=bool(active),fresh=((self.k.GetTickCount()-stamp)&0xffffffff)<500,generation=generation,ack=ack,reason=reason,target_slot=slot,target_guid=guid,hp_before=hp_before,hp_after=hp_after,hits=hits,kills=kills,attempts=attempts,enemies=actors,engine_supported=bool(engine),profile_reason=profile,pending=pending,queued=queued,expired=expired,hit_part=part,hit_position=[xh,yh,zh],target_kind=target_kind,callback_seen=seen_callbacks,callback_owned=owned_callbacks,callback_thread_changed=thread_changes,last_native_flags=native_flags,last_message=message,callback_foreign=foreign_callbacks,boxes_broken=boxes_broken)
        return dict(available=True,active=False,fresh=False,generation=0,ack=0,reason=1,hp_before=0,hp_after=0,hits=0,kills=0,enemies=[])
    def close(self):
        if self.available:self.publish(False,0)
        self.available=False
        if self.address:self.k.UnmapViewOfFile(self.address);self.address=None
        if self.handle:self.k.CloseHandle(self.handle);self.handle=None
