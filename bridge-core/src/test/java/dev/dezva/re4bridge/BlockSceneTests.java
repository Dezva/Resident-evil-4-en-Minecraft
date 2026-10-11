package dev.dezva.re4bridge;
import java.util.ArrayList;
import java.util.List;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
class BlockSceneTests {
    @Test void immutableBoundedSnapshotKeepsLocalShapeAndCommonOrigin() {
        var list=new ArrayList<BlockScene.Bounds>();
        list.add(new BlockScene.Bounds(-500,0,-500,500,500,500));
        var scene=new BlockScene(10,20,30,Math.PI/4,list);list.clear();
        assertEquals(1,scene.boxes().size());
        assertEquals(20,scene.toJson().getAsJsonArray("origin").get(1).getAsDouble());
        assertEquals(500,scene.toJson().getAsJsonArray("boxes").get(0).getAsJsonArray().get(4).getAsDouble());
        assertThrows(UnsupportedOperationException.class,()->scene.boxes().clear());
    }
    @Test void styleIsStrictAndSerializedWithoutChangingBoxes() {
        var box=new BlockScene.Bounds(0,0,0,1000,1000,1000);
        var scene=new BlockScene(1,2,3,0,List.of(box),1);
        assertEquals(1,scene.toJson().get("style").getAsInt());
        assertEquals(2,new BlockScene(1,2,3,0,List.of(box),2).toJson().get("style").getAsInt());
        assertThrows(IllegalArgumentException.class,()->new BlockScene(1,2,3,0,List.of(box),3));
        assertThrows(IllegalArgumentException.class,()->new BlockScene(1,2,3,0,List.of(box),-1));
    }
    @Test void rejectsMalformedAndUnboundedSceneBeforeTransmission() {
        var box=new BlockScene.Bounds(0,0,0,1000,1000,1000);
        assertThrows(IllegalArgumentException.class,()->new BlockScene(0,0,0,0,List.of()));
        assertThrows(IllegalArgumentException.class,()->new BlockScene(0,0,0,0,java.util.Collections.nCopies(129,box)));
        assertThrows(IllegalArgumentException.class,()->new BlockScene(Double.NaN,0,0,0,List.of(box)));
        assertThrows(IllegalArgumentException.class,()->new BlockScene(0,0,0,4,List.of(box)));
        assertThrows(IllegalArgumentException.class,()->new BlockScene.Bounds(0,0,0,0,1000,1000));
    }
    @Test void localPaletteAndFacesAreImmutableAndBounded() {
        var tile=java.util.Base64.getEncoder().encodeToString(new byte[1024]);
        var uv=new java.util.ArrayList<>(List.of(0.,0.,1.,0.,1.,1.,0.,1.));var face=new BlockScene.Face(0,uv);uv.clear();
        var box=new BlockScene.Bounds(0,0,0,1000,1000,1000);
        var scene=new BlockScene(1,2,3,0,List.of(box),3,true,List.of(tile),List.of(java.util.Collections.nCopies(6,face)));
        assertEquals(3,scene.toJson().get("style").getAsInt());assertTrue(scene.toJson().get("collisions").getAsBoolean());
        assertEquals(1368,scene.toJson().getAsJsonArray("tiles").get(0).getAsString().length());assertEquals(9,scene.toJson().getAsJsonArray("faces").get(0).getAsJsonArray().get(0).getAsJsonArray().size());
        assertThrows(UnsupportedOperationException.class,()->face.uv().clear());
        assertThrows(IllegalArgumentException.class,()->new BlockScene(1,2,3,0,List.of(box),3,false,List.of(tile),List.of(List.of(face))));
        assertThrows(IllegalArgumentException.class,()->new BlockScene.Face(64,List.of(0.,0.,1.,0.,1.,1.,0.,1.)));
        assertThrows(IllegalArgumentException.class,()->new BlockScene.Face(0,List.of(Double.NaN,0.,1.,0.,1.,1.,0.,1.)));
    }
}
