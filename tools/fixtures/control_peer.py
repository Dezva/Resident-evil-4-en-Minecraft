"""TEST ONLY: fake native control over real authenticated TCP, without Windows/game."""
import argparse
from pathlib import Path
import socket
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from fake_re4_host import load_config, serve_peer
from re4_control import ControlPolicy


class Writer:
    def __init__(self): self.armed = False; self.buttons = 0
    def publish(self, armed, buttons, **kwargs):
        self.armed = armed
        self.buttons = buttons if armed else 0
        print(f"INPUT armed={armed} buttons={buttons}", flush=True)
        if kwargs.get('look'): print('LOOK ' + str(kwargs['look']), flush=True)
    def reset_stop(self): pass
    def status(self): return {"stop": 0, "ticking": True, "hook_ticks": 123, "native_buttons": self.buttons,
                              "capabilities": 7, "camera_active": False, "look_packets": 10, "camera_ticks": 50}


parser = argparse.ArgumentParser()
parser.add_argument("--config", type=Path, required=True)
args = parser.parse_args()
config = load_config(args.config)
class Preview:
    scene_available=True
    surface_available=True
    def __init__(self): self.enabled=False;self.count=0;self.style=0
    def publish(self,enabled,generation,bounds=None):
        self.enabled=enabled
        self.count=(len(bounds['boxes']) if isinstance(bounds,dict) else 1) if enabled else 0
        self.style=bounds.get('style',0) if enabled and isinstance(bounds,dict) else 0
        print(f'PREVIEW enabled={enabled} generation={generation} bounds={bounds}',flush=True)
    def status(self):return {'status':1,'frames':99,'draws':20,'failures':0,'last_hresult':0,
                             'requested':self.enabled,'skipped_passes':12,'reason':'pase omitido','box_count':self.count,'style':self.style}
policy = ControlPolicy(Writer(),Preview())
with socket.socket() as server:
    server.bind(("127.0.0.1", config["port"]))
    server.listen(1)
    print("READY synthetic control fixture", flush=True)
    peer, _ = server.accept()
    with peer:
        try:
            serve_peer(peer, config["token"], {},
                lambda: {"state":"player", "position_raw":[1,2,3], "angles_raw":[0,0,0]}, policy.apply)
        finally: policy.stop()
