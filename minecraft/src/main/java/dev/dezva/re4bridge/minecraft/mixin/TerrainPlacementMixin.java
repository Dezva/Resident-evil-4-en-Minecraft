package dev.dezva.re4bridge.minecraft.mixin;
import dev.dezva.re4bridge.minecraft.TerrainInteraction;
import net.minecraft.item.*;
import net.minecraft.block.BlockState;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.*;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;
@Mixin(BlockItem.class)
abstract class TerrainPlacementMixin {
 @Inject(method="canPlace",at=@At("HEAD"),cancellable=true)
 private void re4bridge$terrain(ItemPlacementContext placement,BlockState state,CallbackInfoReturnable<Boolean> ci){if(!TerrainInteraction.placement(placement,state))ci.setReturnValue(false);}
}
