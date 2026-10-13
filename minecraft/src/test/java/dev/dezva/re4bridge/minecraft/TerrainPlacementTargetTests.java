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
 @Test void ordinaryHitIsUnchangedAndNonfiniteImpactRejected(){
  var impact=new Vec3d(.1,.2,.3);assertEquals(impact,TerrainInteraction.candidateHit(impact,Direction.NORTH,BlockPos.ORIGIN).getPos());
  assertThrows(IllegalArgumentException.class,()->TerrainInteraction.candidateHit(new Vec3d(Double.NaN,0,0),Direction.UP,BlockPos.ORIGIN));
 }
}
