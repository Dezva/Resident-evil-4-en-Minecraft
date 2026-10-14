package dev.dezva.re4bridge.minecraft;
import dev.dezva.re4bridge.ControllerMapping;
import net.minecraft.util.math.Box;
import java.util.*;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
class BridgeVerticalTests {
 private static final ControllerMapping.Point BASE=new ControllerMapping.Point(.5,0,.5);
 @Test void vanillaJumpHeightReturnsToGroundAndMenuFreezesVelocity(){
  var v=new BridgeVertical();double maximum=0;
  for(int i=0;i<30;i++){var p=v.step(BASE,1,i==0,false,false,List.of(),null,null);maximum=Math.max(maximum,p.y());if(i==3){double height=p.y();for(int j=0;j<100;j++)assertEquals(height,v.step(BASE,1,true,false,true,List.of(),null,null).y());}}
  assertEquals(1.252203352512,maximum,1e-8);assertTrue(v.grounded());assertEquals(0,v.point().y());assertEquals(.05,v.exhaustion(),1e-6);assertEquals(0,v.fallHarm());
 }
 @Test void jumpPlaceBelowLandAndRepeatBuildsAPillarAndBreakingItCausesFallDamage(){
  var v=new BridgeVertical();var boxes=new ArrayList<Box>();int built=0;
  for(int i=0;i<500 && built<6;i++){
   var p=v.step(BASE,1,true,true,false,boxes,null,null);
   if(p.y()>built+1.0001){boxes.add(new Box(0,built,0,1,built+1,1));built++;}
  }
  assertEquals(6,built);for(int i=0;i<25;i++)v.step(BASE,1,false,false,false,boxes,null,null);
  assertEquals(6,v.point().y());assertTrue(v.grounded());assertEquals(1.2,v.exhaustion(),1e-6);
  boxes.clear();for(int i=0;i<100;i++)v.step(BASE,1,false,false,false,boxes,null,null);
  assertEquals(0,v.point().y());assertEquals(3,v.fallHarm());int damage=v.fallHarm();for(int i=0;i<100;i++)v.step(BASE,1,false,false,false,boxes,null,null);assertEquals(damage,v.fallHarm());
 }
 @Test void placedCeilingStopsHeadAndNewGenerationClearsVerticalOwnership(){
  var v=new BridgeVertical();var ceiling=List.of(new Box(0,2,0,1,3,1));
  assertEquals(.2,v.step(BASE,1,true,false,false,ceiling,null,null).y(),1e-6);
  for(int i=0;i<20;i++)assertTrue(v.step(BASE,1,false,false,false,ceiling,null,null).y()<=.200001);
  v.step(BASE,2,false,false,false,List.of(),null,null);assertEquals(0,v.point().y());assertEquals(0,v.exhaustion());
 }
 @Test void unknownHeadCoverageWaitsInsteadOfThrowing() throws Exception {
  var map=new ControllerMapping(new dev.dezva.re4bridge.Re4Sample(0,0,0,0,0),0,0,0,0,1000);
  var j=com.google.gson.JsonParser.parseString("{\"revision\":1,\"generation\":1,\"age_ms\":0,\"complete\":true,\"position\":[0,0,0],\"radius\":12000,\"triangles\":[]}").getAsJsonObject();
  var mesh=dev.dezva.re4bridge.TerrainSnapshot.parse(j,0);var v=new BridgeVertical();var boxes=new ArrayList<Box>();int built=0;
  for(int i=0;i<1000;i++){
   var p=v.step(BASE,1,true,false,false,boxes,mesh,map);
   if(p.y()>built+1.0001){boxes.add(new Box(0,built,0,1,built+1,1));built++;}
  }
  assertTrue(v.point().y()<=10.200001);assertTrue(built>=9);
 }
 @Test void climbingPastTheOldTerrainVolumeWaitsForANewMapWithoutStopping() throws Exception {
  var map=new ControllerMapping(new dev.dezva.re4bridge.Re4Sample(0,0,0,0,0),0,0,0,0,1000);
  var j=com.google.gson.JsonParser.parseString("{\"revision\":1,\"generation\":1,\"age_ms\":0,\"complete\":true,\"position\":[0,0,0],\"radius\":12000,\"triangles\":[]}").getAsJsonObject();
  var mesh=dev.dezva.re4bridge.TerrainSnapshot.parse(j,0);var v=new BridgeVertical();var boxes=new ArrayList<Box>();int built=0;
  for(int i=0;i<1500 && built<24;i++){
   var p=v.step(BASE,1,true,false,false,boxes,mesh,map);
   if(p.y()>built+1.0001){boxes.add(new Box(0,built,0,1,built+1,1));built++;}
   if(i%10==0){var pos=new com.google.gson.JsonArray();pos.add(0);pos.add(p.y()*1000);pos.add(0);j.add("position",pos);mesh=dev.dezva.re4bridge.TerrainSnapshot.parse(j,0);}
  }
  assertEquals(24,built);assertTrue(v.point().y()>24);
 }
 @Test void nativeCeilingLimitsTheSameStandingBody() throws Exception {
  var map=new ControllerMapping(new dev.dezva.re4bridge.Re4Sample(0,0,0,0,0),0,0,0,0,1000);
  var json=com.google.gson.JsonParser.parseString("{\"revision\":1,\"generation\":1,\"age_ms\":0,\"complete\":true,\"position\":[0,0,0],\"radius\":12000,\"triangles\":[[-5000,2000,-5000,10000,2000,-5000,-5000,2000,10000]]}").getAsJsonObject();
  var terrain=dev.dezva.re4bridge.TerrainSnapshot.parse(json,0);var v=new BridgeVertical();
  assertEquals(.1999,v.step(BASE,1,true,false,false,List.of(),terrain,map).y(),1e-6);
 }
 private static dev.dezva.re4bridge.TerrainSnapshot village() throws Exception {
  var j=com.google.gson.JsonParser.parseString("{\"revision\":1,\"generation\":1,\"age_ms\":0,\"complete\":true,\"position\":[0,0,0],\"radius\":12000,\"triangles\":[[-10000,0,-10000,10000,0,-10000,-10000,0,10000],[10000,0,10000,-10000,0,10000,10000,0,-10000],[1000,2000,-2000,4000,2000,-2000,1000,2000,4000],[4000,2000,4000,1000,2000,4000,4000,2000,-2000]],\"kinds\":[\"floor\",\"floor\",\"floor\",\"floor\"]}").getAsJsonObject();
  return dev.dezva.re4bridge.TerrainSnapshot.parse(j,0);
 }
 private static ControllerMapping grid(){return new ControllerMapping(new dev.dezva.re4bridge.Re4Sample(0,0,0,0,0),0,0,0,0,1000);}
 @Test void realActorHeightFeedbackDoesNotBecomeAFloatingFloor(){
  var v=new BridgeVertical();ControllerMapping.Point nativePose=BASE;double peak=0;
  for(int i=0;i<50;i++){
   var p=v.step(nativePose,1,i==0,false,false,List.of(),null,null);peak=Math.max(peak,p.y());
   assertEquals(0,v.groundY());nativePose=p; // the next RE4 sample now contains the previous command's real height
  }
  assertEquals(1.252203352512,peak,1e-8);assertEquals(0,v.point().y());assertTrue(v.grounded());
 }
 @Test void pillarCanReachAndLandOnNativePlatformThenWalkOffIt() throws Exception {
  var v=new BridgeVertical();var boxes=new ArrayList<Box>();int built=0;var terrain=village();var map=grid();var base=BASE;
  for(int i=0;i<250 && built<3;i++){
   var p=v.step(base,1,true,false,false,boxes,terrain,map);base=p;
   if(p.y()>built+1.0001){boxes.add(new Box(0,built,0,1,built+1,1));built++;}
  }
  assertEquals(3,built);for(int i=0;i<30;i++){base=v.step(base,1,false,false,false,boxes,terrain,map);}assertEquals(3,base.y());
  base=new ControllerMapping.Point(2,base.y(),.5);
  for(int i=0;i<40;i++)base=v.step(base,1,false,false,false,boxes,terrain,map);
  assertEquals(2,base.y(),1e-8);assertEquals(2,v.groundY(),1e-8);assertEquals(2000,map.toNative(base.x(),base.y(),base.z()).y(),1e-8);assertTrue(v.grounded());
  base=new ControllerMapping.Point(5,base.y(),.5);
  for(int i=0;i<40;i++)base=v.step(base,1,false,false,false,boxes,terrain,map);
  assertEquals(0,base.y(),1e-8);assertTrue(v.grounded());
 }
 @Test void slopingNativeSampleCannotPushTheFeetIntoPlacedSupport(){
  var v=new BridgeVertical();var boxes=new ArrayList<Box>();
  for(int i=0;i<60;i++){
   var p=v.step(BASE,1,i==0,false,false,boxes,null,null);if(p.y()>1.0001 && boxes.isEmpty())boxes.add(new Box(0,0,0,1,1,1));
  }
  assertEquals(1,v.point().y());
  for(int i=0;i<40;i++)assertEquals(1,v.step(new ControllerMapping.Point(.5+i%2*.03,i%2==0?-.18:.09,.5),1,false,false,false,boxes,null,null).y());
 }
 @Test void roofAboveFeetIsNotUsedAsGround() throws Exception {
  var v=new BridgeVertical();var base=new ControllerMapping.Point(2,0,.5);
  for(int i=0;i<20;i++)assertEquals(0,v.step(base,1,false,false,false,List.of(),village(),grid()).y());
 }
 @Test void missingNativeSnapshotHoldsTheFeetAndResumesTheSameJump() throws Exception {
  var v=new BridgeVertical();var map=grid();var terrain=village();
  assertEquals(0,v.step(BASE,1,true,false,false,List.of(),null,map).y());
  assertEquals(.42,v.step(BASE,1,true,false,false,List.of(),terrain,map).y(),1e-8);
  for(int i=0;i<40;i++)assertEquals(.42,v.step(BASE,1,false,false,false,List.of(),null,map).y(),1e-8);
  assertEquals(.7532,v.step(BASE,1,false,false,false,List.of(),terrain,map).y(),1e-8);
 }
}
