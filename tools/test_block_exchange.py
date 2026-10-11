"""Real authenticated TCP -> control policy -> actual command writer, fake Windows mappings."""
import socket
import struct
import threading
import unittest
import uuid
import fake_re4_host as protocol
from re4_block_preview import NativePreview
from re4_control import ControlPolicy
from test_cube_test import Kernel
from test_re4_control import Writer


class BlockExchangeTests(unittest.TestCase):
    def test_place_remove_f8_rearm_and_disconnect_disable_mapping(self):
        kernel=Kernel(version=2,build=419)
        visual=NativePreview(42,kernel,user=kernel)
        writer=Writer();policy=ControlPolicy(writer,visual)
        errors=[];report={};token='a'*64;session=str(uuid.uuid4())
        bounds=[-50,-50,-650,50,50,-550]
        with socket.socket() as listener:
            listener.bind(('127.0.0.1',0));listener.listen(1);listener.settimeout(3)
            def serve():
                try:
                    peer,_=listener.accept()
                    with peer:
                        try:protocol.serve_peer(peer,token,report,lambda:{'state':'player'},policy.apply)
                        finally:policy.stop()
                except Exception as error:errors.append(error)
            thread=threading.Thread(target=serve);thread.start()
            try:
                with socket.create_connection(listener.getsockname(),timeout=3) as client,client.makefile('rwb') as stream:
                    protocol.write_frame(stream,dict(protocol=1,type='hello',session=session,token=token))
                    self.assertEqual(protocol.read_frame(stream)['mode'],'re4_control')
                    sequence=0
                    def send(armed,arm_id,box):
                        nonlocal sequence
                        sequence+=1
                        frame=dict(protocol=1,type='re4_control',session=session,seq=sequence,
                                   armed=armed,arm_id=arm_id,buttons=0)
                        if box is not None:frame['preview']={'bounds':box}
                        protocol.write_frame(stream,frame)
                        ack=protocol.read_frame(stream)
                        self.assertEqual(ack['seq'],sequence)
                        self.assertTrue(ack['control_state']['preview_available'])
                        return ack['control_state']
                    self.assertFalse(send(False,0,None)['armed'])
                    self.assertTrue(send(True,1,bounds)['armed'])
                    words=struct.unpack('<8I6f',kernel.command.raw[:56])
                    self.assertEqual(words[4],1);self.assertEqual(words[6],1)
                    self.assertEqual(list(words[8:]),bounds)
                    send(True,1,None)  # Destroying the selected block removes the visual lease.
                    self.assertEqual(struct.unpack_from('<I',kernel.command,16)[0],0)
                    send(True,1,bounds)
                    writer.stop_code=1
                    self.assertFalse(send(True,1,bounds)['armed'])
                    self.assertEqual(struct.unpack_from('<I',kernel.command,16)[0],0)
                    writer.stop_code=0
                    self.assertFalse(send(True,1,bounds)['armed'])
                    self.assertTrue(send(True,2,bounds)['armed'])
                thread.join(3)
                self.assertFalse(thread.is_alive());self.assertFalse(errors,errors)
                self.assertTrue(report['disconnected'])
                self.assertEqual(struct.unpack_from('<I',kernel.command,16)[0],0)
            finally:
                thread.join(3);visual.close()


if __name__=='__main__':unittest.main()
