package dev.dezva.re4bridge.minecraft;
import java.util.HashMap;
import net.minecraft.Bootstrap;
import net.minecraft.SharedConstants;
import net.minecraft.block.BlockState;
import net.minecraft.block.Blocks;
import net.minecraft.util.math.BlockPos;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
class EditorFloorTests {
    @BeforeAll static void bootstrap(){SharedConstants.createGameVersion();Bootstrap.initialize();}
    static class World implements EditorFloor.World {
        final HashMap<BlockPos,BlockState> blocks=new HashMap<>();int writes;boolean loaded=true;
        public BlockState read(BlockPos p){return blocks.getOrDefault(p,Blocks.AIR.getDefaultState());}
        public boolean loaded(BlockPos p){return loaded;}
        public boolean write(BlockPos p,BlockState state){writes++;if(state.isAir())blocks.remove(p);else blocks.put(p,state);return true;}
        public int bottom(){return -64;}public int top(){return 320;}
    }
    @Test void followsNativeHeightInSkyWithoutShiftingExistingBuildsOrKeepingATrail(){
        assertEquals(80,EditorFloor.anchorHeight(64,64,320));
        assertEquals(120,EditorFloor.anchorHeight(120,64,320));
        assertThrows(IllegalArgumentException.class,()->EditorFloor.anchorHeight(300,310,320));
        var world=new World();var floor=new EditorFloor();var stone=new BlockPos(1,79,1);world.blocks.put(stone,Blocks.STONE.getDefaultState());
        floor.update(world,new BlockPos(0,80,0));assertEquals(121,world.blocks.size());assertFalse(floor.contains(stone));
        int writes=world.writes;for(int i=0;i<100;i++)floor.update(world,new BlockPos(0,80,0));assertEquals(writes,world.writes);
        var user=new BlockPos(0,79,0);world.blocks.put(user,Blocks.DIAMOND_BLOCK.getDefaultState());
        floor.update(world,new BlockPos(1,78,0));assertEquals(123,world.blocks.size());assertEquals(Blocks.STONE.getDefaultState(),world.read(stone));
        for(int x=2;x<400;x++){floor.update(world,new BlockPos(x,76,0));assertEquals(123,world.blocks.size());}
        floor.clear();assertEquals(2,world.blocks.size());assertEquals(Blocks.DIAMOND_BLOCK.getDefaultState(),world.read(user));
    }
    @Test void aBrokenHelperReplacedByPlayerGlassIsNeverDeletedOrExportFiltered(){
        var world=new World();var floor=new EditorFloor();var point=new BlockPos(0,80,0);var ground=point.down();
        floor.update(world,point);floor.claim(ground);assertFalse(floor.contains(ground));
        world.write(ground,Blocks.AIR.getDefaultState());floor.update(world,point);assertTrue(world.read(ground).isAir());
        world.write(ground,Blocks.GLASS.getDefaultState());floor.update(world,point);floor.clear();assertEquals(1,world.blocks.size());
        assertEquals(Blocks.GLASS.getDefaultState(),world.read(ground));
    }
    @Test void worldSwitchUnloadedCellsAndHeightLimitsDoNotCreateTerrainOrDeleteForeignGlass(){
        var a=new World();var b=new World();var floor=new EditorFloor();var foreign=new BlockPos(0,79,0);a.blocks.put(foreign,Blocks.GLASS.getDefaultState());
        a.loaded=false;floor.update(a,new BlockPos(0,80,0));assertEquals(1,a.blocks.size());
        a.loaded=true;floor.update(a,new BlockPos(0,80,0));assertFalse(floor.contains(foreign));
        floor.update(b,new BlockPos(0,80,0));assertEquals(1,a.blocks.size());assertEquals(121,b.blocks.size());
        floor.update(b,new BlockPos(0,320,0));assertTrue(b.blocks.isEmpty());floor.clear();
    }
    @Test void supportFollowsNativeGroundCellInsteadOfTruncatedActorFeet() throws Exception {
        var json=com.google.gson.JsonParser.parseString("{\"revision\":1,\"generation\":1,\"age_ms\":0,\"complete\":true,\"position\":[0,0,0],\"radius\":12000,\"triangles\":[[-10000,300,-10000,10000,300,-10000,0,300,10000]],\"kinds\":[\"floor\"]}").getAsJsonObject();
        var mesh=dev.dezva.re4bridge.TerrainSnapshot.parse(json,0);
        var map=new dev.dezva.re4bridge.ControllerMapping(new dev.dezva.re4bridge.Re4Sample(0,0,0,0,0),0,80,0,0,1000);
        var world=new World();var floor=new EditorFloor();
        // Actor can be slightly below the collision surface; this must not lower the proxy floor.
        floor.update(world,new BlockPos(0,79,0),(x,z)->{var y=mesh.groundHeight(map,x+.5,z+.5,79);return y==null?null:(int)Math.floor(y+.0001)-1;});
        var ground=new BlockPos(0,79,0);assertTrue(floor.contains(ground));assertFalse(floor.contains(ground.down()));
        var candidate=new BlockPos(0,80,0);assertTrue(world.read(candidate).isAir());
        world.write(candidate,Blocks.STONE.getDefaultState());
        floor.update(world,new BlockPos(0,79,0),(x,z)->79);assertEquals(Blocks.STONE.getDefaultState(),world.read(candidate));
        floor.clear();assertEquals(Blocks.STONE.getDefaultState(),world.read(candidate));assertEquals(1,world.blocks.size());
    }
}
