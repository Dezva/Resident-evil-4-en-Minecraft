package dev.dezva.re4bridge.minecraft.mixin;
import dev.dezva.re4bridge.minecraft.OverlayCapture;
import net.minecraft.client.gl.Framebuffer;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
/** Keep fabulous/translucent item targets inside the active transparent capture. */
@Mixin(Framebuffer.class)
abstract class OverlayFramebufferMixin {
 @Inject(method="beginWrite",at=@At("HEAD"),cancellable=true)
 private void re4bridge$keepTarget(boolean viewport,CallbackInfo ci){if(OverlayCapture.active() && !OverlayCapture.target(this))ci.cancel();}
 @Inject(method="endWrite",at=@At("HEAD"),cancellable=true)
 private void re4bridge$keepBound(CallbackInfo ci){if(OverlayCapture.active())ci.cancel();}
}
