package dev.dezva.re4bridge.minecraft.mixin;

import dev.dezva.re4bridge.minecraft.EditorPhysics;
import net.minecraft.entity.player.PlayerEntity;
import org.objectweb.asm.Opcodes;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(PlayerEntity.class)
abstract class EditorPhysicsMixin {
    // Vanilla resets noClip to isSpectator at the start of tick, after the
    // follower took ownership. Reapply before movement/push-out/server checks.
    @Inject(method="tick",at=@At(value="FIELD",target="Lnet/minecraft/entity/player/PlayerEntity;noClip:Z",opcode=Opcodes.PUTFIELD,shift=At.Shift.AFTER))
    private void re4bridge$editorPhysics(CallbackInfo ci){
        var player=(PlayerEntity)(Object)this;
        if(EditorPhysics.active(player))player.noClip=true;
    }
}
