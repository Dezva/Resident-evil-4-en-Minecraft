package dev.dezva.re4bridge.minecraft.mixin;
import dev.dezva.re4bridge.minecraft.OverlayCapture;
import net.minecraft.client.render.GameRenderer;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
@Mixin(GameRenderer.class)
abstract class OverlayBlurMixin {
 @Inject(method="render",at=@At("TAIL"))
 private void re4bridge$renderInterface(net.minecraft.client.render.RenderTickCounter ticks,boolean tick,CallbackInfo ci){OverlayCapture.render(ticks);}
 @Inject(method="renderBlur",at=@At("HEAD"),cancellable=true)
 private void re4bridge$skipOffscreenBlur(float delta,CallbackInfo ci){if(OverlayCapture.active())ci.cancel();}
}
