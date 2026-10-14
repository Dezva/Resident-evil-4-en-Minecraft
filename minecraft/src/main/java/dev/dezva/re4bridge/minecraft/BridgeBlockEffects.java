package dev.dezva.re4bridge.minecraft;
import net.minecraft.block.BlockState;
import net.minecraft.util.math.BlockPos;
/** Client prediction uses the same successful break event as vanilla particles. */
public final class BridgeBlockEffects {
    static java.util.function.BiConsumer<BlockPos,BlockState> broken=(p,s)->{};
    public static void broken(BlockPos pos,BlockState state){if(state!=null && !state.isAir())broken.accept(pos,state);}
    private BridgeBlockEffects(){}
}
