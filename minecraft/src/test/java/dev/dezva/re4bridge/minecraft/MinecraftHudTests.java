package dev.dezva.re4bridge.minecraft;
import dev.dezva.re4bridge.HudFrame;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Base64;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
class MinecraftHudTests {
    @Test void gpuReadbackIsFlippedAndEncodedForD3DWithoutMutatingInput(){
        var pixels=ByteBuffer.allocate(HudFrame.BYTES+4);pixels.position(4);
        // Bottom left red, top left blue, partially transparent premultiplied green.
        pixels.put(4,(byte)255);pixels.put(7,(byte)255);
        int top=4+(HudFrame.HEIGHT-1)*HudFrame.WIDTH*4;
        pixels.put(top+2,(byte)255);pixels.put(top+3,(byte)255);
        pixels.put(top+5,(byte)64);pixels.put(top+7,(byte)128);
        pixels.put(top+8,(byte)255); // fully transparent garbage must become zero
        byte[] original=pixels.array().clone();
        var output=ByteBuffer.wrap(Base64.getDecoder().decode(MinecraftHud.encode(pixels,HudFrame.WIDTH,HudFrame.HEIGHT).image())).order(ByteOrder.LITTLE_ENDIAN);
        assertEquals(0xFF0000FF,output.getInt(0));assertEquals(0x80008000,output.getInt(4));assertEquals(0,output.getInt(8));
        assertEquals(0xFFFF0000,output.getInt((HudFrame.HEIGHT-1)*HudFrame.WIDTH*4));
        assertEquals(4,pixels.position());assertArrayEquals(original,pixels.array());
    }
    @Test void rejectsWrongDimensionsAndTruncatedReadback(){
        assertThrows(IllegalArgumentException.class,()->MinecraftHud.encode(ByteBuffer.allocate(8),HudFrame.WIDTH,HudFrame.HEIGHT));
        assertThrows(IllegalArgumentException.class,()->MinecraftHud.encode(ByteBuffer.allocate(HudFrame.BYTES),128,64));
        assertThrows(IllegalArgumentException.class,()->new HudFrame("invalid"));
    }
    @Test void highResolutionTransparentLayerUsesBoundedCompressedPixels() throws Exception {
        var rgba=ByteBuffer.allocate(1366*768*4);int top=(768-1)*1366*4;
        rgba.put(top,(byte)128);rgba.put(top+3,(byte)128);
        var overlay=MinecraftHud.encodeOverlay(rgba,1366,768,true);var json=overlay.toJson();
        assertEquals(1366,json.get("width").getAsInt());assertTrue(json.get("menu").getAsBoolean());
        byte[] compressed=Base64.getDecoder().decode(overlay.image());assertTrue(compressed.length<dev.dezva.re4bridge.HudFrame.MAX_COMPRESSED);
        try(var input=new java.util.zip.InflaterInputStream(new java.io.ByteArrayInputStream(compressed))){
            byte[] raw=input.readAllBytes();assertEquals(1366*768*4,raw.length);assertEquals(0x80FF0000,ByteBuffer.wrap(raw).order(ByteOrder.LITTLE_ENDIAN).getInt());
        }
        assertThrows(IllegalArgumentException.class,()->MinecraftHud.encodeOverlay(ByteBuffer.allocate(4),32,32,false));
    }
}
