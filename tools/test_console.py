"""A selected/blocked Windows console must not stall authenticated game control."""
import socket,threading,time,unittest,uuid
from re4_console import ConsoleReporter
from fake_re4_host import serve_peer,write_frame,read_frame

class ConsoleTests(unittest.TestCase):
 def test_blocked_output_is_bounded_and_does_not_block_real_tcp_acknowledgements(self):
  entered=threading.Event();release=threading.Event();errors=[]
  def blocked(message):entered.set();release.wait(3)
  output=ConsoleReporter(blocked);output.emit('selected console')
  self.assertTrue(entered.wait(1))
  server,client=socket.socketpair();client.settimeout(1)
  def run():
   try:
    with server:serve_peer(server,'a'*64,{},lambda:dict(state='player',position_raw=[0,0,0]),lambda frame,sample:dict(armed=True),output.emit)
   except BaseException as error:errors.append(error)
  thread=threading.Thread(target=run);thread.start()
  try:
   started=time.monotonic()
   for i in range(5000):output.emit(str(i))
   self.assertLess(time.monotonic()-started,.5);self.assertLessEqual(output.pending.qsize(),32);self.assertGreater(output.dropped,0)
   with client.makefile('rwb') as stream:
    session=str(uuid.uuid4());write_frame(stream,dict(protocol=1,type='hello',session=session,token='a'*64));self.assertEqual(read_frame(stream)['mode'],'re4_control')
    for sequence in range(1,21):
     write_frame(stream,dict(protocol=1,type='re4_control',session=session,seq=sequence));self.assertEqual(read_frame(stream)['seq'],sequence)
   client.close();thread.join(1);self.assertFalse(thread.is_alive());self.assertFalse(errors)
   started=time.monotonic();output.close();self.assertLess(time.monotonic()-started,.3)
  finally:client.close();release.set();thread.join(1);output.close()
