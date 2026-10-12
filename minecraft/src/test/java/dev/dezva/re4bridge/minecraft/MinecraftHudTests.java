package dev.dezva.re4bridge.minecraft;
import dev.dezva.re4bridge.HudFrame;
import net.minecraft.client.texture.*;
import net.minecraft.util.Identifier;
import net.minecraft.resource.metadata.ResourceMetadata;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
class MinecraftHudTests {
    private static class TestSprite extends Sprite {
        TestSprite(SpriteContents contents){super(Identifier.ofVanilla("test_hud"),contents,256,256,0,0);}
    }
    @Test void nativeGuiSpriteIsBorrowedAndClippedWithoutReadingGpuOrPng(){
        try(var image=new NativeImage(16,16,false)){
            image.fillRect(0,0,16,16,0xFF332211);
            var sprite=new TestSprite(new SpriteContents(Identifier.ofVanilla("test_hud"),new SpriteDimensions(16,16),image,ResourceMetadata.NONE));
            int[] pixels=new int[8192];MinecraftHud.sprite(pixels,sprite,255,31,16,16);
            assertEquals(0xFF112233,pixels[8191]);assertEquals(0,pixels[0]);assertEquals(0xFF332211,image.getColor(0,0));
            MinecraftHud.number(pixels,64,20,20);assertTrue(java.util.Arrays.stream(pixels).anyMatch(x->x==0xFFFFFFFF));
        }
        assertThrows(IllegalArgumentException.class,()->new HudFrame("invalid"));
    }
}
