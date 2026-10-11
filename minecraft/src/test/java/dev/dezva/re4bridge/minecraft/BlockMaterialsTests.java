package dev.dezva.re4bridge.minecraft;
import dev.dezva.re4bridge.BlockScene;
import java.util.List;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
import net.minecraft.util.math.Box;
class BlockMaterialsTests {
    private static class TestSprite extends net.minecraft.client.texture.Sprite {
        TestSprite(net.minecraft.client.texture.SpriteContents contents){super(net.minecraft.util.Identifier.ofVanilla("textures/atlas/blocks.png"),contents,1024,1024,64,96);}
    }
    private net.minecraft.client.texture.SpriteContents contents(net.minecraft.client.texture.NativeImage image){
        // No PNG resource exists for this atlas-generated sprite: use its decoded pixels.
        return new net.minecraft.client.texture.SpriteContents(net.minecraft.util.Identifier.ofVanilla("generated_test"),
            new net.minecraft.client.texture.SpriteDimensions(image.getWidth(),image.getHeight()),image,net.minecraft.resource.metadata.ResourceMetadata.NONE);
    }
    @Test void actualMinecraftSpritesBakeSixFacesAndBorrowPixelsWithCorrectChannels() throws Exception {
        net.minecraft.SharedConstants.createGameVersion();net.minecraft.Bootstrap.initialize();
        try(var image=new net.minecraft.client.texture.NativeImage(16,16,false)) {
            image.fillRect(0,0,16,16,0xFF332211); // NativeImage ABGR -> bridge ARGB FF112233
            var contents=contents(image);var sprite=new TestSprite(contents);
            var factory=new net.minecraft.client.render.model.BakedQuadFactory();
            var directions=new net.minecraft.util.math.Direction[]{net.minecraft.util.math.Direction.WEST,net.minecraft.util.math.Direction.EAST,net.minecraft.util.math.Direction.DOWN,net.minecraft.util.math.Direction.UP,net.minecraft.util.math.Direction.NORTH,net.minecraft.util.math.Direction.SOUTH};
            for(int i=0;i<6;i++) {
                var direction=directions[i];
                var face=new net.minecraft.client.render.model.json.ModelElementFace(direction,-1,"#all",new net.minecraft.client.render.model.json.ModelElementTexture(new float[]{0,0,16,16},0));
                var quad=factory.bake(new org.joml.Vector3f(0,0,0),new org.joml.Vector3f(16,16,16),face,sprite,direction,net.minecraft.client.render.model.ModelRotation.X0_Y0,null,true);
                var mapping=new BlockMaterials.Face(1,BlockMaterials.affine(quad,i/2)).bounds(new Box(0,0,0,1,1,1),i/2);
                assertEquals(8,mapping.uv().size());assertTrue(mapping.uv().stream().allMatch(v->v>=0 && v<=1));
                byte[] data=java.util.Base64.getDecoder().decode(BlockMaterials.compose(List.of(quad),List.of(0xFFFFFF)));
                assertEquals(0xFF112233,java.nio.ByteBuffer.wrap(data).order(java.nio.ByteOrder.LITTLE_ENDIAN).getInt());
                assertEquals(0xFF332211,image.getColor(0,0)); // borrowed image remains alive and unchanged
            }
        }
    }
    @Test void nativeSpriteTintAndTransparentLayersComposeWithoutResourceLookup() throws Exception {
        try(var base=new net.minecraft.client.texture.NativeImage(16,16,false);var overlay=new net.minecraft.client.texture.NativeImage(16,16,false)) {
            base.fillRect(0,0,16,16,0xFF0000FF);overlay.fillRect(0,0,16,16,0x8000FF00);
            var bottom=new net.minecraft.client.render.model.BakedQuad(new int[32],-1,net.minecraft.util.math.Direction.UP,new TestSprite(contents(base)),true);
            var top=new net.minecraft.client.render.model.BakedQuad(new int[32],0,net.minecraft.util.math.Direction.UP,new TestSprite(contents(overlay)),true);
            byte[] data=java.util.Base64.getDecoder().decode(BlockMaterials.compose(List.of(bottom,top),List.of(0xFFFFFF,0x00FF00)));
            assertEquals(0xFF7F8000,java.nio.ByteBuffer.wrap(data).order(java.nio.ByteOrder.LITTLE_ENDIAN).getInt());
        }
        assertEquals(0xFF003300,BlockMaterials.tint(0xFF663344,0x00FF00));
        assertEquals(0x8000FF00,BlockMaterials.over(0,0x8000FF00));
    }
    @Test void animatedSpriteUsesItsDeclaredFirstFrameAndNeverClosesTheImage() throws Exception {
        try(var image=new net.minecraft.client.texture.NativeImage(16,32,false)) {
            image.fillRect(0,0,16,16,0xFF0000FF);image.fillRect(0,16,16,16,0xFF00FF00);
            var animation=new net.minecraft.client.resource.metadata.AnimationResourceMetadata(List.of(
                new net.minecraft.client.resource.metadata.AnimationFrameResourceMetadata(1),
                new net.minecraft.client.resource.metadata.AnimationFrameResourceMetadata(0)),16,16,1,false);
            var metadata=new net.minecraft.resource.metadata.ResourceMetadata.Builder().add(net.minecraft.client.resource.metadata.AnimationResourceMetadata.READER,animation).build();
            var contents=new net.minecraft.client.texture.SpriteContents(net.minecraft.util.Identifier.ofVanilla("generated_animation"),new net.minecraft.client.texture.SpriteDimensions(16,16),image,metadata);
            var quad=new net.minecraft.client.render.model.BakedQuad(new int[32],-1,net.minecraft.util.math.Direction.UP,new TestSprite(contents),true);
            var bytes=java.util.Base64.getDecoder().decode(BlockMaterials.compose(List.of(quad),List.of(0xFFFFFF)));
            assertEquals(0xFF00FF00,java.nio.ByteBuffer.wrap(bytes).order(java.nio.ByteOrder.LITTLE_ENDIAN).getInt());
            assertEquals(0xFF0000FF,image.getColor(0,0));
        }
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
