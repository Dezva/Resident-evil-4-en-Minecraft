"""Bounded read-only terrain snapshots. Inactive until an authenticated FP client asks.
No process writes, native calls, world import, or reads on the input/network thread.
"""
import threading,time
from re4_map_export import extract
from re4_reader import ReaderError

MAX_TRIANGLES=1024
class TerrainFeed:
    def __init__(self,read,base,size,extractor=extract,clock=time.monotonic,radius=12000):
        self.read=read;self.base=base;self.size=size;self.extractor=extractor;self.clock=clock
        self.lock=threading.Lock();self.closed=threading.Event();self.wake=threading.Event();self.thread=None
        self.request=None;self.latest=None;self.revision=0;self.cache={};self.radius=radius
    def observe(self,frame,sample,state):
        requested=frame.get('terrain',False) and state['armed'] and bool(frame.get('look',{}).get('first_person')) and sample.get('state')=='player'
        with self.lock:
            position=list(sample.get('position_raw',()))
            if requested and frame.get('vertical') is not None:position[1]=frame['vertical']['feet_y']
            self.request=(tuple(position),frame['arm_id']) if requested else None
            latest=self.latest
            if requested and self.thread is None:
                self.thread=threading.Thread(target=self.run,name='re4bridge-terrain',daemon=True);self.thread.start()
        if requested:self.wake.set()
        if not requested:return {'terrain_state':'inactivo'}
        if latest is None or latest[0]!=frame['arm_id']:return {'terrain_state':'cargando'}
        generation,stamp,data=latest;age=self.clock()-stamp
        if age>1:return {'terrain_state':'caducado'}
        result={'terrain_state':'disponible' if data['complete'] else data['reason']}
        if frame.get('terrain_revision',0)!=data['revision']:result['terrain']=dict(data,age_ms=max(0,int(age*1000)),generation=generation)
        return result
    def capture(self,request):
        position,generation=request;started=self.clock();radius=self.radius
        try:
            report=self.extractor(self.read,self.base,self.size,position,radius=radius,manager_cache=self.cache)
            rows=report['triangles'];complete=report['complete'] and len(rows)<=MAX_TRIANGLES
            triangles=[[*p[0],*p[1],*p[2]] for row in rows for p in [row['vertices']]] if complete else []
            kinds=[row.get('kind','other') for row in rows] if complete else []
            reason='disponible' if complete else 'mapa incompleto o excede limite local'
        except (ReaderError,ValueError,OSError) as error:complete=False;triangles=[];kinds=[];reason='lectura de mapa no disponible'
        with self.lock:
            if self.request is None or self.request[1]!=generation or max(abs(self.request[0][i]-position[i]) for i in range(3))>3000 or self.closed.is_set() or radius!=self.radius:return
            self.revision=self.revision%2147483647+1
            self.latest=(generation,started,dict(revision=self.revision,complete=complete,reason=reason,position=list(position),radius=radius,triangles=triangles,kinds=kinds))
    def run(self):
        next_capture=0
        while not self.closed.is_set():
            self.wake.wait(.1);self.wake.clear()
            with self.lock:request=self.request
            now=self.clock()
            if request is not None and now>=next_capture:
                self.capture(request);next_capture=self.clock()+.5
    def close(self):
        self.closed.set();self.wake.set()
        if self.thread:self.thread.join(.2)

class CombatTerrainFeed(TerrainFeed):
    """Independent short-range snapshot; an oversized building map never authorizes a blind hit."""
    def observe(self,frame,sample,state):
        self.radius=(frame.get('combat_reach',3)+1)*frame.get('combat_scale',1000)+2500
        translated=dict(frame,terrain=frame.get('combat_enabled',False),terrain_revision=frame.get('combat_terrain_revision',0))
        result=super().observe(translated,sample,state)
        return {('combat_terrain' if key=='terrain' else 'combat_terrain_state'):value for key,value in result.items()}
