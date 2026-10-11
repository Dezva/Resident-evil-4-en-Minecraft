"""One visual-only box, through a separate named mapping; no process-memory writes."""
import ctypes
import math
from fake_re4_host import ProtocolError
from re4_reader import ReaderError


def validate_preview(preview):
    if preview is None: return
    if not isinstance(preview,dict) or set(preview)!={'bounds'}:
        raise ProtocolError('Invalid block preview')
    bounds=preview['bounds']
    if not isinstance(bounds,list) or len(bounds)!=6 or any(type(x) not in (int,float)
            or not math.isfinite(x) or abs(x)>20000000 for x in bounds):
        raise ProtocolError('Invalid preview coordinates')
    if any(not 40<=bounds[i+3]-bounds[i]<=2500 for i in range(3)):
        raise ProtocolError('Invalid preview size')


class NativePreview:
    """Single-writer cube lease using the creation-owned renderer, never the retired IPC."""
    def __init__(self,pid,k=None,user=None):
        from re4_cube_test import CubeSession,read_cube_status
        self.session=None;self.key=None;self.baseline=None;self.generation=0;self.epoch_started=0
        self.requested=False;self.bounds=None
        kernel=k if k is not None else ctypes.WinDLL('kernel32',use_last_error=True)
        try:
            self.session=CubeSession(kernel,pid,user=user)
            if self.session.version!=2:
                raise ReaderError('Bloques requieren DLL0.4.19; conserva los controles desactivados.')
            self.baseline=read_cube_status(kernel,pid)
            if self.baseline['build']!=419 or not self.baseline['live_devices']:
                raise ReaderError('Bloques requieren DLL0.4.19 y dispositivo grafico activo.')
            if self.baseline['restore_failures'] or self.baseline['escaped_interfaces']:
                raise ReaderError('Dibujo bloqueado; reinicia RE4 y envia CAPTURE-RE4-GRAPHICS.')
        except Exception:
            self.close();raise

    def publish(self,enabled,generation,bounds=None):
        if enabled:
            validate_preview({'bounds':bounds})
            if type(generation) is not int or generation<=0:
                raise ProtocolError('Bloque requiere arm explicito')
            if generation!=self.generation:
                from re4_cube_test import read_cube_status
                self.baseline=read_cube_status(self.session.k,self.session.pid)
                self.generation=generation
            key=(generation,tuple(bounds))
            if key!=self.key:
                import secrets
                self.session.epoch=secrets.randbits(32) or 1
                self.key=key
                import time
                self.epoch_started=time.monotonic()
        # Retain the epoch through removal/disarm. A new arm or placement rearms it.
        self.session.publish(enabled,bounds if enabled else None)
        self.requested=bool(enabled)
        self.bounds=list(bounds) if enabled else None

    def status(self):
        from re4_cube_test import read_cube_status
        snapshot=read_cube_status(self.session.k,self.session.pid)
        # Reset/Present may occur between attempts. An armed frame must not silently
        # reactivate after a new device loss; require another explicit arm.
        import time
        blocked=(not snapshot['live_devices'] or snapshot['restore_failures'] or snapshot['escaped_interfaces']
                 or (snapshot['stop_reason'] in (3,4,9) and time.monotonic()-self.epoch_started>.5)
                 or any(snapshot[field]>self.baseline[field] for field in
                        ('failures','present_failures','reset_calls','reset_failures')))
        return {'status':3 if not snapshot['live_devices'] else 2 if blocked else 1,
                'frames':snapshot['frames'],'draws':snapshot['draws'],
                'failures':snapshot['failures']+snapshot['restore_failures'],
                'last_hresult':int(snapshot['last_hresult'],16),
                'reason':snapshot['reason'],'blocked':bool(blocked),
                'stop_reason':snapshot['stop_reason'],'requested':self.requested,'bounds':self.bounds,
                'skipped_passes':snapshot['skipped_passes'],
                'camera_rejections':snapshot['camera_rejections'],'lease_rejections':snapshot['lease_rejections']}

    def close(self):
        if self.session is not None:
            self.session.close();self.session=None
        self.requested=False;self.bounds=None
