package dev.dezva.re4bridge.minecraft;

import dev.dezva.re4bridge.BlockPreview;
import java.util.ArrayDeque;
import java.util.function.Function;
import net.minecraft.block.Block;
import net.minecraft.block.BlockState;
import net.minecraft.util.math.BlockPos;

/** Bounded pending attempts from event and pre-input tick; repeated input cannot
 * overwrite an earlier unconfirmed placement or keep extending its deadline. */
final class BlockPlacementTracker {
    private final ArrayDeque<BlockPlacementProbe> pending = new ArrayDeque<>();
    void attempt(BlockPos pos, BlockState before, Block expected, BlockPreview box, String dimension) {
        for (var probe : pending) if (probe.sameAttempt(pos,before,expected,dimension)) return;
        if (pending.size()==16) pending.removeFirst();
        pending.addLast(new BlockPlacementProbe(pos,before,expected,box,dimension));
    }
    BlockPlacementProbe.Placed poll(Function<BlockPos,BlockState> read, String dimension) {
        var all=pollAll(read,dimension);
        return all.isEmpty()?null:all.getLast();
    }
    java.util.List<BlockPlacementProbe.Placed> pollAll(Function<BlockPos,BlockState> read,String dimension) {
        var placed=new java.util.ArrayList<BlockPlacementProbe.Placed>();
        var iterator=pending.iterator();
        while (iterator.hasNext()) {
            var probe=iterator.next();
            var confirmed=probe.poll(read,dimension);
            if (confirmed!=null) placed.add(confirmed);
            if (probe.finished()) iterator.remove();
        }
        return placed;
    }
    int size() { return pending.size(); }
    void clear() { pending.clear(); }
}
