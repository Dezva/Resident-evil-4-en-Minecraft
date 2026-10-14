"""Local native-input IPC. Never writes process memory or automates desktop keys."""
import ctypes
from ctypes import wintypes as w
import os
import time
import math
from fake_re4_host import ProtocolError
from re4_reader import ReaderError
from re4_block_preview import validate_preview
from re4_hud import validate_hud
from re4_health import validate_health

MAGIC = 0x52453443


def validate_control(frame):
    if type(frame.get('action_id',0)) is not int or not 0<=frame.get('action_id',0)<=2147483647 or frame.get('action_id',0) and (not frame.get('armed') or frame.get('menu',False)):raise ProtocolError('Invalid action request')
    if type(frame.get('action_kind',0)) is not int or frame.get('action_kind',0) not in (0,1) or frame.get('action_kind',0) and not frame.get('action_id',0):raise ProtocolError('Invalid action kind')
    vertical=frame.get('vertical')
    if vertical is not None:
        if not isinstance(vertical,dict) or set(vertical)!={'lift','scale','token','feet_y'} or type(vertical['token']) is not int or not 1<=vertical['token']<=2147483647:raise ProtocolError('Invalid vertical request')
        if any(type(vertical[k]) not in (int,float) or not math.isfinite(vertical[k]) for k in ('lift','scale','feet_y')) or abs(vertical['feet_y'])>20000000 or not 40<=vertical['scale']<=2500 or not 0<=vertical['lift']<=128*vertical['scale']:raise ProtocolError('Invalid vertical position')
        if not frame.get('armed'):raise ProtocolError('Vertical request requires arm')
    validate_health(frame.get('health'))
    if type(frame.get('menu',False)) is not bool or frame.get('menu',False) and frame.get('buttons')!=0:raise ProtocolError('Menu requires zero movement')
    if type(frame.get('depth',False)) is not bool:raise ProtocolError('Invalid depth mode')
    if type(frame.get('terrain',False)) is not bool or type(frame.get('terrain_revision',0)) is not int or not 0<=frame.get('terrain_revision',0)<=2147483647:raise ProtocolError('Invalid terrain request')
    validate_preview(frame.get('preview'))
    validate_hud(frame.get('hud'))
    if frame.get("type") != "re4_control" or type(frame.get("armed")) is not bool:
        raise ProtocolError("Invalid control frame")
    if type(frame.get("buttons")) is not int or frame["buttons"] < 0 or frame["buttons"] & ~79:
        raise ProtocolError("Only movement and run buttons are allowed")
    if type(frame.get("arm_id")) is not int or not 0 <= frame["arm_id"] <= 2147483647:
        raise ProtocolError("Invalid arm generation")
    if frame["armed"] and frame["arm_id"] == 0:
        raise ProtocolError("Explicit arm generation required")
    look = frame.get("look")
    if look is not None:
        if not isinstance(look, dict) or type(look.get("enabled")) is not bool or type(look.get("first_person")) is not bool:
            raise ProtocolError("Invalid look flags")
        if look["first_person"] and not look["enabled"]:
            raise ProtocolError("First person requires mouse control")
        absolute=look.get('absolute_yaw',False)
        if type(absolute) is not bool or absolute and (not look['first_person'] or not look['enabled']):
            raise ProtocolError('Invalid absolute gaze flags')
        if absolute and (type(look.get('total_yaw')) not in (int,float) or abs(look['total_yaw'])>180):
            raise ProtocolError('Invalid absolute gaze')
        for name, limit in [("total_yaw", 10000000), ("pitch", 90), ("eye_height", 2500)]:
            value = look.get(name)
            if type(value) not in (int, float) or not math.isfinite(value) or abs(value) > limit:
                raise ProtocolError("Invalid look value")
        if abs(look["eye_height"]) < 40:
            raise ProtocolError("Invalid eye height")


class ControlPolicy:
    """New explicit arm generation required after a stop. Tested with fake IPC."""
    def __init__(self, writer, preview=None, hud=None, health=None, runtime=None):
        self.writer = writer
        self.preview = preview
        self.hud = hud
        self.health = health
        self.runtime = runtime
        self.generation = 0
        self.armed = False

    def apply(self, frame, sample):
        validate_control(frame)
        activity=self.runtime.status() if self.runtime is not None else {}
        scripted=activity.get('fresh',False) and activity.get('scene')==1
        reason = "requiere otro arm"
        if not frame["armed"] or sample.get("state") != "player" and not (scripted and self.armed):
            reason = "cliente desarmado" if not frame["armed"] else "sin personaje RE4"
            self.armed = False
            self.writer.publish(False, 0)
        elif frame["arm_id"] != self.generation:
            if frame["arm_id"] <= self.generation:
                raise ProtocolError("Stale arm generation")
            self.generation = frame["arm_id"]
            self.writer.reset_stop()
            self.armed = True
        status = self.writer.status()
        if status["stop"] or not status["ticking"] and not scripted:
            self.armed = False
            reason = "parada nativa" if status["stop"] else "modulo sin ticks recientes"
        look = frame.get("look")
        if look and look["enabled"] and not (status.get("capabilities", 0) & 1):
            self.armed = False
            reason = "DLL sin soporte de raton"
        if look and look["first_person"] and not (status.get("capabilities", 0) & 2):
            self.armed = False
            reason = "firma de camara no disponible"
        if look and look.get('absolute_yaw') and not (status.get('capabilities',0)&8):
            self.armed=False
            reason='mirada sincronizada requiere DLL0.4.30'
        if frame["buttons"] & 64 and not (status.get("capabilities",0) & 4):
            self.armed = False
            reason = "DLL sin soporte de correr; instala 0.4.10"
        box=frame.get('preview')
        if box and box.get('style',0) and (self.preview is None or not getattr(self.preview,'surface_available',False)):
            self.armed=False
            reason='caras rellenas requieren DLL0.4.22; usa modo wire'
        if box and box.get('style',0)==2 and (self.preview is None or not getattr(self.preview,'texture_available',False)):
            self.armed=False
            reason='textura requiere DLL0.4.23; usa solid o wire'
        if box and (box.get('style',0)==3 or box.get('collisions',False)) and (self.preview is None or not getattr(self.preview,'materials_available',False)):
            self.armed=False
            reason='materiales/colisiones requieren DLL0.4.24'
        if box and box.get('collisions',False) and (self.preview is None or not getattr(self.preview,'collision_available',False)):
            self.armed=False
            reason='canal de colision no disponible; reinicia RE4'
        hud=frame.get('hud')
        if hud is not None and (self.hud is None or not self.hud.available or not look or not look.get('first_person') or (hud.get("encoding")=="zlib-argb" and not getattr(self.hud,"supports_overlay",False))):
            self.armed=False;reason='interfaz requiere DLL/mod0.4.33 y primera persona'
        if hud is not None and getattr(self.hud,"supports_overlay",False) and hud.get("encoding")!="zlib-argb":
            self.armed=False;reason='interfaz requiere mod Minecraft0.4.33'
        if hud is not None and hud.get("menu") is True and frame["buttons"]!=0:
            self.armed=False;reason='inventario requiere movimiento detenido'
        self.writer.publish(self.armed, frame["buttons"] if self.armed and not scripted else 0,
                            look=look if self.armed else None, generation=frame["arm_id"],menu=frame.get("menu",False))
        if self.hud is not None:self.hud.publish(self.armed and hud is not None,frame['arm_id'],hud if self.armed else None)
        result = {"hud_available":self.hud is not None and self.hud.available,"armed": self.armed, "stop": status["stop"], "hook_ticks": status["hook_ticks"],
                "requested_buttons": frame["buttons"], "native_buttons": status.get("native_buttons", 0),
                "reason": "activo" if self.armed else reason,
                "capabilities": status.get("capabilities", 0), "native_hud_supported": bool(status.get("capabilities", 0)&16), "camera_active": status.get("camera_active", False),
                "look_packets": status.get("look_packets", 0), "camera_ticks": status.get("camera_ticks", 0)}
        if self.preview is not None:
            result['preview_available']=True
            result['scene_available']=getattr(self.preview,'scene_available',False)
            result['surface_available']=getattr(self.preview,'surface_available',False)
            result['texture_available']=getattr(self.preview,'texture_available',False)
            result['materials_available']=getattr(self.preview,'materials_available',False)
            result['collision_available']=getattr(self.preview,'collision_available',False)
            box=frame.get('preview')
            self.preview.publish(self.armed and box is not None,frame['arm_id'],(box if 'origin' in box else box['bounds']) if box else None)
            depth=getattr(self.preview,'depth',None)
            if depth is not None:depth.publish(self.armed and bool(frame.get('depth')) and box is not None and box.get('style',0)>0,frame['arm_id'])
            result['preview_state']=self.preview.status()
            if box is not None and self.armed and result['preview_state'].get('blocked',False):
                self.armed=False
                self.writer.publish(False,0)
                self.preview.publish(False,frame['arm_id'])
                if depth is not None:depth.publish(False,0)
                if self.hud is not None:self.hud.publish(False)
                result['armed']=False
                result['reason']='dibujo bloqueado; repite arm o reinicia RE4: '+result['preview_state'].get('reason','')
                result['preview_state']=self.preview.status()
        if self.hud is not None and hasattr(self.hud,"status"):result.update(self.hud.status())
        if self.health is not None:
            self.health.publish(self.armed,frame['arm_id'],frame.get('health'))
            result['health_state']=self.health.status()
        if self.runtime is not None:
            self.runtime.publish(frame['arm_id'],frame.get('action_id',0) if self.armed else 0,frame.get('vertical') if self.armed and not scripted else None,frame.get('action_kind',0) if self.armed else 0)
            result['runtime_state']=activity
        return result

    def stop(self):
        if self.runtime is not None:self.runtime.publish(0,0)
        if self.health is not None:self.health.publish(False,0)
        if self.preview is not None and getattr(self.preview,'depth',None) is not None:self.preview.depth.publish(False,0)
        if self.hud is not None:self.hud.publish(False)
        self.armed = False
        self.writer.publish(False, 0)
        if self.preview is not None: self.preview.publish(False,0)


class NativeControl:
    def __init__(self, pid):
        if os.name != "nt":
            raise ReaderError("El control nativo requiere Windows")
        self.k = ctypes.WinDLL("kernel32", use_last_error=True)
        for name, args, result in [
            ("OpenFileMappingW", [w.DWORD, w.BOOL, w.LPCWSTR], w.HANDLE),
            ("MapViewOfFile", [w.HANDLE, w.DWORD, w.DWORD, w.DWORD, ctypes.c_size_t], ctypes.c_void_p),
            ("UnmapViewOfFile", [ctypes.c_void_p], w.BOOL), ("CloseHandle", [w.HANDLE], w.BOOL),
            ("GetTickCount", [], w.DWORD),
        ]:
            fn = getattr(self.k, name)
            fn.argtypes, fn.restype = args, result
        self.handle = self.k.OpenFileMappingW(6, False, f"Local\\RE4Bridge-Control-{pid}")
        if not self.handle:
            code = ctypes.get_last_error()
            hint = ("Acceso denegado: ejecuta el host con los mismos permisos que RE4."
                    if code == 5 else
                    "El canal no existe: ejecuta DIAGNOSE-RE4-CONTROLS.bat para comprobar la DLL cargada."
                    if code == 2 else
                    "Ejecuta DIAGNOSE-RE4-CONTROLS.bat y comunica el resultado.")
            raise ReaderError(f"OpenFileMapping fallo (Windows {code}, PID {pid}). {hint}")
        self.address = self.k.MapViewOfFile(self.handle, 6, 0, 0, 40)
        if not self.address:
            code = ctypes.get_last_error()
            self.k.CloseHandle(self.handle)
            raise ReaderError(f"MapViewOfFile fallo (Windows {code})")
        self.words = (ctypes.c_uint32 * 10).from_address(self.address)
        self.version = self.words[1]
        if self.version == 2:
            self.k.UnmapViewOfFile(self.address)
            self.address = self.k.MapViewOfFile(self.handle, 6, 0, 0, 80)
            if not self.address:
                code = ctypes.get_last_error()
                self.k.CloseHandle(self.handle)
                raise ReaderError(f"MapViewOfFile IPC v2 fallo (Windows {code})")
            self.words = (ctypes.c_uint32 * 20).from_address(self.address)
        self.last_ticks = self.words[8]
        self.last_tick_time = time.monotonic() - 1
        if self.words[0] != MAGIC or self.version not in (1,2) or self.words[7] != 1:
            code = self.words[7]
            self.k.UnmapViewOfFile(self.address)
            self.k.CloseHandle(self.handle)
            raise ReaderError(f"Modulo nativo incompatible/no inicializado (estado {code}); no se activara")
        self.publish(False, 0)

    def status(self):
        ticks = self.words[8]
        if ticks != self.last_ticks:
            self.last_ticks = ticks
            self.last_tick_time = time.monotonic()
        return {"stop": self.words[6], "hook_ticks": ticks,
                "native_buttons": self.words[9],
                "capabilities": self.words[15] if self.version == 2 else 0,
                "camera_active": bool(self.words[19] and self.words[4]) if self.version == 2 else False,
                "look_packets": self.words[18] if self.version == 2 else 0,
                "camera_ticks": self.words[17] if self.version == 2 else 0,
                "ticking": ticks != 0 and time.monotonic() - self.last_tick_time < 0.5}

    def reset_stop(self):
        self.words[6] = 0

    def publish(self, armed, buttons, look=None, generation=0, menu=False):
        seq = (self.words[2] + 1) | 1
        self.words[2] = seq & 0xFFFFFFFF
        self.words[3] = self.k.GetTickCount()
        self.words[4] = int(armed)
        self.words[5] = buttons
        if self.version == 2:
            for offset, value in [(40, look["total_yaw"] if look else 0),
                                  (44, look["pitch"] if look else 0), (48, look["eye_height"] if look else 1600)]:
                ctypes.c_float.from_address(self.address + offset).value = value
            self.words[13] = (int(look["enabled"]) | (int(look["first_person"]) << 1) | (int(look.get("absolute_yaw",False)) << 2) | (int(menu) << 3)) if armed and look else 0
            self.words[14] = generation
        self.words[2] = (seq + 1) & 0xFFFFFFFF

    def close(self):
        self.publish(False, 0)
        self.k.UnmapViewOfFile(self.address)
        self.k.CloseHandle(self.handle)
