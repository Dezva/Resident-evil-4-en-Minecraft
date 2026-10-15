package dev.dezva.re4bridge.minecraft.mixin;

import dev.dezva.re4bridge.minecraft.BridgeKeys;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.gui.screen.Screen;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

@Mixin(MinecraftClient.class)
abstract class BridgeMenuMixin {
    @Inject(method="getFramerateLimit",at=@At("RETURN"),cancellable=true)
    private void re4bridge$frameBudget(CallbackInfoReturnable<Integer> ci){ci.setReturnValue(BridgeKeys.frameLimit(ci.getReturnValueI()));}
    @Inject(method="setScreen",at=@At("HEAD"))
    private void re4bridge$menu(Screen next,CallbackInfo ci){BridgeKeys.screen(next);}
    @Inject(method="onWindowFocusChanged",at=@At("HEAD"))
    private void re4bridge$focus(boolean focused,CallbackInfo ci){BridgeKeys.focus(focused);}
    @Inject(method="handleInputEvents",at=@At("HEAD"))
    private void re4bridge$scripted(CallbackInfo ci){var client=(MinecraftClient)(Object)this;BridgeKeys.input(client);dev.dezva.re4bridge.minecraft.TerrainInteraction.crosshair(client,1);}
}
