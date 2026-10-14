package dev.dezva.re4bridge.minecraft.mixin;
import dev.dezva.re4bridge.minecraft.BridgeBlockEffects;
import net.minecraft.block.BlockState;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.network.ClientPlayerInteractionManager;
import net.minecraft.util.math.BlockPos;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;
@Mixin(ClientPlayerInteractionManager.class)
abstract class BridgeBreakMixin {
    @Unique private BlockState re4bridge$before;
    @Inject(method="breakBlock",at=@At("HEAD"))
    private void re4bridge$capture(BlockPos pos,CallbackInfoReturnable<Boolean> ci){var world=MinecraftClient.getInstance().world;re4bridge$before=world==null?null:world.getBlockState(pos);}
    @Inject(method="breakBlock",at=@At("RETURN"))
    private void re4bridge$broken(BlockPos pos,CallbackInfoReturnable<Boolean> ci){if(ci.getReturnValueZ())BridgeBlockEffects.broken(pos,re4bridge$before);re4bridge$before=null;}
}
