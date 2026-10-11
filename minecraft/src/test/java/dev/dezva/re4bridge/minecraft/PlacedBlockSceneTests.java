package dev.dezva.re4bridge.minecraft;

import dev.dezva.re4bridge.BlockPreview;
import net.minecraft.Bootstrap;
import net.minecraft.SharedConstants;
import net.minecraft.block.Blocks;
import net.minecraft.block.BlockState;
import net.minecraft.block.ShapeContext;
import net.minecraft.block.enums.SlabType;
import net.minecraft.state.property.Properties;
import net.minecraft.util.math.BlockPos;
import net.minecraft.world.EmptyBlockView;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

class PlacedBlockSceneTests {
    @BeforeAll static void bootstrapMinecraft() {SharedConstants.createGameVersion();Bootstrap.initialize();}
    private static java.util.List<net.minecraft.util.math.Box> shapes(BlockState state) {
        return state.getCollisionShape(EmptyBlockView.INSTANCE,BlockPos.ORIGIN,ShapeContext.absent()).getBoundingBoxes();
    }
    private boolean put(PlacedBlockScene scene,BlockPos pos,BlockState state) {
        // Each later attempt is anchored somewhere else: it must never move the grid.
        var nativeBox=new BlockPreview((pos.getX()%1000)*99+10,20,30,1000);
        return scene.put(new BlockPlacementProbe.Placed(pos,state,nativeBox,"overworld"),shapes(state),0);
    }
    @Test void wallPreservesSpacingAndHeightAndIndividualRemovalKeepsOriginalReference() {
        var scene=new PlacedBlockScene();var state=Blocks.STONE.getDefaultState();
        assertTrue(put(scene,BlockPos.ORIGIN,state));scene.align(0);
        assertTrue(put(scene,new BlockPos(1,0,0),state));assertTrue(put(scene,new BlockPos(0,1,0),state));
        var before=scene.snapshot();assertEquals(3,before.boxes().size());assertEquals(10,before.x());
        assertEquals(1000,before.boxes().get(1).minX()-before.boxes().get(0).minX());
        assertEquals(1000,before.boxes().get(2).minY());
        scene.update(pos -> pos.equals(BlockPos.ORIGIN)?Blocks.AIR.getDefaultState():state,pos -> shapes(state),"overworld");
        assertEquals(2,scene.size());assertEquals(10,scene.snapshot().x());
        assertEquals(500,scene.snapshot().boxes().getFirst().minX());
        scene.update(pos -> Blocks.AIR.getDefaultState(),pos -> shapes(state),"overworld");
        assertNull(scene.snapshot());assertTrue(scene.anchored());
        assertTrue(put(scene,new BlockPos(2,0,0),state));assertEquals(1500,scene.snapshot().boxes().getFirst().minX());
    }
    @Test void bottomTopAndDoubleSlabsHaveActualMinecraftHeight() {
        var scene=new PlacedBlockScene();
        var bottom=Blocks.STONE_SLAB.getDefaultState().with(Properties.SLAB_TYPE,SlabType.BOTTOM);
        assertTrue(put(scene,BlockPos.ORIGIN,bottom));scene.align(0);
        assertTrue(put(scene,new BlockPos(1,0,0),bottom.with(Properties.SLAB_TYPE,SlabType.TOP)));
        var boxes=scene.snapshot().boxes();assertEquals(500,boxes.getFirst().maxY());assertEquals(500,boxes.get(1).minY());
        var doubled=bottom.with(Properties.SLAB_TYPE,SlabType.DOUBLE);
        scene.update(pos -> doubled,pos -> shapes(doubled),"overworld");
        assertEquals(1000,scene.snapshot().boxes().getFirst().maxY());
    }
    @Test void gridYawAlignsMinecraftFacingWithRe4AndHeightAdjustmentKeepsSpacing() {
        var scene=new PlacedBlockScene();var state=Blocks.STONE.getDefaultState();
        scene.put(new BlockPlacementProbe.Placed(BlockPos.ORIGIN,state,new BlockPreview(10,20,30,1000),"overworld"),shapes(state),90);
        scene.align(.25);assertEquals(-Math.PI/2-.25,scene.snapshot().yaw(),1e-9);
        scene.height(250);assertEquals(270,scene.snapshot().y());
        scene.update(pos -> state,pos -> shapes(state),"nether");
        assertFalse(scene.anchored());assertNull(scene.snapshot());
    }
    @Test void capacityAndInvalidShapesRejectNewBlockWithoutLosingPreviousOnes() {
        var scene=new PlacedBlockScene();var state=Blocks.STONE.getDefaultState();
        for (int i=0;i<64;i++) assertTrue(put(scene,new BlockPos(i,0,0),state));
        assertFalse(put(scene,new BlockPos(64,0,0),state));assertEquals(64,scene.snapshot().boxes().size());
        scene.clear();assertTrue(put(scene,BlockPos.ORIGIN,state));
        assertFalse(put(scene,new BlockPos(2_000_000,0,0),state));assertEquals(1,scene.size());
        scene.clear();assertFalse(put(scene,BlockPos.ORIGIN,Blocks.AIR.getDefaultState()));assertFalse(scene.anchored());
    }
    @Test void changingStylePreservesFixedGridAndClearResetsIt() {
        var scene=new PlacedBlockScene();assertTrue(put(scene,BlockPos.ORIGIN,Blocks.STONE.getDefaultState()));scene.align(.25);
        var before=scene.snapshot();scene.style(1);var after=scene.snapshot();
        assertEquals(1,after.style());assertEquals(before.boxes(),after.boxes());assertEquals(before.x(),after.x());assertEquals(before.yaw(),after.yaw());
        scene.style(2);assertEquals(2,scene.snapshot().style());assertEquals(before.boxes(),scene.snapshot().boxes());
        scene.style(0);assertEquals(0,scene.snapshot().style());scene.style(1);scene.clear();assertEquals(0,scene.style());assertNull(scene.snapshot());
    }
    @Test void unloadedChunkIsNotMistakenForDestroyedBlock() {
        var scene=new PlacedBlockScene();var state=Blocks.STONE.getDefaultState();
        assertTrue(put(scene,BlockPos.ORIGIN,state));
        scene.update(pos -> {fail("Must not read an unloaded block");return null;},pos -> shapes(state),pos -> false,"overworld");
        assertEquals(1,scene.size());assertNotNull(scene.snapshot());
    }
}
