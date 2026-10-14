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
 @Test void nativeCeilingLimitsTheSameStandingBody() throws Exception {
  var map=new ControllerMapping(new dev.dezva.re4bridge.Re4Sample(0,0,0,0,0),0,0,0,0,1000);
  var json=com.google.gson.JsonParser.parseString("{\"revision\":1,\"generation\":1,\"age_ms\":0,\"complete\":true,\"position\":[0,0,0],\"radius\":12000,\"triangles\":[[-5000,2000,-5000,10000,2000,-5000,-5000,2000,10000]]}").getAsJsonObject();
  var terrain=dev.dezva.re4bridge.TerrainSnapshot.parse(json,0);var v=new BridgeVertical();
  assertEquals(.1999,v.step(BASE,1,true,false,false,List.of(),terrain,map).y(),1e-6);
 }
}
