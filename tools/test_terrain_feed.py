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
        self.assertEqual(calls[0]['radius'],12000);self.assertIs(calls[0]['manager_cache'],feed.cache)
        frame['arm_id']=4;self.assertEqual(feed.observe(frame,sample,{'armed':True})['terrain_state'],'cargando')
    def test_partial_or_over_budget_geometry_is_unknown_and_never_truncated(self):
        row={'vertices':[[0,0,0],[1000,0,0],[0,0,1000]]}
        for complete,rows in [(False,[row]),(True,[row]*(MAX_TRIANGLES+1))]:
            feed=TerrainFeed(None,1,2,extractor=lambda *a,**k:{'complete':complete,'triangles':rows})
            feed.request=((0,0,0),1);feed.capture(feed.request)
            data=feed.latest[2];self.assertFalse(data['complete']);self.assertEqual(data['triangles'],[]);self.assertEqual(data['kinds'],[])
    def test_read_finishing_after_disarm_is_discarded(self):
        feed=None
        def extract(*args,**kwargs):feed.request=None;return {'complete':True,'triangles':[]}
        feed=TerrainFeed(None,1,2,extractor=extract);feed.request=((0,0,0),1);feed.capture(feed.request);self.assertIsNone(feed.latest)
