"""Local native-input IPC. Never writes process memory or automates desktop keys."""
import ctypes
from ctypes import wintypes as w
import os
import time
import math
from fake_re4_host import ProtocolError
from re4_reader import ReaderError
from re4_block_preview import validate_preview

MAGIC = 0x52453443


def validate_control(frame):
    validate_preview(frame.get('preview'))
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
        for name, limit in [("total_yaw", 10000000), ("pitch", 85), ("eye_height", 2500)]:
            value = look.get(name)
            if type(value) not in (int, float) or not math.isfinite(value) or abs(value) > limit:
                raise ProtocolError("Invalid look value")
        if abs(look["eye_height"]) < 40:
            raise ProtocolError("Invalid eye height")


class ControlPolicy:
    """New explicit arm generation required after a stop. Tested with fake IPC."""
    def __init__(self, writer, preview=None):
        self.writer = writer
        self.preview = preview
        self.generation = 0
        self.armed = False

    def apply(self, frame, sample):
        validate_control(frame)
        reason = "requiere otro arm"
        if not frame["armed"] or sample.get("state") != "player":
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
        if status["stop"] or not status["ticking"]:
            self.armed = False
            reason = "parada nativa" if status["stop"] else "modulo sin ticks recientes"
        look = frame.get("look")
        if look and look["enabled"] and not (status.get("capabilities", 0) & 1):
            self.armed = False
            reason = "DLL sin soporte de raton"
        if look and look["first_person"] and not (status.get("capabilities", 0) & 2):
            self.armed = False
            reason = "firma de camara no disponible"
        if frame["buttons"] & 64 and not (status.get("capabilities",0) & 4):
            self.armed = False
            reason = "DLL sin soporte de correr; instala 0.4.10"
        self.writer.publish(self.armed, frame["buttons"] if self.armed else 0,
                            look=look if self.armed else None, generation=frame["arm_id"])
        result = {"armed": self.armed, "stop": status["stop"], "hook_ticks": status["hook_ticks"],
                "requested_buttons": frame["buttons"], "native_buttons": status.get("native_buttons", 0),
                "reason": "activo" if self.armed else reason,
                "capabilities": status.get("capabilities", 0), "camera_active": status.get("camera_active", False),
                "look_packets": status.get("look_packets", 0), "camera_ticks": status.get("camera_ticks", 0)}
        if self.preview is not None:
            box=frame.get('preview')
            self.preview.publish(self.armed and box is not None,frame['arm_id'],box['bounds'] if box else None)
            result['preview_state']=self.preview.status()
        return result

    def stop(self):
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

    def publish(self, armed, buttons, look=None, generation=0):
        seq = (self.words[2] + 1) | 1
        self.words[2] = seq & 0xFFFFFFFF
        self.words[3] = self.k.GetTickCount()
        self.words[4] = int(armed)
        self.words[5] = buttons
        if self.version == 2:
            for offset, value in [(40, look["total_yaw"] if look else 0),
                                  (44, look["pitch"] if look else 0), (48, look["eye_height"] if look else 1600)]:
                ctypes.c_float.from_address(self.address + offset).value = value
            self.words[13] = (int(look["enabled"]) | (int(look["first_person"]) << 1)) if armed and look else 0
            self.words[14] = generation
        self.words[2] = (seq + 1) & 0xFFFFFFFF

    def close(self):
        self.publish(False, 0)
        self.k.UnmapViewOfFile(self.address)
        self.k.CloseHandle(self.handle)
