import ctypes
import unittest
from re4_block_preview import NativePreview,validate_preview
from fake_re4_host import ProtocolError
from re4_control import ControlPolicy
from test_re4_control import Writer

class PreviewTests(unittest.TestCase):
    bounds=[-500,0,-3500,500,1000,-2500]
    def frame(self,**changes):
        return dict(type='re4_control',armed=True,buttons=1,arm_id=1,preview={'bounds':self.bounds},**changes)
    def test_rejects_oversized_nonfinite_inverted_and_unknown_data(self):
        validate_preview({'bounds':self.bounds})
        for bad in [{'bounds':[0]*6},{'bounds':[float('nan')]*6}, {'bounds':[True]*6},
                    {'bounds':[0,0,0,3000,1000,1000]}, {'bounds':self.bounds,'path':'x'}]:
            with self.assertRaises(ProtocolError): validate_preview(bad)

    def test_publication_preserves_render_counters(self):
        preview=NativePreview.__new__(NativePreview)
        backing=(ctypes.c_uint32*17)();preview.words=backing;preview.address=ctypes.addressof(backing)
        class Kernel:
            def GetTickCount(self):return 100
        preview.k=Kernel();backing[12]=1;backing[13]=99;backing[14]=20
        preview.publish(True,7,self.bounds)
        self.assertEqual(backing[2]&1,0);self.assertEqual(backing[4],1);self.assertEqual(backing[5],7)
        self.assertEqual(list((ctypes.c_float*6).from_address(preview.address+24)),self.bounds)
        preview.publish(False,7)
        self.assertEqual(backing[4],0);self.assertEqual(preview.status()['frames'],99)
        self.assertEqual(preview.status()['draws'],20)

    def test_stop_disarm_and_removed_preview_disable_the_visual_channel(self):
        class Preview:
            def __init__(self):self.enabled=False;self.bounds=None
            def publish(self,enabled,generation,bounds=None):self.enabled=enabled;self.bounds=bounds
            def status(self):return {'status':1,'frames':9,'draws':3,'failures':0,'last_hresult':0}
        visual=Preview();writer=Writer();policy=ControlPolicy(writer,visual)
        frame=self.frame();sample={'state':'player'}
        self.assertEqual(policy.apply(frame,sample)['preview_state']['draws'],3)
        self.assertTrue(visual.enabled)
        empty=dict(frame);del empty['preview'];policy.apply(empty,sample)
        self.assertFalse(visual.enabled)
        policy.apply(frame,sample);self.assertTrue(visual.enabled)
        writer.stop_code=1;policy.apply(frame,sample);self.assertFalse(visual.enabled)
        writer.stop_code=0;policy.apply(frame,sample);self.assertFalse(visual.enabled)
        policy.apply(dict(frame,arm_id=2),sample);self.assertTrue(visual.enabled)
        policy.stop();self.assertFalse(visual.enabled)
