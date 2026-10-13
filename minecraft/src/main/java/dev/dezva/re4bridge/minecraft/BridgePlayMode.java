package dev.dezva.re4bridge.minecraft;
import net.minecraft.entity.player.PlayerEntity;

final class BridgePlayMode {
    static boolean supported(PlayerEntity player){return player!=null && !player.isSpectator() && player.getAbilities().allowModifyWorld;}
    static net.minecraft.world.Difficulty survivalDifficulty(net.minecraft.world.Difficulty difficulty){return difficulty==net.minecraft.world.Difficulty.PEACEFUL?net.minecraft.world.Difficulty.NORMAL:difficulty;}
    static void survival(net.minecraft.server.network.ServerPlayerEntity player){
        if(player.isCreative())return;
        var server=player.getServer();var difficulty=server.getSaveProperties().getDifficulty();
        if(survivalDifficulty(difficulty)!=difficulty)server.setDifficulty(survivalDifficulty(difficulty),true);
    }
    // Vanilla sprint exhaustion: 0.1 per horizontal meter. Actual native travel,
    // not held keys against a wall; reject discontinuities and stale follower steps.
    static float sprintExhaustion(double distance,long elapsed,boolean sprinting){
        return sprinting && Double.isFinite(distance) && distance>=0 && distance<=8 && elapsed>0 && elapsed<200_000_000L?(float)(distance*.1):0;
    }
    private BridgePlayMode(){}
}
