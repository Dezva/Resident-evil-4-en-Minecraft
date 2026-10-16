package dev.dezva.re4bridge.minecraft;
import dev.dezva.re4bridge.*;
import java.util.*;
import org.junit.jupiter.api.Test;
import org.joml.Matrix4f;
import static org.junit.jupiter.api.Assertions.*;
class WorldProjectileTests {
 @Test void vanillaVertexConsumerTransformsBeforeNativeMappingAndKeepsUvColor(){
  var map=new ControllerMapping(new Re4Sample(100,200,300,0,0),0,0,0,0,1000);
  var vertices=new ArrayList<WorldMesh.Vertex>();var out=new BridgeProjectileRenderer.Collector(vertices,map,3);
  out.vertex(new Matrix4f().translation(10,20,30),1,2,3).color(17,34,51,255).texture(.25f,.5f).light(0,0).overlay(0,0).normal(0,1,0);out.finish();
  assertEquals(1,vertices.size());var v=vertices.getFirst();assertEquals(11100,v.x());assertEquals(22200,v.y());assertEquals(33300,v.z());assertEquals(.625f,v.u());assertEquals(.75f,v.v());assertEquals(0xFF112233,v.color());
  out.finish();assertEquals(1,vertices.size());
 }
 @Test void unknownCoverageNeverLooksLikeAnEmbeddedArrowInRe4(){
  for(var reason:new String[]{"session","outside terrain coverage","terrain=caducado"})assertFalse(BridgeArrow.visibleDuringPause(reason));
  assertTrue(BridgeArrow.visibleDuringPause(null));assertTrue(BridgeArrow.visibleDuringPause("embedded"));assertTrue(BridgeArrow.visibleDuringPause("combat"));
 }
}
