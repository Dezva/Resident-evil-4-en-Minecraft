"""Actual authenticated TCP sessions through main(), with simulated Windows readers."""
import contextlib,io,json,socket,tempfile,threading,unittest,uuid
from pathlib import Path
from unittest.mock import patch
import re4_live_host as host
from fake_re4_host import write_frame,read_frame

class HostRecoveryTests(unittest.TestCase):
 def test_unexpected_session_error_is_logged_disarmed_and_next_connection_succeeds(self):
  real_socket=socket.socket
  with real_socket() as probe:
   probe.bind(('127.0.0.1',0));port=probe.getsockname()[1]
  ready=threading.Event();stops=[];results=[];failures=[]
  class Server(real_socket):
   def __init__(self,*args,**kwargs):super().__init__(*args,**kwargs);self.count=0
   def listen(self,*args):super().listen(*args);ready.set()
   def accept(self):
    if self.count==2:raise KeyboardInterrupt()
    self.count+=1;return super().accept()
  class Native:
   available=True;base=0x400000;size=0x800000;k=None
   def __init__(self,*args):pass
   def read(self,*args):raise AssertionError('unexpected process read')
   def close(self):pass
   def status(self):return {'capabilities':31,'engine_supported':True}
  class Feed:
   def __init__(self,*args):pass
   def observe(self,*args):return {'terrain_state':'inactivo'}
   def close(self):pass
  class Policy:
   count=0
   def __init__(self,*args):self.number=Policy.count;Policy.count+=1
   def apply(self,frame,sample):
    if self.number==0:raise RuntimeError('simulated transient session failure')
    return {'armed':False,'reason':'reconnected'}
   def stop(self):stops.append(self.number)
  with tempfile.TemporaryDirectory() as d:
   root=Path(d);(root/'config').mkdir();token='a'*64
   (root/'config/re4bridge.json').write_text(json.dumps({'protocol':1,'port':port,'token':token}))
   with contextlib.ExitStack() as stack:
    stack.enter_context(patch.object(host,'__file__',str(root/'tools/re4_live_host.py')))
    stack.enter_context(patch('sys.argv',['host','--control','--game-dir',d]))
    for name in ('WindowsProcess','NativeControl','NativeRuntime','NativeCombat','NativeHealth','NativePreview','NativeOverlay','NativeWorldMesh'):stack.enter_context(patch.object(host,name,Native))
    stack.enter_context(patch.object(host,'ControlPolicy',Policy));stack.enter_context(patch.object(host,'TerrainFeed',Feed))
    stack.enter_context(patch.object(host,'find_pid',return_value=42));stack.enter_context(patch.object(host,'locate_player_slot',return_value=1))
    stack.enter_context(patch.object(host,'sample_player',return_value={'state':'player','position_raw':[0,0,0]}))
    stack.enter_context(patch.object(host,'read_capture',return_value={'hook_description':'mock','device_success':1}))
    stack.enter_context(patch.object(host.socket,'socket',Server))
    def run():
     try:results.append(host.main())
     except BaseException as error:failures.append(error)
    thread=threading.Thread(target=run);thread.start();self.assertTrue(ready.wait(3))
    for attempt in range(2):
     with real_socket() as client:
      client.settimeout(3);client.connect(('127.0.0.1',port));session=str(uuid.uuid4())
      with client.makefile('rwb') as stream:
       write_frame(stream,{'protocol':1,'type':'hello','session':session,'token':token})
       self.assertEqual(read_frame(stream)['mode'],'re4_control')
       write_frame(stream,{'protocol':1,'type':'re4_control','session':session,'seq':1})
       response=read_frame(stream)
       if attempt==0:self.assertIsNone(response)
       else:self.assertEqual(response['control_state']['reason'],'reconnected')
    thread.join(3);self.assertFalse(thread.is_alive());self.assertFalse(failures);self.assertEqual(results,[0]);self.assertEqual(stops,[0,1])
   log=(root/'logs/re4bridge-host.log').read_text();self.assertIn('RuntimeError: simulated transient session failure',log);self.assertNotIn(token,log)
