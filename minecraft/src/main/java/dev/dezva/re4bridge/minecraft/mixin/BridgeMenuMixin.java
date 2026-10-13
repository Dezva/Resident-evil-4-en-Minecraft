package dev.dezva.re4bridge.minecraft.mixin;

import dev.dezva.re4bridge.minecraft.BridgeKeys;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.gui.screen.Screen;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(MinecraftClient.class)
abstract class BridgeMenuMixin {
    @Inject(method="setScreen",at=@At("HEAD"))
    private void re4bridge$menu(Screen next,CallbackInfo ci){BridgeKeys.screen(next);}
    @Inject(method="handleInputEvents",at=@At("HEAD"))
    private void re4bridge$scripted(CallbackInfo ci){BridgeKeys.input((MinecraftClient)(Object)this);}
}
