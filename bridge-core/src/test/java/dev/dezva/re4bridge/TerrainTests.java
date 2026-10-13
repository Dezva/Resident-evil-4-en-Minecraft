package dev.dezva.re4bridge;
import com.google.gson.*;
import org.junit.jupiter.api.Test;
import java.io.IOException;
import static org.junit.jupiter.api.Assertions.*;
class TerrainTests {
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
