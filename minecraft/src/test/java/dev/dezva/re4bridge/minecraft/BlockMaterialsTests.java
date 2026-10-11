package dev.dezva.re4bridge.minecraft;
import dev.dezva.re4bridge.BlockScene;
import java.util.List;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
import net.minecraft.util.math.Box;
class BlockMaterialsTests {
    @Test void pngColorsAndTintStayArgbAndTransparentLayersCompose() throws Exception {
        var image=new java.awt.image.BufferedImage(2,2,java.awt.image.BufferedImage.TYPE_INT_ARGB);
        image.setRGB(0,0,0xFF112233);image.setRGB(1,1,0x80445566);
        var buffer=new java.io.ByteArrayOutputStream();javax.imageio.ImageIO.write(image,"PNG",buffer);
        var decoded=BlockMaterials.readPng(buffer.toByteArray());assertEquals(0xFF112233,decoded.getRGB(0,0));assertEquals(0x80445566,decoded.getRGB(1,1));
        assertEquals(0xFF003300,BlockMaterials.tint(0xFF663344,0x00FF00));
        assertEquals(0xFF112233,BlockMaterials.over(0xFF112233,0));assertEquals(0xFF445566,BlockMaterials.over(0xFF112233,0xFF445566));
        assertEquals(0x8000FF00,BlockMaterials.over(0,0x8000FF00));
    }
    @Test void corruptOrOversizedImagesAreRejectedBeforeDecode() throws Exception {
        assertThrows(java.io.IOException.class,()->BlockMaterials.readPng(new byte[40]));
        var image=new java.awt.image.BufferedImage(1,1,java.awt.image.BufferedImage.TYPE_INT_ARGB);var stream=new java.io.ByteArrayOutputStream();javax.imageio.ImageIO.write(image,"PNG",stream);
        byte[] bytes=stream.toByteArray();java.nio.ByteBuffer.wrap(bytes).putInt(16,8192);assertThrows(java.io.IOException.class,()->BlockMaterials.readPng(bytes));
    }
    @Test void faceUvOrientationAndSlabCropUseModelMapping() {
        // Local face axes for +X are Y,Z; texture V decreases towards the top.
        double[][] quad={{0,0,0,1},{1,0,0,0},{1,1,1,0},{0,1,1,1}};
        double[] map=BlockMaterials.affine(quad);var face=new BlockMaterials.Face(2,map);
        var full=face.bounds(new Box(0,0,0,1,1,1),0);var slab=face.bounds(new Box(0,0,0,1,.5,1),0);
        assertEquals(List.of(0.,1.,0.,0.,1.,0.,1.,1.),full.uv());
        assertEquals(List.of(0.,1.,0.,.5,1.,.5,1.,1.),slab.uv());assertEquals(2,slab.tile());
        assertThrows(IllegalArgumentException.class,()->BlockMaterials.affine(new double[][]{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}));
    }
    @Test void fallbackPaletteIsBoundedAndCleared() {
        var materials=new BlockMaterials();assertEquals(1,materials.tiles().size());
        assertEquals(1024,java.util.Base64.getDecoder().decode(materials.tiles().getFirst()).length);materials.clear();assertEquals(0,materials.missing());
    }
}
