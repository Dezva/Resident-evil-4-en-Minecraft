package dev.dezva.re4bridge.minecraft;
import org.junit.jupiter.api.Test;
import org.objectweb.asm.*;
import org.objectweb.asm.tree.*;
import static org.junit.jupiter.api.Assertions.*;
class TerrainMixinTests {
 @Test void pinnedMinecraftTargetsExistIncludingSweptArrowRay() throws Exception {
  net.minecraft.SharedConstants.createGameVersion();net.minecraft.Bootstrap.initialize();
  var renderer=Class.forName("net.minecraft.client.render.GameRenderer",false,getClass().getClassLoader());
  assertNotNull(renderer.getDeclaredMethod("updateCrosshairTarget",float.class));
  var item=Class.forName("net.minecraft.item.BlockItem",false,getClass().getClassLoader());
  assertNotNull(item.getDeclaredMethod("canPlace",net.minecraft.item.ItemPlacementContext.class,net.minecraft.block.BlockState.class));
  var arrow=Class.forName("net.minecraft.entity.projectile.PersistentProjectileEntity",false,getClass().getClassLoader());
  assertNotNull(arrow.getDeclaredField("inGround"));assertNotNull(arrow.getDeclaredMethod("onBlockHit",net.minecraft.util.hit.BlockHitResult.class));
  try(var input=arrow.getResourceAsStream("PersistentProjectileEntity.class")){
   assertNotNull(input);var tree=new ClassNode();new ClassReader(input).accept(tree,0);
   var tick=tree.methods.stream().filter(m->m.name.equals("tick") && m.desc.equals("()V")).findFirst().orElseThrow();
   long rays=0;for(var node:tick.instructions)if(node instanceof MethodInsnNode call && call.name.equals("raycast") && call.desc.equals("(Lnet/minecraft/world/RaycastContext;)Lnet/minecraft/util/hit/BlockHitResult;"))rays++;
   assertEquals(1,rays,"Redirect must match exactly one swept ray in the pinned version");
  }
 }
}
