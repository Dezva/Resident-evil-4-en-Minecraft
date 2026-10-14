"""Visual-only box or bounded rigid grid, through a command mapping; no process-memory writes."""
import ctypes
import math
import base64
from fake_re4_host import ProtocolError
from re4_reader import ReaderError


MAX_BOXES=128

def validate_preview(preview):
    if preview is None: return
    if not isinstance(preview,dict): raise ProtocolError('Invalid block preview')
    scene='origin' in preview and {'origin','boxes'}<=set(preview) and set(preview)<={'origin','boxes','style','collisions','tiles','faces','collider_count'}
    if not scene and set(preview)!={'bounds'}: raise ProtocolError('Invalid block preview')
    boxes=preview['boxes'] if scene else [preview['bounds']]
    if not isinstance(boxes,list) or not 1<=len(boxes)<=MAX_BOXES: raise ProtocolError('Invalid scene size')
    if scene:
        if type(preview.get('style',0)) is not int or preview.get('style',0) not in (0,1,2,3):raise ProtocolError('Invalid scene style')
        if type(preview.get('collisions',False)) is not bool:raise ProtocolError('Invalid collision flag')
        if type(preview.get('collider_count',len(boxes))) is not int or not 0<=preview.get('collider_count',len(boxes))<=len(boxes):raise ProtocolError('Invalid collision prefix')
        tiles=preview.get('tiles',[]);faces=preview.get('faces',[])
        if preview.get('style',0)==3:
            if not isinstance(tiles,list) or not 1<=len(tiles)<=64 or not isinstance(faces,list) or len(faces)!=len(boxes):raise ProtocolError('Invalid material count')
            for tile in tiles:
                try:
                    if type(tile) is not str or len(tile)!=1368 or len(base64.b64decode(tile,validate=True))!=1024:raise ValueError()
                except (ValueError,TypeError):raise ProtocolError('Invalid texture tile')
            for group in faces:
                if not isinstance(group,list) or len(group)!=6:raise ProtocolError('Invalid face count')
                for face in group:
                    if not isinstance(face,list) or len(face)!=9 or type(face[0]) is not int or not 0<=face[0]<len(tiles):raise ProtocolError('Invalid face tile')
                    if any(type(v) not in (int,float) or not math.isfinite(v) or not 0<=v<=1 for v in face[1:]):raise ProtocolError('Invalid face UV')
        elif tiles or faces or 'tiles' in preview or 'faces' in preview:raise ProtocolError('Unexpected textures')
        origin=preview['origin']
        if not isinstance(origin,list) or len(origin)!=4 or any(type(x) not in (int,float)
                or not math.isfinite(x) or abs(x)> (math.pi if i==3 else 20000000) for i,x in enumerate(origin)):
            raise ProtocolError('Invalid scene origin')
    for bounds in boxes:
        if not isinstance(bounds,list) or len(bounds)!=6 or any(type(x) not in (int,float)
                or not math.isfinite(x) or abs(x)>20000000 for x in bounds):
            raise ProtocolError('Invalid preview coordinates')
        if any(not (1 if scene else 40)<=bounds[i+3]-bounds[i]<=(7500 if scene else 2500) for i in range(3)):
            raise ProtocolError('Invalid preview size')


class NativePreview:
    """Single-writer cube lease using the creation-owned renderer, never the retired IPC."""
    def __init__(self,pid,k=None,user=None):
        self.depth=None
        from re4_cube_test import CubeSession,read_cube_status,initial_snapshot
        self.session=None;self.key=None;self.baseline=None;self.generation=0;self.epoch_started=0
        self.requested=False;self.bounds=None;self.origin=None;self.box_count=0;self.style=0;self.scene_collisions=False;self.tile_count=0;self._collision_available=False
        kernel=k if k is not None else ctypes.WinDLL('kernel32',use_last_error=True)
        try:
            self.session=CubeSession(kernel,pid,user=user)
            if self.session.version not in (2,3,4,5,6):
                raise ReaderError('Bloques requieren DLL0.4.19/0.4.21/0.4.22/0.4.23; conserva los controles desactivados.')
            self.baseline=initial_snapshot(read_cube_status,kernel,pid)
            self.last_snapshot=self.baseline
            if (self.session.version,self.baseline['build']) not in ((2,419),(3,421),(4,422),(5,423),(6,424),(6,426),(6,427),(6,428),(6,429),(6,430),(6,432),(6,433),(6,434),(6,435),(6,436),(6,437),(6,438),(6,439),(6,441),(6,442),(6,443),(6,444)) or not self.baseline['live_devices']:
                raise ReaderError('Bloques requieren DLL0.4.19/0.4.21/0.4.22/0.4.23 y dispositivo grafico activo.')
            if self.session.version==6:
                from re4_cube_test import read_collision_status
                self.last_collision=initial_snapshot(read_collision_status,kernel,pid)
                self._collision_available=self.last_collision is not None
            if self.baseline['restore_failures'] or self.baseline['escaped_interfaces']:
                raise ReaderError('Dibujo bloqueado; reinicia RE4 y envia CAPTURE-RE4-GRAPHICS.')
            from re4_depth import NativeDepth
            if self.baseline['build']>=435:self.depth=NativeDepth(kernel,pid)
        except Exception:
            self.close();raise

    @property
    def scene_available(self):return self.session.version>=3

    @property
    def surface_available(self):return self.session.version>=4

    @property
    def texture_available(self):return self.session.version>=5

    @property
    def materials_available(self):return self.session.version==6

    @property
    def collision_available(self):return self._collision_available

    def publish(self,enabled,generation,bounds=None):
        scene=bounds if isinstance(bounds,dict) else None
        if enabled:
            validate_preview(scene if scene is not None else {'bounds':bounds})
            if scene is not None and not self.scene_available:raise ReaderError('Cuadricula requiere DLL0.4.21 o0.4.22.')
            if scene is not None and scene.get('style',0) and not self.surface_available:raise ReaderError('Caras rellenas requieren DLL0.4.22.')
            if scene is not None and scene.get('style',0)==2 and not self.texture_available:raise ReaderError('Textura requiere DLL0.4.23.')
            if scene is not None and (scene.get('style',0)==3 or scene.get('collisions',False)) and not self.materials_available:raise ReaderError('Materiales/colisiones requieren DLL0.4.24.')
            if scene is not None and scene.get('collisions',False) and not self.collision_available:raise ReaderError('Canal de colision no disponible; reinicia RE4.')
            if type(generation) is not int or not 1<=generation<=2147483647:
                raise ProtocolError('Bloque requiere arm explicito')
            if generation!=self.generation:
                from re4_cube_test import read_cube_status,SnapshotBusy
                try:self.baseline=read_cube_status(self.session.k,self.session.pid);self.last_snapshot=self.baseline
                except SnapshotBusy:self.baseline=self.last_snapshot
                self.generation=generation
            key=(generation,tuple(scene['origin']),tuple(tuple(box) for box in scene['boxes']),scene.get('style',0),scene.get('collisions',False),scene.get('collider_count',len(scene['boxes'])),tuple(scene.get('tiles',[])),tuple(tuple(tuple(f) for f in group) for group in scene.get('faces',[]))) if scene is not None else (generation,tuple(bounds))
            if key!=self.key:
                import secrets
                self.session.epoch=secrets.randbits(32) or 1
                self.key=key
                import time
                self.epoch_started=time.monotonic()
        # Retain the epoch through removal/disarm. A new arm or placement rearms it.
        self.session.publish(enabled,bounds if enabled and scene is None else None,scene=scene if enabled else None,generation=generation if enabled else 0)
        self.requested=bool(enabled)
        self.bounds=list(bounds) if enabled and scene is None else None
        self.origin=list(scene['origin']) if enabled and scene is not None else None
        self.box_count=len(scene['boxes']) if enabled and scene is not None else int(enabled)
        self.style=scene.get('style',0) if enabled and scene is not None else 0
        self.scene_collisions=bool(scene.get('collisions',False)) if enabled and scene is not None else False
        self.tile_count=len(scene.get('tiles',[])) if enabled and scene is not None else 0

    def status(self):
        from re4_cube_test import read_cube_status,SnapshotBusy
        stale=False
        try:snapshot=read_cube_status(self.session.k,self.session.pid);self.last_snapshot=snapshot
        except SnapshotBusy:snapshot=getattr(self,'last_snapshot',self.baseline);stale=True
        # Reset/Present may occur between attempts. An armed frame must not silently
        # reactivate after a new device loss; require another explicit arm.
        import time
        blocked=(not snapshot['live_devices'] or snapshot['restore_failures'] or snapshot['escaped_interfaces']
                 or (snapshot['stop_reason'] in (3,4,9) and time.monotonic()-self.epoch_started>.5)
                 or any(snapshot[field]>self.baseline[field] for field in
                        ('failures','present_failures','reset_calls','reset_failures')))
        collision=None
        if self.collision_available:
            from re4_cube_test import read_collision_status
            try:collision=read_collision_status(self.session.k,self.session.pid);self.last_collision=collision
            except SnapshotBusy:collision=getattr(self,'last_collision',None);stale=True
            if self.requested and self.scene_collisions and ((collision is None and not stale) or (collision is not None and collision['generation']==self.generation and collision['reason'] in (3,5))):blocked=True
        reply={'status':3 if not snapshot['live_devices'] else 2 if blocked else 1,
                'frames':snapshot['frames'],'draws':snapshot['draws'],
                'failures':snapshot['failures']+snapshot['restore_failures'],
                'last_hresult':int(snapshot['last_hresult'],16),
                'reason':snapshot['reason'],'blocked':bool(blocked),
                'stop_reason':snapshot['stop_reason'],'requested':self.requested,'bounds':self.bounds,
                'origin':self.origin,'box_count':self.box_count,'style':self.style,
                'collisions_requested':self.requested and self.scene_collisions,'tile_count':self.tile_count,
                'skipped_passes':snapshot['skipped_passes'],
                'camera_rejections':snapshot['camera_rejections'],'lease_rejections':snapshot['lease_rejections'],'telemetry_stale':stale}
        if collision is None and self.collision_available:collision=dict(ticks=0,contacts=0,corrections=0,rejections=0,reason=1,active=False,generation=0)
        if collision is not None:reply['collision_state']=collision
        if self.depth is not None:reply.update({key:value for key,value in self.depth.status().items() if key not in ('depth_projection','depth_gx_projection')})
        return reply

    def close(self):
        if self.depth is not None:self.depth.close();self.depth=None
        if self.session is not None:
            self.session.close();self.session=None
        self.requested=False;self.bounds=None;self.origin=None;self.box_count=0;self.style=0;self.scene_collisions=False;self.tile_count=0
