import unittest
from re4_geometry import CollisionWorld


def report(points,kind='wall'):
    return dict(mode='READ ONLY COLLISION GEOMETRY',complete=True,position=[0,0,0],radius_native=20000,
                triangles=[dict(source='SAT',object=0,kind=kind,vertices=points)])


class GeometryTests(unittest.TestCase):
    def test_ray_uses_surface_not_only_bounding_box_and_hits_both_sides(self):
        world=CollisionWorld(report([[0,0,1000],[1000,0,1000],[0,1000,1000]]))
        hit=world.raycast([100,100,0],[0,0,2],4000)
        self.assertEqual(1000,hit.distance);self.assertEqual((100,100,1000),hit.point);self.assertEqual((0,0,-1),hit.normal)
        self.assertEqual(1000,world.raycast([100,100,2000],[0,0,-1],4000).distance)
        self.assertIsNone(world.raycast([900,900,0],[0,0,1],4000));self.assertIsNone(world.raycast([100,100,0],[0,0,1],500))

    def test_placement_allows_contact_but_rejects_actual_overlap_and_false_aabb_hits(self):
        world=CollisionWorld(report([[0,0,1000],[1000,0,1000],[0,1000,1000]]))
        self.assertEqual(1,len(world.intersections([50,50,950,150,150,1050],.5)))
        self.assertFalse(world.intersections([850,850,950,950,950,1050],.5))
        self.assertFalse(world.intersections([50,50,0,150,150,1000],.5))
        self.assertEqual(1,len(world.intersections([50,50,0,150,150,1000])))

    def test_floor_query_can_select_floor_and_preserves_triangle_slope(self):
        world=CollisionWorld(report([[0,0,0],[1000,0,0],[0,500,1000]],'slope'))
        hit=world.raycast([100,1000,100],[0,-1,0],3000,{'floor','slope'})
        self.assertAlmostEqual(50,hit.point[1]);self.assertIsNone(world.raycast([100,1000,100],[0,-1,0],3000,{'wall'}))

    def test_bvh_returns_nearest_across_different_leaves(self):
        data=report([[0,0,100],[1000,0,100],[0,1000,100]])
        data['triangles']*=1
        for z in range(200,4200,100):data['triangles'].append(dict(source='EAT',object=0,kind='wall',vertices=[[0,0,z],[1000,0,z],[0,1000,z]]))
        world=CollisionWorld(data)
        self.assertEqual(100,world.raycast([10,10,0],[0,0,1],5000).distance)
        self.assertEqual(4100,world.raycast([10,10,5000],[0,0,-1],5000).point[2])

    def test_incomplete_geometry_or_unknown_space_is_never_free(self):
        data=report([[0,0,1000],[1000,0,1000],[0,1000,1000]]);data['complete']=False
        with self.assertRaises(ValueError):CollisionWorld(data)
        data['complete']=True;world=CollisionWorld(data)
        for origin,direction,distance in [([19900,0,0],[1,0,0],200),([0,0,0],[0,0,0],100),([0,0,0],[1,0,0],float('nan'))]:
            with self.assertRaises(ValueError):world.raycast(origin,direction,distance)
        with self.assertRaises(ValueError):world.intersections([19900,0,0,20100,100,100])
        data['triangles'][0]['vertices'][0][0]=float('nan')
        with self.assertRaises(ValueError):CollisionWorld(data)


if __name__=='__main__':unittest.main()
