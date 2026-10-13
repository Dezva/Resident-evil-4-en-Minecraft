package dev.dezva.re4bridge.minecraft.mixin;
import dev.dezva.re4bridge.minecraft.BridgeKeys;
import net.minecraft.client.Keyboard;
import net.minecraft.client.MinecraftClient;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
@Mixin(Keyboard.class)
abstract class BridgeKeyboardMixin {
    @Inject(method="onKey",at=@At("HEAD"),cancellable=true)
    private void re4bridge$mode(long window,int key,int scancode,int action,int modifiers,CallbackInfo ci){
        if(BridgeKeys.key(MinecraftClient.getInstance(),window,key,action))ci.cancel();
    }
}
