package dev.dezva.re4bridge.minecraft.mixin;
import dev.dezva.re4bridge.minecraft.BridgeCombat;
import net.minecraft.client.MinecraftClient;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.*;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;
@Mixin(MinecraftClient.class)
abstract class BridgeCombatMixin {
 @Inject(method="doAttack",at=@At("HEAD"),cancellable=true)
 private void re4bridge$melee(CallbackInfoReturnable<Boolean> ci){if(BridgeCombat.attack((MinecraftClient)(Object)this))ci.setReturnValue(true);}
}
