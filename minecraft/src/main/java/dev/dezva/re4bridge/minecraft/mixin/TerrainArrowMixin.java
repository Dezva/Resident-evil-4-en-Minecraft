package dev.dezva.re4bridge.minecraft.mixin;
import dev.dezva.re4bridge.minecraft.TerrainInteraction;
import dev.dezva.re4bridge.minecraft.BridgeCombat;
import dev.dezva.re4bridge.minecraft.BridgeArrow;
import dev.dezva.re4bridge.minecraft.ProjectileTrace;
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
 @Unique private boolean re4bridge$spawnChecked,re4bridge$rendered;
 @Unique private boolean re4bridge$visible=true;
 @Unique private boolean re4bridge$nativeMiss;
 @Override public void re4bridge$nativeMiss(){re4bridge$nativeMiss=true;}
 @Override public boolean re4bridge$takeNativeMiss(){boolean value=re4bridge$nativeMiss;re4bridge$nativeMiss=false;return value;}
 @Override public boolean re4bridge$visible(){return re4bridge$visible;}
 @Override public boolean re4bridge$managed(){return re4bridge$generation!=0;}
 @Override public boolean re4bridge$markRendered(){if(re4bridge$rendered)return false;re4bridge$rendered=true;return true;}
 @Inject(method="onBlockHit",at=@At("TAIL"))
 private void re4bridge$embed(BlockHitResult hit,CallbackInfo ci){re4bridge$embedded=re4bridge$surfacePending;re4bridge$surfacePending=false;}
 @Inject(method="tick",at=@At("HEAD"),cancellable=true)
 private void re4bridge$unknownTerrain(CallbackInfo ci){
  var arrow=(PersistentProjectileEntity)(Object)this;
  int generation=BridgeCombat.projectileGeneration();
  if(!re4bridge$spawnChecked){
   re4bridge$spawnChecked=true;
   boolean owned=TerrainInteraction.managedArrow(arrow);
   re4bridge$generation=BridgeArrow.launchGeneration(arrow.age,generation,owned);
   if(owned)ProjectileTrace.event(generation,"spawn",arrow.getId(),"age="+arrow.age+" client="+arrow.getWorld().isClient+" tracked="+re4bridge$managed());
  }
  if(re4bridge$generation!=0){
   if(generation!=0 && generation!=re4bridge$generation){if(!arrow.getWorld().isClient)arrow.discard();ci.cancel();return;}
   String hold=generation==0?"session":(re4bridge$embedded && inGround)?"embedded":TerrainInteraction.freeze(arrow)?TerrainInteraction.projectilePauseReason(arrow):BridgeCombat.projectile(arrow)?"combat":null;
   re4bridge$visible=BridgeArrow.visibleDuringPause(hold);
   if(hold!=null){
    if(re4bridge$heldTicks==0)ProjectileTrace.event(re4bridge$generation,"hold",arrow.getId(),hold);
    if(++re4bridge$heldTicks>=1200 && !arrow.getWorld().isClient)arrow.discard();ci.cancel();return;
   }
  }
  re4bridge$heldTicks=0;
  re4bridge$surfacePending=false;
  if(TerrainInteraction.freeze(arrow))ci.cancel();
 }
 @Redirect(method="tick",at=@At(value="INVOKE",target="Lnet/minecraft/world/World;raycast(Lnet/minecraft/world/RaycastContext;)Lnet/minecraft/util/hit/BlockHitResult;"))
 private BlockHitResult re4bridge$terrain(World world,RaycastContext ray){var hit=TerrainInteraction.projectile((PersistentProjectileEntity)(Object)this,world,ray.getStart(),ray.getEnd(),world.raycast(ray));
  re4bridge$surfacePending=hit.getType()==net.minecraft.util.hit.HitResult.Type.BLOCK && world.getBlockState(hit.getBlockPos()).isAir();return hit;}
}
