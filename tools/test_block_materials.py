import base64
import copy
import struct
import unittest
from unittest.mock import patch
from fake_re4_host import ProtocolError
from re4_reader import ReaderError
from re4_block_preview import NativePreview,validate_preview
from re4_cube_test import read_collision_status
from re4_control import ControlPolicy
from test_cube_test import Kernel
from test_re4_control import Writer

def material_scene(collisions=False):
    tiles=[base64.b64encode(struct.pack('<256I',*([color]*256))).decode() for color in (0xFF112233,0x80445566)]
    face=[0,0,0,1,0,1,1,0,1]
    return dict(origin=[10,20,30,.25],boxes=[[-500,0,-500,500,1000,500],[500,0,-500,1500,500,500]],style=3,tiles=tiles,
                faces=[[list(face) for _ in range(6)],[[1]+face[1:] for _ in range(6)]],collisions=collisions)

class MaterialsTests(unittest.TestCase):
    def test_debris_motion_and_expiry_keep_epoch_but_solid_changes_rearm(self):
        k=Kernel(version=6,build=424);v=NativePreview(42,k,user=k)
        scene=material_scene(True);scene['collider_count']=1
        v.publish(True,7,scene);epoch=v.session.epoch
        for step in range(1,21):
            frame=copy.deepcopy(scene);frame['boxes'][1]=[500+step*20,1000,-500,590+step*20,1090,-410]
            v.publish(True,7,frame)
            self.assertEqual(v.session.epoch,epoch)
            self.assertEqual(struct.unpack_from('<f',k.command,104)[0],500+step*20)
        expired=dict(scene,boxes=scene['boxes'][:1],faces=scene['faces'][:1],tiles=scene['tiles'][:1])
        v.publish(True,7,expired);self.assertEqual(v.session.epoch,epoch)
        changed=copy.deepcopy(expired);changed['boxes'][0][3]+=100
        with patch('secrets.randbits',return_value=epoch+1):v.publish(True,7,changed)
        self.assertEqual(v.session.epoch,epoch+1);v.close()

    def test_complete_v6_palette_faces_cropped_uv_and_cleanup(self):
        k=Kernel(version=6,build=424);v=NativePreview(42,k,user=k);scene=material_scene(True)
        self.assertTrue(v.materials_available);self.assertTrue(v.collision_available)
        v.publish(True,7,scene);self.assertEqual(struct.unpack_from('<4I',k.command,3152),(1,7,2,0))
        self.assertEqual(struct.unpack_from('<I8f',k.command,3168),tuple(scene['faces'][0][0]))
        self.assertEqual(struct.unpack_from('<I',k.command,30816)[0],0xFF112233)
        self.assertEqual(struct.unpack_from('<I',k.command,31840)[0],0x80445566)
        epoch=v.session.epoch;smaller=dict(scene,boxes=scene['boxes'][1:],faces=scene['faces'][1:])
        with patch('secrets.randbits',return_value=epoch+1):v.publish(True,7,smaller)
        self.assertEqual(v.session.epoch,epoch+1);self.assertEqual(struct.unpack_from('<I',k.command,3168)[0],1)
        self.assertEqual(k.command.raw[3384:30816],bytes(30816-3384))
        report=v.status();self.assertTrue(report['collisions_requested']);self.assertEqual(report['tile_count'],2)
        v.close();self.assertEqual(k.command.raw[56:96352],bytes(96352-56))

    def test_validation_rejects_palette_uv_indices_flags_before_writing(self):
        k=Kernel(version=6,build=424);v=NativePreview(42,k,user=k);before=k.command.raw;scene=material_scene()
        invalid=[dict(scene,tiles=[]),dict(scene,tiles=scene['tiles']*33),dict(scene,tiles=['x'*1368]),dict(scene,faces=[]),dict(scene,collisions=1),dict(scene,path='x')]
        for value in (float('nan'),-1,1.01,True):
            bad=copy.deepcopy(scene);bad['faces'][0][0][1]=value;invalid.append(bad)
        for index in (2,-1,True):
            bad=copy.deepcopy(scene);bad['faces'][0][0][0]=index;invalid.append(bad)
        for bad in invalid:
            with self.assertRaises(ProtocolError):v.publish(True,1,bad)
            self.assertEqual(k.command.raw,before)
        with self.assertRaises(ReaderError):v.session.publish(True,scene=material_scene(True))
        self.assertEqual(k.command.raw,before);v.close()

    def test_old_channels_never_activate_materials_or_collision(self):
        for version,build in ((3,421),(4,422),(5,423)):
            k=Kernel(version=version,build=build);v=NativePreview(42,k,user=k);writer=Writer();policy=ControlPolicy(writer,v);before=k.command.raw
            with self.assertRaises(ReaderError):v.publish(True,1,material_scene())
            self.assertEqual(k.command.raw,before)
            state=policy.apply(dict(type='re4_control',armed=True,arm_id=1,buttons=1,preview=material_scene(True)),{'state':'player'})
            self.assertFalse(state['armed']);self.assertEqual(writer.writes[-1],(False,0));v.close()

    def test_palette_and_mining_uv_updates_keep_epoch_without_moving_grid(self):
        k=Kernel(version=6,build=424);v=NativePreview(42,k,user=k);scene=material_scene();v.publish(True,1,scene);epoch=v.session.epoch
        origin=k.command.raw[56:72];scene['tiles']=scene['tiles'][::-1]
        with patch('secrets.randbits',return_value=epoch+1):v.publish(True,1,scene)
        self.assertEqual(v.session.epoch,epoch);self.assertEqual(k.command.raw[56:72],origin)
        self.assertEqual(struct.unpack_from('<I',k.command,30816)[0],0x80445566)
        scene['faces'][0][0][1]=.25;v.publish(True,1,scene)
        self.assertEqual(v.session.epoch,epoch);self.assertEqual(struct.unpack_from('<f',k.command,3172)[0],.25);v.close()

    def test_collision_readonly_snapshot_and_current_generation_fail_stop(self):
        k=Kernel(version=6,build=424);v=NativePreview(42,k,user=k);before=k.collision.raw
        self.assertEqual(read_collision_status(k,42)['reason'],1);self.assertEqual(k.collision.raw,before)
        writer=Writer();policy=ControlPolicy(writer,v);frame=dict(type='re4_control',armed=True,arm_id=1,buttons=1,preview=material_scene(True))
        self.assertTrue(policy.apply(frame,{'state':'player'})['armed'])
        struct.pack_into('<I',k.collision,36,3);struct.pack_into('<I',k.collision,44,1)
        self.assertFalse(policy.apply(frame,{'state':'player'})['armed'])
        self.assertEqual(struct.unpack_from('<I',k.command,3152)[0],0)
        # An old rejection must not refuse the next generation before native callbacks see it.
        self.assertTrue(policy.apply(dict(frame,arm_id=2),{'state':'player'})['armed'])
        policy.stop();v.close()

    def test_largest_scene_stays_inside_network_and_mapping_limits(self):
        import json
        s=material_scene();s['tiles']=s['tiles']*32;s['boxes']=s['boxes']*64;s['faces']=s['faces']*64
        validate_preview(s);self.assertLess(len(json.dumps(s).encode()),524288)
        k=Kernel(version=6,build=424);v=NativePreview(42,k,user=k);v.publish(True,1,s)
        self.assertEqual(struct.unpack_from('<I',k.command,3160)[0],64);self.assertEqual(len(k.command.raw[:96352]),96352);v.close()
