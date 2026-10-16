package dev.dezva.re4bridge.minecraft.mixin;
import dev.dezva.re4bridge.minecraft.TerrainInteraction;
import dev.dezva.re4bridge.minecraft.BridgeCombat;
import dev.dezva.re4bridge.minecraft.BridgeArrow;
import net.minecraft.entity.projectile.PersistentProjectileEntity;
import net.minecraft.world.*;
import net.minecraft.util.hit.BlockHitResult;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.*;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
@Mixin(PersistentProjectileEntity.class)
abstract class TerrainArrowMixin implements BridgeArrow {
 @Shadow protected boolean inGround;
 @Unique private boolean re4bridge$surfacePending,re4bridge$embedded;
 @Unique private int re4bridge$generation,re4bridge$heldTicks;
 @Override public boolean re4bridge$managed(){return re4bridge$generation!=0;}
 @Inject(method="onBlockHit",at=@At("TAIL"))
 private void re4bridge$embed(BlockHitResult hit,CallbackInfo ci){re4bridge$embedded=re4bridge$surfacePending;re4bridge$surfacePending=false;}
 @Inject(method="tick",at=@At("HEAD"),cancellable=true)
 private void re4bridge$unknownTerrain(CallbackInfo ci){
  var arrow=(PersistentProjectileEntity)(Object)this;
  int generation=BridgeCombat.projectileGeneration();
  if(re4bridge$generation==0 && arrow.age==0 && TerrainInteraction.managedArrow(arrow))re4bridge$generation=generation;
  if(re4bridge$generation!=0){
   if(generation!=0 && generation!=re4bridge$generation){if(!arrow.getWorld().isClient)arrow.discard();ci.cancel();return;}
   if(generation==0 || (re4bridge$embedded && inGround) || TerrainInteraction.freeze(arrow) || BridgeCombat.projectile(arrow)){
    if(++re4bridge$heldTicks>=1200 && !arrow.getWorld().isClient)arrow.discard();ci.cancel();return;
   }
  }
  re4bridge$surfacePending=false;
  if(TerrainInteraction.freeze(arrow))ci.cancel();
 }
 @Redirect(method="tick",at=@At(value="INVOKE",target="Lnet/minecraft/world/World;raycast(Lnet/minecraft/world/RaycastContext;)Lnet/minecraft/util/hit/BlockHitResult;"))
 private BlockHitResult re4bridge$terrain(World world,RaycastContext ray){var hit=TerrainInteraction.projectile((PersistentProjectileEntity)(Object)this,world,ray.getStart(),ray.getEnd(),world.raycast(ray));
  re4bridge$surfacePending=hit.getType()==net.minecraft.util.hit.HitResult.Type.BLOCK && world.getBlockState(hit.getBlockPos()).isAir();return hit;}
}
