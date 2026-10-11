package dev.dezva.re4bridge.minecraft;

import dev.dezva.re4bridge.BlockPreview;
import net.minecraft.Bootstrap;
import net.minecraft.SharedConstants;
import net.minecraft.block.Blocks;
import net.minecraft.util.math.BlockPos;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

class BlockPlacementTrackerTests {
    private static final BlockPreview BOX = new BlockPreview(10,20,30,BlockPreview.DEFAULT_SIZE);
    @BeforeAll static void bootstrapMinecraft() {
        SharedConstants.createGameVersion();Bootstrap.initialize();
    }
    private void stone(BlockPlacementTracker tracker,BlockPos pos,BlockPreview box) {
        tracker.attempt(pos,Blocks.AIR.getDefaultState(),Blocks.STONE,box,"overworld");
    }
    @Test void duplicateEventAndTickPreserveFirstAnchorAndConfirmOnce() {
        var tracker=new BlockPlacementTracker();
        stone(tracker,BlockPos.ORIGIN,BOX);
        stone(tracker,BlockPos.ORIGIN,new BlockPreview(999,20,30,1000));
        assertEquals(1,tracker.size());
        var placed=tracker.poll(pos -> Blocks.STONE.getDefaultState(),"overworld");
        assertNotNull(placed);assertEquals(BOX,placed.box());
        assertEquals(0,tracker.size());
        assertNull(tracker.poll(pos -> Blocks.STONE.getDefaultState(),"overworld"));
    }
    @Test void secondHandAndLaterAttemptCannotOverwriteDelayedPlacement() {
        var tracker=new BlockPlacementTracker();
        stone(tracker,BlockPos.ORIGIN,BOX);
        assertNull(tracker.poll(pos -> Blocks.AIR.getDefaultState(),"overworld"));
        tracker.attempt(BlockPos.ORIGIN,Blocks.AIR.getDefaultState(),Blocks.DIRT,BOX,"overworld");
        stone(tracker,new BlockPos(5,0,0),BOX);
        var placed=tracker.poll(pos -> pos.equals(BlockPos.ORIGIN) ? Blocks.STONE.getDefaultState()
            : Blocks.AIR.getDefaultState(),"overworld");
        assertNotNull(placed);assertEquals(BlockPos.ORIGIN,placed.pos());
        assertEquals(2,tracker.size());
    }
    @Test void heldUseDoesNotExtendDeadlineAndWrongMaterialDoesNotConfirm() {
        var tracker=new BlockPlacementTracker();
        for (int i=0;i<10;i++) {
            stone(tracker,BlockPos.ORIGIN,BOX);
            assertNull(tracker.poll(pos -> Blocks.DIRT.getDefaultState(),"overworld"));
        }
        assertEquals(0,tracker.size());
        assertNull(tracker.poll(pos -> Blocks.STONE.getDefaultState(),"overworld"));
    }
    @Test void latestConfirmationWinsAndDimensionOrClearCancelsPendingAttempts() {
        var tracker=new BlockPlacementTracker();
        stone(tracker,BlockPos.ORIGIN,BOX);
        var last=new BlockPos(1,0,0);stone(tracker,last,BOX);
        assertEquals(last,tracker.poll(pos -> Blocks.STONE.getDefaultState(),"overworld").pos());
        stone(tracker,BlockPos.ORIGIN,BOX);
        assertNull(tracker.poll(pos -> Blocks.STONE.getDefaultState(),"nether"));
        assertEquals(0,tracker.size());
        stone(tracker,BlockPos.ORIGIN,BOX);tracker.clear();
        assertNull(tracker.poll(pos -> Blocks.STONE.getDefaultState(),"overworld"));
    }
    @Test void pendingAttemptsStayBoundedDuringRepeatedInput() {
        var tracker=new BlockPlacementTracker();
        for (int i=0;i<32;i++) stone(tracker,new BlockPos(i,0,0),BOX);
        assertEquals(16,tracker.size());
        assertNull(tracker.poll(pos -> pos.getX()<16 ? Blocks.STONE.getDefaultState()
            : Blocks.AIR.getDefaultState(),"overworld"));
    }
}
