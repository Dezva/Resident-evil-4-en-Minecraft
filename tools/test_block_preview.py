import ctypes
import unittest
from unittest.mock import Mock,patch
from re4_block_preview import NativePreview,validate_preview
from fake_re4_host import ProtocolError
from re4_control import ControlPolicy
from test_re4_control import Writer
from re4_reader import ReaderError

class PreviewTests(unittest.TestCase):
    bounds=[-500,0,-3500,500,1000,-2500]
    def test_release441_and_legacy439_connect_all_graphics_channels(self):
        for build in (439,441,442,443,444,445,446,447,448,449,450,451,452):
            import struct
            from test_cube_test import Kernel,Function
            kernel=Kernel(version=6,build=build)
            struct.pack_into('<I',kernel.collision,12,build)
            depth=ctypes.create_string_buffer(256);struct.pack_into('<5I',depth,0,0x52453444,1,2,build,42)
            original_open=kernel.OpenFileMappingW.callback;original_view=kernel.MapViewOfFile.callback
            def open_mapping(access,inherit,name):
                if name=='Local\\RE4Bridge-Depth-42':
                    self.assertEqual(access,6);return 14
                return original_open(access,inherit,name)
            def view(handle,access,high,low,size):
                if handle==14:
                    self.assertEqual((access,high,low,size),(6,0,0,256));return ctypes.addressof(depth)
                return original_view(handle,access,high,low,size)
            kernel.OpenFileMappingW=Function(open_mapping);kernel.MapViewOfFile=Function(view)
            preview=NativePreview(42,kernel,user=kernel);writer=Writer();writer.capabilities=31
            policy=ControlPolicy(writer,preview)
            scene={'origin':[0,0,-600,0],'boxes':[self.bounds],'style':1,'collisions':True}
            state=policy.apply(dict(type='re4_control',armed=True,buttons=0,arm_id=7,preview=scene,depth=True),{'state':'player'})
            self.assertTrue(state['armed']);self.assertTrue(state['scene_available']);self.assertTrue(state['collision_available']);self.assertTrue(state['preview_state']['depth_available'])
            self.assertEqual(struct.unpack_from('<2I',depth,216),(1,7));self.assertEqual(struct.unpack_from('<I',kernel.command,16)[0],1)
            policy.stop();self.assertEqual(struct.unpack_from('<2I',depth,216),(0,0));self.assertEqual(struct.unpack_from('<I',kernel.command,16)[0],0)
            preview.close()

    def test_busy_collision_and_graphics_telemetry_never_close_active_control(self):
        from test_cube_test import Kernel
        from re4_cube_test import SnapshotBusy
        kernel=Kernel(version=6,build=434);preview=NativePreview(42,kernel,user=kernel)
        writer=Writer();writer.capabilities=31;policy=ControlPolicy(writer,preview)
        frame=dict(type='re4_control',armed=True,buttons=1,arm_id=1,preview={'bounds':self.bounds})
        sample={'state':'player'};policy.apply(frame,sample)
        with patch('re4_cube_test.read_collision_status',side_effect=SnapshotBusy('busy')),patch('re4_cube_test.read_cube_status',side_effect=SnapshotBusy('busy')):
            for _ in range(1000):
                status=policy.apply(frame,sample);self.assertTrue(status['armed']);self.assertTrue(status['preview_state']['telemetry_stale'])
        self.assertFalse(policy.apply(frame,sample)['preview_state']['telemetry_stale'])
        with patch('re4_cube_test.read_collision_status',side_effect=ReaderError('bad ABI')):
            with self.assertRaises(ReaderError):policy.apply(frame,sample)
        preview.close()
    def test_performance_build_429_keeps_scene_and_collision_channels_compatible(self):
        import struct
        from test_cube_test import Kernel
        from re4_cube_test import read_collision_status
        kernel=Kernel(version=6,build=432)
        struct.pack_into('<I',kernel.collision,12,432)
        preview=NativePreview(42,kernel,user=kernel)
        self.assertEqual(preview.baseline['build'],432)
        preview.publish(True,7,self.bounds)
        self.assertTrue(struct.unpack_from('<I',kernel.command,16)[0])
        self.assertEqual(read_collision_status(kernel,42)['reason'],1)
        preview.close()
        self.assertEqual(struct.unpack_from('<I',kernel.command,16)[0],0)
    def frame(self,**changes):
        return dict(type='re4_control',armed=True,buttons=1,arm_id=1,preview={'bounds':self.bounds},**changes)
    def test_rejects_oversized_nonfinite_inverted_and_unknown_data(self):
        validate_preview({'bounds':self.bounds})
        for bad in [{'bounds':[0]*6},{'bounds':[float('nan')]*6}, {'bounds':[True]*6},
                    {'bounds':[0,0,0,3000,1000,1000]}, {'bounds':self.bounds,'path':'x'}]:
            with self.assertRaises(ProtocolError): validate_preview(bad)

    def test_new_channel_publication_preserves_telemetry_and_fixed_epoch(self):
        from test_cube_test import Kernel
        import struct
        kernel=Kernel(version=2,build=419)
        preview=NativePreview(42,kernel,user=kernel)
        before=kernel.telemetry.raw
        preview.publish(True,7,self.bounds)
        epoch=preview.session.epoch
        words=struct.unpack('<8I6f',kernel.command.raw[:56])
        self.assertFalse(words[2]&1);self.assertEqual(words[4:8],(1,epoch,1,0))
        self.assertEqual(list(words[8:]),self.bounds)
        self.assertEqual(kernel.telemetry.raw,before)
        report=preview.status()
        self.assertTrue(report['requested']);self.assertEqual(report['bounds'],self.bounds)
        self.assertIn('skipped_passes',report);self.assertIn('camera_rejections',report)
        preview.publish(True,7,self.bounds);self.assertEqual(preview.session.epoch,epoch)
        preview.publish(False,7);self.assertEqual(struct.unpack_from('<I',kernel.command,16)[0],0)
        self.assertFalse(preview.status()['requested']);self.assertIsNone(preview.status()['bounds'])
        preview.publish(True,7,self.bounds);self.assertEqual(preview.session.epoch,epoch)
        # A new placement updates the immutable world bounds, not the camera anchor.
        changed=[x+100 for x in self.bounds]
        with patch('secrets.randbits',return_value=epoch+1):preview.publish(True,7,changed)
        self.assertEqual(preview.session.epoch,epoch+1)
        with patch('secrets.randbits',return_value=epoch+2):preview.publish(True,8,changed)
        self.assertEqual(preview.session.epoch,epoch+2)
        preview.close();preview.close()
        self.assertEqual(struct.unpack_from('<I',kernel.command,16)[0],0)
        self.assertEqual(kernel.released,1)
        self.assertIn((11,6,56),kernel.map_requests)

    def test_observed_block_cannot_activate_native_channel_until_armed(self):
        from test_cube_test import Kernel
        import struct
        kernel=Kernel(version=2,build=419);visual=NativePreview(42,kernel,user=kernel)
        writer=Writer();policy=ControlPolicy(writer,visual);sample={'state':'player'}
        frame=dict(self.frame(),armed=False,buttons=0)
        self.assertFalse(policy.apply(frame,sample)['preview_state']['requested'])
        self.assertEqual(struct.unpack_from('<I',kernel.command,16)[0],0)
        result=policy.apply(dict(frame,armed=True,arm_id=2),sample)
        self.assertTrue(result['armed']);self.assertTrue(result['preview_state']['requested'])
        epoch=visual.session.epoch
        policy.apply(dict(frame,armed=True,arm_id=2),sample)
        self.assertEqual(visual.session.epoch,epoch)
        policy.stop();self.assertFalse(visual.status()['requested']);visual.close()

    def test_rejects_old_mapping_wrong_build_and_second_writer_without_activation(self):
        from test_cube_test import Kernel
        for kernel in [Kernel(),Kernel(version=2,build=418),Kernel(version=2,build=419,wait=0x102)]:
            before=kernel.command.raw
            with self.assertRaises(ReaderError):NativePreview(42,kernel,user=kernel)
            self.assertEqual(kernel.command.raw,before)
            self.assertIn(12,kernel.closed)

    def test_graphics_failure_disarms_and_needs_new_arm(self):
        from test_cube_test import Kernel
        import struct
        kernel=Kernel(version=2,build=419);visual=NativePreview(42,kernel,user=kernel)
        writer=Writer();policy=ControlPolicy(writer,visual);sample={'state':'player'}
        frame=self.frame();result=policy.apply(frame,sample);self.assertTrue(result['armed'])
        # The renderer's failed draw must release native input as well as the visual lease.
        struct.pack_into('<I',kernel.telemetry,44,1)
        struct.pack_into('<I',kernel.telemetry,64,9)
        result=policy.apply(frame,sample);self.assertFalse(result['armed'])
        self.assertFalse(result['preview_state']['requested'])
        self.assertEqual(struct.unpack_from('<I',kernel.command,16)[0],0)
        self.assertFalse(policy.apply(frame,sample)['armed'])
        # A stale reason from the last scene cannot reject a newly requested epoch.
        result=policy.apply(dict(frame,arm_id=2),sample);self.assertTrue(result['armed'])
        struct.pack_into('<I',kernel.telemetry,48,1)
        result=policy.apply(dict(frame,arm_id=2),sample);self.assertFalse(result['armed'])
        self.assertFalse(policy.apply(dict(frame,arm_id=3),sample)['armed'])
        visual.close()

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
