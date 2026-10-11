package dev.dezva.re4bridge.minecraft;

import dev.dezva.re4bridge.BlockPreview;
import java.util.function.Function;
import net.minecraft.block.Block;
import net.minecraft.block.BlockState;
import net.minecraft.util.math.BlockPos;

/** Confirms an attempted placement through a subsequent world-state change. */
final class BlockPlacementProbe {
    record Placed(BlockPos pos, BlockState state, BlockPreview box, String dimension) {}
    private final BlockPos pos;
    private final BlockState before;
    private final Block expected;
    private final BlockPreview box;
    private final String dimension;
    private int remaining = 10;

    BlockPlacementProbe(BlockPos pos, BlockState before, Block expected,
            BlockPreview box, String dimension) {
        this.pos = pos.toImmutable(); this.before = before; this.expected = expected;
        this.box = box; this.dimension = dimension;
    }
    Placed poll(Function<BlockPos, BlockState> read, String currentDimension) {
        if (remaining == 0) return null;
        if (!dimension.equals(currentDimension)) { remaining = 0; return null; }
        --remaining;
        BlockState state = read.apply(pos);
        if (!state.equals(before) && state.isOf(expected) && !state.isAir()) {
            remaining = 0;
            return new Placed(pos, state, box, dimension);
        }
        return null;
    }
    boolean finished() { return remaining == 0; }
    boolean sameAttempt(BlockPos candidate, BlockState state, Block block, String world) {
        return pos.equals(candidate) && before.equals(state) && expected==block && dimension.equals(world);
    }
}
