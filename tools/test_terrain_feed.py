import unittest,time
from re4_terrain_feed import TerrainFeed,MAX_TRIANGLES
class TerrainFeedTests(unittest.TestCase):
    def test_idle_has_no_process_reads_and_requests_have_bounded_revisions(self):
        calls=[];clock=[0.]
        def extract(*args,**kwargs):
            calls.append(kwargs);return {'complete':True,'triangles':[{'kind':'floor','vertices':[[0,0,0],[1000,0,0],[0,0,1000]]}]}
        feed=TerrainFeed(lambda *a:None,1,2,extractor=extract,clock=lambda:clock[0])
        sample={'state':'player','position_raw':[0,0,0]};frame={'terrain':False,'arm_id':3,'look':{'first_person':True}}
        self.assertEqual(feed.observe(frame,sample,{'armed':True})['terrain_state'],'inactivo');self.assertIsNone(feed.thread);self.assertFalse(calls)
        # Drive a snapshot without a background thread for deterministic budget/age checks.
        feed.request=((0,0,0),3);feed.capture(feed.request);self.assertEqual(len(calls),1)
        frame['terrain']=True;feed.thread=object()
        output=feed.observe(frame,sample,{'armed':True});self.assertEqual(len(output['terrain']['triangles'][0]),9)
        self.assertEqual(output['terrain']['kinds'],['floor'])
        frame['terrain_revision']=output['terrain']['revision'];self.assertNotIn('terrain',feed.observe(frame,sample,{'armed':True}))
        clock[0]=1.01;self.assertEqual(feed.observe(frame,sample,{'armed':True})['terrain_state'],'caducado')
        self.assertEqual(calls[0]['radius'],32000);self.assertIs(calls[0]['manager_cache'],feed.cache)
        frame['arm_id']=4;self.assertEqual(feed.observe(frame,sample,{'armed':True})['terrain_state'],'cargando')
    def test_partial_or_over_budget_geometry_is_unknown_and_never_truncated(self):
        row={'vertices':[[0,0,0],[1000,0,0],[0,0,1000]]}
        for complete,rows in [(False,[row]),(True,[row]*(MAX_TRIANGLES+1))]:
            feed=TerrainFeed(None,1,2,extractor=lambda *a,**k:{'complete':complete,'triangles':rows})
            feed.request=((0,0,0),1);feed.capture(feed.request)
            data=feed.latest[2];self.assertFalse(data['complete']);self.assertEqual(data['triangles'],[]);self.assertEqual(data['kinds'],[])
    def test_absolute_feet_move_the_query_without_expanding_its_budget(self):
        feed=TerrainFeed(None,1,2);feed.thread=object()
        frame=dict(terrain=True,arm_id=3,look=dict(first_person=True),vertical=dict(lift=24000,scale=1000,token=1,feet_y=24002))
        feed.observe(frame,dict(state="player",position_raw=[1,2,3]),dict(armed=True))
        self.assertEqual(feed.request,((1,24002,3),3));self.assertIsNone(feed.latest)
        feed.observe(frame,dict(state="player",position_raw=[1,24002,3]),dict(armed=True))
        self.assertEqual(feed.request,((1,24002,3),3)) # real actor sample must not double the height
    def test_read_finishing_after_disarm_is_discarded(self):
        feed=None
        def extract(*args,**kwargs):feed.request=None;return {'complete':True,'triangles':[]}
        feed=TerrainFeed(None,1,2,extractor=extract);feed.request=((0,0,0),1);feed.capture(feed.request);self.assertIsNone(feed.latest)
    def test_measured_village_sized_map_is_complete_and_reaches_elevated_targets(self):
        from re4_terrain_feed import bounded_snapshot,MAX_TERRAIN_BYTES
        import json
        row={'kind':'wall','vertices':[[2000,10000,0],[2000,12000,0],[2000,10000,1000]]}
        complete,triangles,kinds,radius=bounded_snapshot([row]*2985,[0,0,0],32000)
        self.assertTrue(complete);self.assertEqual(radius,32000);self.assertEqual(len(triangles),2985)
        self.assertLessEqual(len(json.dumps(dict(triangles=triangles,kinds=kinds),separators=(',',':')).encode()),MAX_TERRAIN_BYTES)
    def test_over_budget_reduces_coverage_instead_of_truncating_or_stalling_everywhere(self):
        from re4_terrain_feed import bounded_snapshot
        near={'vertices':[[0,0,0],[1000,0,0],[0,0,1000]]}
        far={'vertices':[[30000,0,0],[31000,0,0],[30000,0,1000]]}
        complete,triangles,kinds,radius=bounded_snapshot([near]+[far]*MAX_TRIANGLES,[0,0,0],32000)
        self.assertTrue(complete);self.assertLess(radius,30000);self.assertEqual(triangles,[[0,0,0,1000,0,0,0,0,1000]])
    def test_one_large_triangle_crossing_reduced_volume_is_never_dropped(self):
        from re4_terrain_feed import bounded_snapshot
        crossing={'vertices':[[-40000,0,0],[40000,0,0],[0,0,40000]]}
        complete,triangles,kinds,radius=bounded_snapshot([crossing]*(MAX_TRIANGLES+1),[0,0,0],32000)
        self.assertFalse(complete);self.assertEqual(triangles,[])

class CombatTerrainFeedTests(unittest.TestCase):
    def test_short_range_geometry_has_its_own_revision_and_does_not_replace_building_map(self):
        from re4_terrain_feed import CombatTerrainFeed
        calls=[]
        def extract(*args,**kwargs):calls.append(kwargs);return dict(complete=True,triangles=[])
        feed=CombatTerrainFeed(None,1,2,extractor=extract,clock=lambda:0);feed.thread=object()
        frame=dict(arm_id=7,combat_enabled=True,combat_scale=1000,combat_reach=3,look=dict(first_person=True))
        sample=dict(state='player',position_raw=[0,0,0]);feed.observe(frame,sample,dict(armed=True));feed.capture(feed.request)
        reply=feed.observe(frame,sample,dict(armed=True));self.assertIn('combat_terrain',reply);self.assertNotIn('terrain',reply);self.assertEqual(calls[0]['radius'],6500);self.assertEqual(reply['combat_terrain']['radius'],6500)
        frame['combat_terrain_revision']=reply['combat_terrain']['revision'];self.assertNotIn('combat_terrain',feed.observe(frame,sample,dict(armed=True)))
        frame['combat_enabled']=False;self.assertEqual(feed.observe(frame,sample,dict(armed=True))['combat_terrain_state'],'inactivo');self.assertIsNone(feed.request)
    def test_radius_change_during_capture_cannot_mislabel_coverage(self):
        feed=None
        def extract(*args,**kwargs):feed.radius=6500;return dict(complete=True,triangles=[])
        feed=TerrainFeed(None,1,2,extractor=extract);feed.request=((0,0,0),7);feed.capture(feed.request);self.assertIsNone(feed.latest)
