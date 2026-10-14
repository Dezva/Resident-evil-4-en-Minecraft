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
                              "capabilities": 31, "camera_active": False, "look_packets": 10, "camera_ticks": 50}


parser = argparse.ArgumentParser()
parser.add_argument("--config", type=Path, required=True)
parser.add_argument("--overlay",action="store_true")
args = parser.parse_args()
config = load_config(args.config)
class Preview:
    scene_available=True
    surface_available=True
    texture_available=True
    materials_available=True
    collision_available=True
    def __init__(self): self.enabled=False;self.count=0;self.style=0
    def publish(self,enabled,generation,bounds=None):
        self.enabled=enabled
        self.count=(len(bounds['boxes']) if isinstance(bounds,dict) else 1) if enabled else 0
        self.style=bounds.get('style',0) if enabled and isinstance(bounds,dict) else 0
        print(f'PREVIEW enabled={enabled} generation={generation} bounds={bounds}',flush=True)
    def status(self):return {'status':1,'frames':99,'draws':20,'failures':0,'last_hresult':0,
                             'requested':self.enabled,'skipped_passes':12,'reason':'pase omitido','box_count':self.count,'style':self.style,
                             'collision_state':dict(contacts=5,corrections=4,active=self.enabled,reason=0)}
class Hud:
    available=True
    supports_overlay=args.overlay
    def publish(self,enabled,generation=0,hud=None):
        print(f'HUD enabled={enabled} generation={generation} pixels={len(hud["image"]) if hud else 0}',flush=True)
        if hud and args.overlay:print(f'OVERLAY width={hud["width"]} menu={hud["menu"]}',flush=True)
class Health:
    def __init__(self):self.enabled=False;self.generation=0;self.request=None
    def publish(self,enabled,generation,request=None):
        self.enabled=enabled and request is not None;self.generation=generation;self.request=request
        if request:print(f"HEALTH mode={request['mode']} ratio={request['ratio']} damage_ack={request['damage_ack']} generation={generation}",flush=True)
    def status(self):
        return dict(available=True,active=self.enabled,fresh=True,reason=0,generation=self.generation,player=4096,hp=800,maximum=1000,damage=200000,healing=0,dead=False,mode=self.request['mode'] if self.request else 0)
class Runtime:
    def __init__(self):self.last=None;self.actions=0;self.skips=0;self.last_vertical=None
    def publish(self,generation,action_id,vertical=None,action_kind=0):
        if vertical and self.last_vertical!=vertical:self.last_vertical=vertical;print(f"VERTICAL generation={generation} token={vertical['token']} lift={vertical['lift']} feet_y={vertical['feet_y']} native_ground={vertical['native_ground']}",flush=True)
        if action_id and self.last!=(generation,action_id):
            self.last=(generation,action_id)
            if action_kind:self.skips+=1
            else:self.actions+=1
            print(f'ACTION generation={generation} id={action_id} kind={action_kind}',flush=True)
    def status(self):return dict(available=True,fresh=True,scene=0,flags=0,enemies_available=True,action_available=True,enemy_contacts=0,enemy_corrections=0,actions=self.actions,skips=self.skips)
class Combat:
    def __init__(self):self.enabled=False;self.ack=0;self.generation=0;self.hits=0
    def publish(self,enabled,generation,scale=1000,request=None):
        self.enabled=enabled;self.generation=generation
        if enabled and request and request['id']!=self.ack:
            self.ack=request['id'];self.hits+=1;print(f"MELEE generation={generation} id={self.ack} damage={request['damage']} slot={request['slot']}",flush=True)
    def status(self):return dict(available=True,active=self.enabled,fresh=True,generation=self.generation,ack=self.ack,reason=0,hp_before=1000,hp_after=950,hits=self.hits,kills=0,enemies=[])
policy = ControlPolicy(Writer(),Preview(),Hud(),Health(),Runtime(),Combat())
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
