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
        assertThrows(IllegalArgumentException.class,()->new BlockScene(1,2,3,0,List.of(box),2));
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
}
