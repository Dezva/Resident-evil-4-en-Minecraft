package dev.dezva.re4bridge.minecraft;
import dev.dezva.re4bridge.*;
import java.util.*;
import java.nio.*;
import net.minecraft.util.math.BlockPos;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
class BlockEffectsTests {
 static String tile(int color){var b=ByteBuffer.allocate(1024).order(ByteOrder.LITTLE_ENDIAN);for(int i=0;i<256;i++)b.putInt(color);return Base64.getEncoder().encodeToString(b.array());}
 @Test void cracksMultiplyRealPixelsKeepAlphaAndClearStageRestoresBase(){
  var original=tile(0x80406080);int[] crack=new int[256];Arrays.fill(crack,0xFFFFFFFF);crack[0]=0xFF000000;
  var output=ByteBuffer.wrap(Base64.getDecoder().decode(BlockMaterials.crackTile(original,crack))).order(ByteOrder.LITTLE_ENDIAN);
  assertEquals(0x80000000,output.getInt(0));assertEquals(0x80406080,output.getInt(4));assertEquals(0x80406080,ByteBuffer.wrap(Base64.getDecoder().decode(original)).order(ByteOrder.LITTLE_ENDIAN).getInt());
 }
 @Test void lastBlockDebrisIsVisibleFiniteBoundedAndHasNoColliderUntilItExpires(){
  var particles=new BlockParticles();var p=BlockPos.ORIGIN;var origin=new BlockPreview(10,20,30,1000);
  for(int i=0;i<100;i++)particles.burst(p,tile(0xFF778899));
  var scene=particles.decorate(null,p,origin,.7,0);assertNotNull(scene);assertEquals(24,scene.boxes().size());assertEquals(0,scene.colliderCount());assertFalse(scene.collisions());
  assertEquals(0,scene.toJson().get("collider_count").getAsInt());for(var b:scene.boxes())assertTrue(b.maxY()>b.minY() && Double.isFinite(b.minX()+b.minY()+b.minZ()));
  for(int i=0;i<100;i++)particles.tick(false);assertNotNull(particles.decorate(null,p,origin,.7,0));
  for(int i=0;i<20;i++)particles.tick(true);assertNull(particles.decorate(null,p,origin,.7,0));
 }
 @Test void debrisKeepsSolidCollisionPrefixAndSourceGeometryUnchanged(){
  var box=new BlockScene.Bounds(-500,0,-500,500,1000,500);var f=new BlockScene.Face(0,List.of(0.,0.,1.,0.,1.,1.,0.,1.));
  var original=new BlockScene(0,0,0,0,List.of(box),3,true,List.of(tile(0xFF112233)),List.of(Collections.nCopies(6,f)));
  var particles=new BlockParticles();particles.burst(BlockPos.ORIGIN,tile(0xFF112233));var scene=particles.decorate(original,BlockPos.ORIGIN,new BlockPreview(0,0,0,1000),0,0);
  assertTrue(scene.collisions());assertEquals(1,scene.colliderCount());assertEquals(box,scene.boxes().getFirst());assertEquals(1,original.boxes().size());
 }
}
