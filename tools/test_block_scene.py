import math
import struct
import unittest
from unittest.mock import patch
from re4_block_preview import NativePreview,validate_preview
from re4_control import ControlPolicy
from fake_re4_host import ProtocolError
from re4_reader import ReaderError
from test_cube_test import Kernel
from test_re4_control import Writer

class SceneTests(unittest.TestCase):
    def test_render_only_collision_prefix_is_validated(self):
        scene=self.scene();scene['collider_count']=0;validate_preview(scene)
        for value in (True,-1,len(scene['boxes'])+1):
            with self.assertRaises(ProtocolError):validate_preview(dict(scene,collider_count=value))
    def scene(self):
        return {'origin':[10,20,30,.25], 'boxes':[[-500,0,-500,500,1000,500],[500,0,-500,1500,500,500]]}
    def test_scene_mapping_full_snapshot_rotation_individual_removal_and_cleanup(self):
        k=Kernel(version=3,build=421);before=k.telemetry.raw
        visual=NativePreview(42,k,user=k);scene=self.scene()
        self.assertTrue(visual.scene_available);visual.publish(True,1,scene)
        command=k.command.raw[:3152];head=struct.unpack_from('<8I',command)
        self.assertEqual(head[1],3);self.assertEqual(head[4],1);self.assertEqual(head[6],2);self.assertFalse(head[2]&1)
        self.assertEqual(struct.unpack_from('<4f2I',command,56),(10,20,30,.25,2,0))
        self.assertEqual(list(struct.unpack_from('<6f',command,104)),scene['boxes'][1])
        self.assertEqual(command[128:],bytes(3152-128));self.assertEqual(k.telemetry.raw,before)
        epoch=visual.session.epoch;visual.publish(True,1,self.scene());self.assertEqual(visual.session.epoch,epoch)
        smaller={'origin':scene['origin'],'boxes':[scene['boxes'][1]]}
        with patch('secrets.randbits',return_value=epoch+1):visual.publish(True,1,smaller)
        self.assertEqual(struct.unpack_from('<I',k.command,72)[0],1)
        self.assertEqual(k.command.raw[104:3152],bytes(3152-104))
        state=visual.status();self.assertEqual(state['box_count'],1);self.assertEqual(state['origin'],scene['origin'])
        visual.close();self.assertEqual(struct.unpack_from('<I',k.command,16)[0],0)
        self.assertEqual(k.command.raw[56:3152],bytes(3152-56));self.assertEqual(k.released,1)
        self.assertIn((11,6,3152),k.map_requests)

    def test_invalid_count_origin_and_shapes_do_not_modify_command(self):
        k=Kernel(version=3,build=421);visual=NativePreview(42,k,user=k);before=k.command.raw
        valid=self.scene()
        bad=[dict(valid,boxes=[]),dict(valid,boxes=valid['boxes']*65),dict(valid,origin=[0,0,0,math.nan]),
             dict(valid,origin=[0,0,0,4]),dict(valid,boxes=[[0,0,0,0,1,1]]),dict(valid,boxes=[[0,0,0,9000,1,1]]),
             dict(valid,bounds=[0]*6),dict(valid,origin=[True,0,0,0])]
        for scene in bad:
            with self.assertRaises(ProtocolError):visual.publish(True,1,scene)
            self.assertEqual(k.command.raw,before)
        visual.close()

    def test_legacy_channel_rejects_scene_without_writing_but_still_supports_single_box(self):
        k=Kernel(version=2,build=419);visual=NativePreview(42,k,user=k);before=k.command.raw
        self.assertFalse(visual.scene_available)
        with self.assertRaises(ReaderError):visual.publish(True,1,self.scene())
        self.assertEqual(k.command.raw,before)
        visual.publish(True,1,[-500,0,-500,500,1000,500]);self.assertTrue(visual.status()['requested']);visual.close()

    def test_scene_arm_f8_new_arm_and_restore_failure_stop_input_and_drawing(self):
        k=Kernel(version=3,build=421);visual=NativePreview(42,k,user=k);writer=Writer();policy=ControlPolicy(writer,visual)
        frame={'type':'re4_control','armed':False,'buttons':0,'arm_id':0,'preview':self.scene()};sample={'state':'player'}
        result=policy.apply(frame,sample);self.assertTrue(result['scene_available']);self.assertFalse(result['preview_state']['requested'])
        frame=dict(frame,armed=True,arm_id=1);self.assertTrue(policy.apply(frame,sample)['armed'])
        writer.stop_code=1;self.assertFalse(policy.apply(frame,sample)['armed'])
        self.assertEqual(struct.unpack_from('<I',k.command,72)[0],0)
        writer.stop_code=0;self.assertFalse(policy.apply(frame,sample)['armed'])
        frame=dict(frame,arm_id=2);self.assertTrue(policy.apply(frame,sample)['armed'])
        struct.pack_into('<I',k.telemetry,48,1)
        result=policy.apply(frame,sample);self.assertFalse(result['armed']);self.assertFalse(result['preview_state']['requested'])
        self.assertFalse(policy.apply(dict(frame,arm_id=3),sample)['armed']);visual.close()

    def test_maximum_scene_and_thin_shapes_are_bounded(self):
        valid={'origin':[0,0,0,-math.pi],'boxes':[[0,0,0,10,500,1000]]*128}
        validate_preview(valid)
        k=Kernel(version=3,build=421);visual=NativePreview(42,k,user=k)
        visual.publish(True,1,valid);self.assertEqual(struct.unpack_from('<I',k.command,72)[0],128)
        self.assertEqual(struct.unpack_from('<6f',k.command,80+127*24),(0,0,0,10,500,1000));visual.close()

    def test_surface_v4_style_change_preserves_origin_removal_and_cleanup(self):
        k=Kernel(version=4,build=422);before=k.telemetry.raw
        visual=NativePreview(42,k,user=k);scene=dict(self.scene(),style=1)
        self.assertTrue(visual.surface_available);visual.publish(True,1,scene)
        self.assertEqual(struct.unpack_from('<I',k.command,76)[0],1)
        self.assertEqual(visual.status()['style'],1);epoch=visual.session.epoch
        visual.publish(True,1,scene);self.assertEqual(visual.session.epoch,epoch)
        origin=k.command.raw[56:72];boxes=k.command.raw[80:128]
        with patch('secrets.randbits',return_value=epoch+1):visual.publish(True,1,dict(scene,style=0))
        self.assertEqual(visual.session.epoch,epoch+1)
        self.assertEqual(k.command.raw[56:72],origin);self.assertEqual(k.command.raw[80:128],boxes)
        self.assertEqual(struct.unpack_from('<I',k.command,76)[0],0)
        visual.publish(True,1,dict(scene,boxes=scene['boxes'][1:]))
        self.assertEqual(k.command.raw[104:3152],bytes(3152-104))
        # The fake kernel advances frame metrics when status is mapped; publishing must leave them alone.
        before=k.telemetry.raw;visual.publish(True,1,dict(scene,boxes=scene['boxes'][1:]));self.assertEqual(k.telemetry.raw,before)
        visual.close();self.assertEqual(k.command.raw[56:3152],bytes(3152-56))

    def test_surface_invalid_style_or_old_build_never_activates_controls(self):
        scene=self.scene()
        for version,build in ((3,421),(4,422)):
            k=Kernel(version=version,build=build);visual=NativePreview(42,k,user=k);before=k.command.raw
            for style in (-1,3,True,1.0,'solid'):
                with self.assertRaises(ProtocolError):visual.publish(True,1,dict(scene,style=style))
                self.assertEqual(k.command.raw,before)
            if version==3:
                with self.assertRaises(ReaderError):visual.publish(True,1,dict(scene,style=1))
                self.assertEqual(k.command.raw,before)
                with self.assertRaises(ReaderError):visual.session.publish(True,scene=dict(scene,style=1))
                writer=Writer();policy=ControlPolicy(writer,visual)
                state=policy.apply(dict(type='re4_control',armed=True,buttons=1,arm_id=1,preview=dict(scene,style=1)),{'state':'player'})
                self.assertFalse(state['armed']);self.assertFalse(state['preview_state']['requested'])
                self.assertEqual(writer.writes[-1],(False,0));self.assertFalse(state['surface_available'])
            visual.close()
        for version,build in ((4,421),(3,422)):
            k=Kernel(version=version,build=build)
            with self.assertRaises(ReaderError):NativePreview(42,k,user=k)

    def test_surface_f8_requires_new_arm_and_restore_failure_blocks_rearm(self):
        k=Kernel(version=4,build=422);visual=NativePreview(42,k,user=k);writer=Writer();policy=ControlPolicy(writer,visual)
        frame=dict(type='re4_control',armed=True,buttons=1,arm_id=1,preview=dict(self.scene(),style=1))
        self.assertTrue(policy.apply(frame,{'state':'player'})['armed']);writer.stop_code=1
        self.assertFalse(policy.apply(frame,{'state':'player'})['armed'])
        self.assertEqual(struct.unpack_from('<I',k.command,76)[0],0);writer.stop_code=0
        self.assertFalse(policy.apply(frame,{'state':'player'})['armed'])
        frame=dict(frame,arm_id=2);self.assertTrue(policy.apply(frame,{'state':'player'})['armed'])
        struct.pack_into('<I',k.telemetry,48,1)
        self.assertFalse(policy.apply(frame,{'state':'player'})['armed'])
        self.assertFalse(policy.apply(dict(frame,arm_id=3),{'state':'player'})['armed']);visual.close()

    def test_texture_v5_mapping_style_and_stable_reference_removal_and_close(self):
        k=Kernel(version=5,build=423);visual=NativePreview(42,k,user=k);scene=dict(self.scene(),style=2)
        self.assertTrue(visual.texture_available);visual.publish(True,1,scene)
        self.assertEqual(struct.unpack_from('<I',k.command,76)[0],2);self.assertEqual(visual.status()['style'],2)
        origin=k.command.raw[56:72];boxes=k.command.raw[80:128];epoch=visual.session.epoch
        with patch('secrets.randbits',return_value=epoch+1):visual.publish(True,1,dict(scene,style=1))
        self.assertEqual(visual.session.epoch,epoch+1);self.assertEqual(k.command.raw[56:72],origin);self.assertEqual(k.command.raw[80:128],boxes)
        visual.publish(True,1,dict(scene,boxes=scene['boxes'][1:]))
        self.assertEqual(struct.unpack_from('<I',k.command,72)[0],1);self.assertEqual(k.command.raw[104:3152],bytes(3152-104))
        visual.close();self.assertEqual(k.command.raw[56:3152],bytes(3152-56))

    def test_texture_old_channel_rejects_before_input_or_mapping_write(self):
        for version,build in ((2,419),(3,421),(4,422)):
            k=Kernel(version=version,build=build);visual=NativePreview(42,k,user=k);before=k.command.raw
            scene=dict(self.scene(),style=2);self.assertFalse(visual.texture_available)
            with self.assertRaises(ReaderError):visual.publish(True,1,scene)
            with self.assertRaises(ReaderError):visual.session.publish(True,scene=scene)
            self.assertEqual(k.command.raw,before)
            writer=Writer();policy=ControlPolicy(writer,visual)
            state=policy.apply(dict(type='re4_control',armed=True,buttons=1,arm_id=1,preview=scene),{'state':'player'})
            self.assertFalse(state['armed']);self.assertEqual(writer.writes[-1],(False,0));self.assertFalse(state['preview_state']['requested']);visual.close()
        for version,build in ((4,423),(5,422)):
            k=Kernel(version=version,build=build)
            with self.assertRaises(ReaderError):NativePreview(42,k,user=k)

    def test_texture_f8_restore_failure_disarms_and_blocks_reactivation(self):
        k=Kernel(version=5,build=423);visual=NativePreview(42,k,user=k);writer=Writer();policy=ControlPolicy(writer,visual)
        frame=dict(type='re4_control',armed=True,buttons=1,arm_id=1,preview=dict(self.scene(),style=2))
        self.assertTrue(policy.apply(frame,{'state':'player'})['armed']);writer.stop_code=1
        self.assertFalse(policy.apply(frame,{'state':'player'})['armed']);writer.stop_code=0
        self.assertEqual(struct.unpack_from('<I',k.command,76)[0],0)
        self.assertFalse(policy.apply(frame,{'state':'player'})['armed'])
        frame=dict(frame,arm_id=2);self.assertTrue(policy.apply(frame,{'state':'player'})['armed'])
        struct.pack_into('<I',k.telemetry,48,1)
        self.assertFalse(policy.apply(frame,{'state':'player'})['armed'])
        self.assertFalse(policy.apply(dict(frame,arm_id=3),{'state':'player'})['armed']);visual.close()

if __name__=='__main__':unittest.main()
