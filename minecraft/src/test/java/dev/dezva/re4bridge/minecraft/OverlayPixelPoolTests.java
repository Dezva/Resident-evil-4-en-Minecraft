package dev.dezva.re4bridge.minecraft;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
class OverlayPixelPoolTests {
    @Test void inFlightBuffersAreNotReusedAndDifferentSizeBuffersAreRetired(){
        var pool=new OverlayPixelPool();var a=pool.acquire(32);var b=pool.acquire(32);
        assertNotSame(a,b);pool.recycle(a);assertSame(a,pool.acquire(32));
        pool.recycle(b);assertEquals(64,pool.acquire(64).length);
        pool.recycle(a);assertEquals(64,pool.acquire(64).length);
        pool.close();pool.recycle(a);assertThrows(IllegalStateException.class,()->pool.acquire(32));
    }
    @Test void conversionScratchDoesNotLeaveGhostPixelsWhenTheHandMoves() throws Exception {
        var raw=new byte[32*32*4];java.util.Arrays.fill(raw,(byte)255);
        var result=MinecraftHud.encodeOverlay(java.nio.ByteBuffer.allocate(raw.length),32,32,false,raw);
        try(var zip=new java.util.zip.InflaterInputStream(new java.io.ByteArrayInputStream(java.util.Base64.getDecoder().decode(result.image())))){
            assertArrayEquals(new byte[raw.length],zip.readAllBytes());
        }
    }
}
