package dev.dezva.re4bridge.minecraft;
import org.junit.jupiter.api.Test;
import net.minecraft.network.PacketByteBuf;
import net.minecraft.util.hit.BlockHitResult;
import net.minecraft.util.math.*;
import static org.junit.jupiter.api.Assertions.*;
class TerrainPlacementTargetTests {
 // Exact location/center gate of ServerPlayNetworkHandler 1.21.1: each axis <1.0000001.
 boolean serverLocationAccepts(BlockHitResult hit){var d=hit.getPos().subtract(Vec3d.ofCenter(hit.getBlockPos()));return Math.abs(d.x)<1.0000001 && Math.abs(d.y)<1.0000001 && Math.abs(d.z)<1.0000001;}
 @Test void elevatedSlopeCellHasValidPacketPoint(){
  var cell=new BlockPos(0,65,0);var impact=new Vec3d(.2,64.3,.7);
  assertFalse(serverLocationAccepts(new BlockHitResult(impact,Direction.UP,cell,false)));
  var hit=TerrainInteraction.candidateHit(impact,Direction.UP,cell);
  assertTrue(serverLocationAccepts(hit));assertEquals(cell,hit.getBlockPos());assertEquals(Direction.UP,hit.getSide());assertEquals(65,hit.getPos().y);assertEquals(.2,hit.getPos().x);
  var buffer=new PacketByteBuf(io.netty.buffer.Unpooled.buffer());
  try {buffer.writeBlockHitResult(hit);var decoded=buffer.readBlockHitResult();assertTrue(serverLocationAccepts(decoded));assertEquals(cell,decoded.getBlockPos());assertEquals(Direction.UP,decoded.getSide());}
  finally {buffer.release();}
 }
 @Test void shiftedWallTargetsKeepCellSideAndLocationInvariant(){
  for(var side:Direction.values()){
   var cell=new BlockPos(-5,-12,9).offset(side);var hit=TerrainInteraction.candidateHit(new Vec3d(-4.7,-11.8,9.3),side,cell);
   assertEquals(cell,hit.getBlockPos());assertEquals(side,hit.getSide());assertTrue(serverLocationAccepts(hit));
   assertTrue(hit.getPos().x>=cell.getX() && hit.getPos().x<=cell.getX()+1);
   assertTrue(hit.getPos().y>=cell.getY() && hit.getPos().y<=cell.getY()+1);
   assertTrue(hit.getPos().z>=cell.getZ() && hit.getPos().z<=cell.getZ()+1);
  }
 }
 @Test void rayFromStandingMinecraftEyesSelectsFirstBlocksTopForStacking(){
  var stone=new BlockPos(0,0,2);var start=new Vec3d(.5,1.62,0);var end=new Vec3d(.5,.8,3);
  var hit=TerrainInteraction.nearestPlaced(start,end,java.util.Set.of(stone),pos->net.minecraft.util.shape.VoxelShapes.fullCube());
  assertNotNull(hit);assertEquals(stone,hit.getBlockPos());assertEquals(Direction.UP,hit.getSide());assertEquals(1,hit.getPos().y,1e-8);
  assertEquals(new BlockPos(0,1,2),hit.getBlockPos().offset(hit.getSide()));assertTrue(serverLocationAccepts(hit));
 }
 @Test void supportContactToleranceDoesNotMoveCellOrEraseWalls(){
  var box=new Box(0,0,0,1,1,1);var bounds=TerrainInteraction.placementBounds(box);
  assertEquals(.02,bounds[1]);assertEquals(1,bounds[4]);assertEquals(box.minX,bounds[0]);assertEquals(box.maxX,bounds[3]);assertEquals(box.minZ,bounds[2]);assertEquals(box.maxZ,bounds[5]);
  var thin=TerrainInteraction.placementBounds(new Box(0,0,0,1,.005,1));assertTrue(thin[1]>=0 && thin[1]<thin[4]);
 }
 @Test void smallGroundContactDoesNotRaiseAWholeBlockButWallStillRejects() throws Exception {
  String template="{\"revision\":1,\"generation\":1,\"age_ms\":0,\"complete\":true,\"position\":[0,0,0],\"radius\":12000,\"triangles\":[%s]}";
  var floor=dev.dezva.re4bridge.TerrainSnapshot.parse(com.google.gson.JsonParser.parseString(template.formatted("[-1000,5,-1000,2000,5,-1000,-1000,5,2000]")).getAsJsonObject(),0);
  var map=new dev.dezva.re4bridge.ControllerMapping(new dev.dezva.re4bridge.Re4Sample(0,0,0,0,0),0,0,0,0,1000);
  assertTrue(floor.intersects(map,new double[]{0,0,0,1,1,1},.0005));
  assertFalse(floor.intersects(map,TerrainInteraction.placementBounds(new Box(0,0,0,1,1,1)),.0005));
  var wall=dev.dezva.re4bridge.TerrainSnapshot.parse(com.google.gson.JsonParser.parseString(template.formatted("[500,-1000,-1000,500,2000,-1000,500,0,2000]")).getAsJsonObject(),0);
  assertTrue(wall.intersects(map,TerrainInteraction.placementBounds(new Box(0,0,0,1,1,1)),.0005));
 }
 @Test void ordinaryHitIsUnchangedAndNonfiniteImpactRejected(){
  var impact=new Vec3d(.1,.2,.3);assertEquals(impact,TerrainInteraction.candidateHit(impact,Direction.NORTH,BlockPos.ORIGIN).getPos());
  assertThrows(IllegalArgumentException.class,()->TerrainInteraction.candidateHit(new Vec3d(Double.NaN,0,0),Direction.UP,BlockPos.ORIGIN));
 }
}
