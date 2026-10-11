package dev.dezva.re4bridge.minecraft;

import dev.dezva.re4bridge.BlockPreview;
import net.minecraft.Bootstrap;
import net.minecraft.SharedConstants;
import net.minecraft.block.Blocks;
import net.minecraft.util.math.BlockPos;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

class BlockPlacementProbeTests {
    @BeforeAll static void bootstrapMinecraft() {
        SharedConstants.createGameVersion();Bootstrap.initialize();
    }
    private BlockPlacementProbe probe() {
        return new BlockPlacementProbe(BlockPos.ORIGIN,Blocks.AIR.getDefaultState(),Blocks.STONE,
            new BlockPreview(10,20,30,100),"overworld");
    }
    @Test void confirmsWorldChangeOnceAndRetainsNativeAnchor() {
        var probe=probe();
        assertNull(probe.poll(pos -> Blocks.AIR.getDefaultState(),"overworld"));
        var result=probe.poll(pos -> Blocks.STONE.getDefaultState(),"overworld");
        assertNotNull(result);assertEquals(BlockPos.ORIGIN,result.pos());
        assertEquals(new BlockPreview(10,20,30,100),result.box());
        assertNull(probe.poll(pos -> Blocks.STONE.getDefaultState(),"overworld"));
        assertTrue(probe.finished());
    }
    @Test void failedWrongItemOrUnchangedPlacementNeverCreatesPreview() {
        var probe=probe();
        for (int i=0;i<10;i++) assertNull(probe.poll(pos -> Blocks.DIRT.getDefaultState(),"overworld"));
        assertTrue(probe.finished());
        assertNull(probe.poll(pos -> Blocks.STONE.getDefaultState(),"overworld"));
        var unchanged=new BlockPlacementProbe(BlockPos.ORIGIN,Blocks.STONE.getDefaultState(),Blocks.STONE,
            new BlockPreview(10,20,30,100),"overworld");
        assertNull(unchanged.poll(pos -> Blocks.STONE.getDefaultState(),"overworld"));
    }
    @Test void dimensionChangeCancelsPendingPlacement() {
        var probe=probe();
        assertNull(probe.poll(pos -> Blocks.STONE.getDefaultState(),"nether"));
        assertTrue(probe.finished());
        assertNull(probe.poll(pos -> Blocks.STONE.getDefaultState(),"overworld"));
    }
}
