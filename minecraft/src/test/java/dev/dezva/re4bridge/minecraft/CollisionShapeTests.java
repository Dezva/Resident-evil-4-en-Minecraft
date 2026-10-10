package dev.dezva.re4bridge.minecraft;

import net.minecraft.Bootstrap;
import net.minecraft.SharedConstants;
import net.minecraft.block.Blocks;
import net.minecraft.block.ShapeContext;
import net.minecraft.block.enums.SlabType;
import net.minecraft.state.property.Properties;
import net.minecraft.util.math.BlockPos;
import net.minecraft.world.EmptyBlockView;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

/** Exercises Minecraft's real registered shapes, without a graphical client or RE4. */
class CollisionShapeTests {
    @BeforeAll static void bootstrapMinecraft() {
        SharedConstants.createGameVersion();
        Bootstrap.initialize();
    }

    @Test void slabsRetainTheirHalfHeightInsteadOfBecomingFullCubes() {
        for (SlabType type : new SlabType[]{SlabType.BOTTOM, SlabType.TOP, SlabType.DOUBLE}) {
            var state = Blocks.STONE_SLAB.getDefaultState().with(Properties.SLAB_TYPE, type);
            var boxes = state.getCollisionShape(EmptyBlockView.INSTANCE, BlockPos.ORIGIN, ShapeContext.absent()).getBoundingBoxes();
            assertEquals(1, boxes.size());
            var box = boxes.getFirst();
            assertEquals(type == SlabType.TOP ? 0.5 : 0, box.minY);
            assertEquals(type == SlabType.BOTTOM ? 0.5 : 1, box.maxY);
        }
    }

    @Test void fenceCanExtendBeyondTheBlockAndAirHasNoCollision() {
        var fence = Blocks.OAK_FENCE.getDefaultState().getCollisionShape(EmptyBlockView.INSTANCE,
                BlockPos.ORIGIN, ShapeContext.absent()).getBoundingBoxes();
        assertTrue(fence.stream().anyMatch(box -> box.maxY > 1));
        assertTrue(Blocks.AIR.getDefaultState().getCollisionShape(EmptyBlockView.INSTANCE,
                BlockPos.ORIGIN, ShapeContext.absent()).isEmpty());
    }
}
