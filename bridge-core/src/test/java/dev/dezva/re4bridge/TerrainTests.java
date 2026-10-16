package dev.dezva.re4bridge;
import com.google.gson.*;
import org.junit.jupiter.api.Test;
import java.io.IOException;
import static org.junit.jupiter.api.Assertions.*;
class TerrainTests {
    @Test void villageSizedTerrainKeepsElevatedCollisionAndRejectsOversizedOrPartialSnapshots() throws Exception {
        var j=data();var rows=new JsonArray();
        for(int i=0;i<2985;i++)rows.add(JsonParser.parseString("[2000,10000,0,2000,12000,0,2000,10000,1000]"));
        j.add("triangles",rows);j.addProperty("radius",32000);
        var mesh=TerrainSnapshot.parse(j,0);
        assertNotNull(mesh.raycast(new TerrainSnapshot.Point(0,10500,100),new TerrainSnapshot.Point(4000,10500,100)));
        while(rows.size()<=4096)rows.add(rows.get(0));
        assertThrows(IOException.class,()->TerrainSnapshot.parse(j,0));
        rows.remove(rows.size()-1);assertNotNull(TerrainSnapshot.parse(j,0));
        j.addProperty("complete",false);assertThrows(IOException.class,()->TerrainSnapshot.parse(j,0));
    }
    TerrainSnapshot surface(String triangles,String kinds) throws Exception {
        var json=data();json.add("triangles",JsonParser.parseString("["+triangles+"]"));json.add("kinds",JsonParser.parseString("["+kinds+"]"));return TerrainSnapshot.parse(json,0);
    }
    @Test void quantizedGroundSupportNeverRequiresTheCellAbove() throws Exception {
        var map=new ControllerMapping(new Re4Sample(0,0,0,0,0),0,80,0,0,1000);
        for(double fraction:new double[]{.005,.05,.3,.75,.99}){
            double y=fraction*1000;
            var floor=surface("[-10000,"+y+",-10000,10000,"+y+",-10000,0,"+y+",10000]","\"floor\"");
            assertTrue(floor.intersects(map,new double[]{0,80,0,1,81,1},.0005));
            assertFalse(floor.placementIntersects(map,new double[]{0,80,0,1,81,1},.0005));
            assertEquals(80+fraction,floor.groundHeight(map,.5,.5,80),1e-8);
            var aligned=map.groundAnchor(floor);
            if(fraction<=.25){assertNotNull(aligned);assertEquals(y,aligned.origin().y(),1e-7);assertEquals(80,floor.groundHeight(aligned,.5,.5,80),1e-8);}
            else assertNull(aligned); // do not adopt an overhead floor/roof
        }
    }
    @Test void supportKindDoesNotRemoveWallsCeilingsOrTerrainThroughTheTop() throws Exception {
        var map=new ControllerMapping(new Re4Sample(0,0,0,0,0),0,0,0,0,1000);
        String flat="[-1000,300,-1000,2000,300,-1000,-1000,300,2000]",wall="[500,-1000,-1000,500,2000,-1000,500,0,2000]";
        var bounds=new double[]{0,0,0,1,1,1};
        for(String kind:new String[]{"wall","other"})assertTrue(surface(flat,"\""+kind+"\"").placementIntersects(map,bounds,.0005));
        assertTrue(surface(wall,"\"floor\"").placementIntersects(map,bounds,.0005)); // invalid classification cannot bypass a vertical wall
        assertTrue(surface(flat+","+wall,"\"floor\",\"wall\"").placementIntersects(map,bounds,.0005));
        assertTrue(surface(wall,"\"wall\"").placementIntersects(map,new double[]{0,0,0,1,.25,1},.0005));
        var bad=data();bad.add("kinds",JsonParser.parseString("[]"));assertThrows(IOException.class,()->TerrainSnapshot.parse(bad,0));
    }
    @Test void slopeIsClippedToFootprintAndRoofIsNotGroundForCalibration() throws Exception {
        var map=new ControllerMapping(new Re4Sample(0,0,0,0,0),0,80,0,0,1000);
        var slope=surface("[-10000,-1800,-10000,10000,2200,-10000,0,200,10000]","\"slope\"");
        assertFalse(slope.placementIntersects(map,new double[]{0,80,0,1,81,1},.0005));
        assertTrue(slope.placementIntersects(map,new double[]{0,80,0,1,80.25,1},.0005));
        var roofs=surface("[-10000,5,-10000,10000,5,-10000,0,5,10000],[-10000,2000,-10000,10000,2000,-10000,0,2000,10000]","\"floor\",\"floor\"");
        assertEquals(5, map.groundAnchor(roofs).origin().y(),1e-8);
    }
    JsonObject data(){return JsonParser.parseString("{\"revision\":1,\"generation\":2,\"age_ms\":0,\"complete\":true,\"position\":[0,0,0],\"radius\":12000,\"triangles\":[[1000,-1000,-1000,1000,1000,-1000,1000,0,1000]]}").getAsJsonObject();}
    @Test void doubleSidedSweptRayAndCoverage() throws Exception {
        var t=TerrainSnapshot.parse(data(),1_000_000_000L);
        var hit=t.raycast(new TerrainSnapshot.Point(0,0,0),new TerrainSnapshot.Point(2000,0,0));
        assertEquals(.5,hit.fraction());assertEquals(-1,hit.normal().x());
        assertEquals(.5,t.raycast(new TerrainSnapshot.Point(2000,0,0),new TerrainSnapshot.Point(0,0,0)).fraction());
        assertNull(t.raycast(new TerrainSnapshot.Point(0,900,900),new TerrainSnapshot.Point(2000,900,900)));
        assertNull(t.raycast(new TerrainSnapshot.Point(0,0,0),new TerrainSnapshot.Point(0,0,0)));
        assertThrows(IllegalArgumentException.class,()->t.raycast(new TerrainSnapshot.Point(0,0,0),new TerrainSnapshot.Point(13000,0,0)));
        assertTrue(t.fresh(1_999_000_000L));assertFalse(t.fresh(2_000_000_000L));
    }
    @Test void RotatedGridVolumeRejectsWallButAllowsSurfaceContact() throws Exception {
        var t=TerrainSnapshot.parse(data(),0);var map=new ControllerMapping(new Re4Sample(0,0,0,0,0),0,0,0,Math.PI/2,1000);
        assertTrue(t.intersects(map,new double[]{-.5,-.5,-1.5,.5,.5,-.5},.0005));
        assertFalse(t.intersects(map,new double[]{-.5,-.5,-1,.5,.5,0},.0005));
        assertThrows(IllegalArgumentException.class,()->t.intersects(map,new double[]{0,0,20,1,1,21},0));
    }
    @Test void malformedIncompleteAndDegenerateAreNeverFreeSpace() throws Exception {
        var data=data();data.addProperty("complete",false);var incomplete=data;assertThrows(IOException.class,()->TerrainSnapshot.parse(incomplete,0));
        data.add("triangles",new JsonArray());var t=TerrainSnapshot.parse(data,0);assertFalse(t.fresh(0));assertFalse(t.covered(new TerrainSnapshot.Point(0,0,0)));
        data=data();data.addProperty("generation",1.5);var bad=data;assertThrows(IOException.class,()->TerrainSnapshot.parse(bad,0));
        data=data();data.getAsJsonArray("triangles").set(0,JsonParser.parseString("[0,0,0,0,0,0,0,0,0]"));var flat=data;assertThrows(IOException.class,()->TerrainSnapshot.parse(flat,0));
    }
}
