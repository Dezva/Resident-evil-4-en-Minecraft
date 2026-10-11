package dev.dezva.re4bridge;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
class BlockPreviewTests {
    @Test void boxIsAnchoredOnceAheadAtFeetHeight() {
        var sample=new Re4Sample(10,20,30,0,0);
        var box=BlockPreview.ahead(sample,1000);
        assertEquals(10,box.x());assertEquals(20,box.y());assertEquals(3030,box.z());
        var bounds=box.toJson().getAsJsonArray("bounds");
        assertEquals(6,bounds.size());assertEquals(20,bounds.get(1).getAsDouble());
        assertEquals(1020,bounds.get(4).getAsDouble());
        assertEquals(1000,bounds.get(3).getAsDouble()-bounds.get(0).getAsDouble());
    }
    @Test void nativeYawAndBoundsAreExplicit() {
        var box=BlockPreview.ahead(new Re4Sample(0,0,0,Math.PI/2,0),1000);
        assertEquals(3000,box.x(),1e-6);assertEquals(0,box.z(),1e-6);
        assertThrows(IllegalArgumentException.class,()->new BlockPreview(0,0,0,0));
        assertThrows(IllegalArgumentException.class,()->new BlockPreview(Double.NaN,0,0,1000));
    }
}
