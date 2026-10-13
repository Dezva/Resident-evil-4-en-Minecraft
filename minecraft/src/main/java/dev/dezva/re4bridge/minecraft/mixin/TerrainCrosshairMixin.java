package dev.dezva.re4bridge.minecraft.mixin;
import dev.dezva.re4bridge.minecraft.TerrainInteraction;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.render.GameRenderer;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.*;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
@Mixin(GameRenderer.class)
abstract class TerrainCrosshairMixin {
 @Inject(method="updateCrosshairTarget",at=@At("TAIL"))
 private void re4bridge$terrain(float delta,CallbackInfo ci){TerrainInteraction.crosshair(MinecraftClient.getInstance(),delta);}
}
