package dev.dezva.re4bridge;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
import com.google.gson.*;
import java.io.IOException;
class CombatTests {
 @Test void projectileTickCarriesItsOwnIdentityWithoutIncreasingSegmentLength(){
  var r=new CombatRequest(7,2,0,10,300,1000,3,p(12000,1000,0),p(15000,1000,0),0,1,88);
  assertEquals(1,r.toJson().get("kind").getAsInt());assertEquals(88,r.toJson().get("projectile").getAsInt());
  assertThrows(IllegalArgumentException.class,()->new CombatRequest(7,2,0,10,300,1000,3,p(12000,1000,0),p(19000,1000,0),0,1,88));
  assertThrows(IllegalArgumentException.class,()->new CombatRequest(7,2,0,10,300,1000,3,p(12000,1000,0),p(15000,1000,0),0,1,0));
  assertThrows(IllegalArgumentException.class,()->new CombatRequest(7,2,0,10,300,1000,3,p(12000,1000,0),p(15000,1000,0),0,0,88));
 }
 @Test void lowTrapBodyCanBeTargetedBelowItsModelOriginWithoutLoweringLivingHitboxes() throws Exception {
  var json=state();var actor=json.getAsJsonArray("enemies").get(0).getAsJsonObject();actor.addProperty("kind",0x2A);actor.addProperty("radius",1000);
  assertNotNull(CombatState.parse(json,0).raycast(p(0,-200,0),p(3000,-200,0)));
  assertNull(CombatState.parse(json,0).raycast(p(0,-401,0),p(3000,-401,0)));
  actor.addProperty("kind",0x12);assertNull(CombatState.parse(json,0).raycast(p(0,-200,0),p(3000,-200,0)));
 }
 private static TerrainSnapshot.Point p(double x,double y,double z){return new TerrainSnapshot.Point(x,y,z);}
 private static JsonObject state(){return JsonParser.parseString("{\"available\":true,\"active\":true,\"fresh\":true,\"generation\":7,\"ack\":1,\"reason\":0,\"hp_before\":1000,\"hp_after\":950,\"hits\":1,\"kills\":0,\"enemies\":[{\"slot\":0,\"guid\":10,\"kind\":18,\"hp\":950,\"maximum\":1000,\"position\":[2000,0,0],\"radius\":250,\"height\":1600}]}").getAsJsonObject();}
 @Test void vanillaCooldownWeightsWeaponAttributeWithoutIncreasingSpamDamage(){
  assertEquals(1,CombatRequest.damage(1,1));assertEquals(6,CombatRequest.damage(6,1));assertEquals(1.2,CombatRequest.damage(6,0),1e-9);assertEquals(2.4,CombatRequest.damage(6,.5),1e-9);assertEquals(9,CombatRequest.damage(9,1));
  for(double invalid:new double[]{Double.NaN,-1,1001})assertThrows(IllegalArgumentException.class,()->CombatRequest.damage(invalid,1));
 }
 @Test void rayFindsOnlyNearestEnemyInThreeDimensionsAndExpiresLocally() throws Exception {
  var json=state();var second=json.getAsJsonArray("enemies").get(0).deepCopy().getAsJsonObject();second.addProperty("slot",2);second.add("position",JsonParser.parseString("[1000,0,0]"));json.getAsJsonArray("enemies").add(second);
  var s=CombatState.parse(json,100);var hit=s.raycast(p(0,1000,0),p(3000,1000,0));assertEquals(2,hit.enemy().slot());assertEquals(.25,hit.fraction(),1e-9);
  assertNull(s.raycast(p(0,4000,0),p(3000,4000,0)));assertNull(s.raycast(p(0,1000,0),p(-3000,1000,0)));assertTrue(s.fresh(100));assertFalse(s.fresh(200000100));
 }
 @Test void requestRejectsInvalidRangeAndPreservesNativeIdentity(){
  var r=new CombatRequest(7,1,2,0xffffffffL,300,1000,3,p(0,1000,0),p(3000,1000,0),100);assertEquals(0xffffffffL,r.toJson().get("guid").getAsLong());
  assertThrows(IllegalArgumentException.class,()->new CombatRequest(7,1,2,0,300,1000,3,p(0,0,0),p(7000,0,0),0));
 }
 @Test void parserRejectsDuplicatesNonIntegralCountersAndMalformedActorBounds(){
  var duplicate=state();duplicate.getAsJsonArray("enemies").add(duplicate.getAsJsonArray("enemies").get(0).deepCopy());assertThrows(IOException.class,()->CombatState.parse(duplicate,0));
  var counter=state();counter.addProperty("ack",1.5);assertThrows(IOException.class,()->CombatState.parse(counter,0));
  var bounds=state();bounds.getAsJsonArray("enemies").get(0).getAsJsonObject().addProperty("height",Double.NaN);assertThrows(IOException.class,()->CombatState.parse(bounds,0));
 }
 @Test void legacyHostCannotEnableReactionsAndConfirmedImpactsRetainNativeCoordinates() throws Exception {
  var json=state();assertFalse(CombatState.parse(json,0).reactionSupported());
  json.addProperty("engine_supported",true);json.add("hit_position",JsonParser.parseString("[1800,1000,0]"));json.addProperty("target_kind",35);
  var nativeState=CombatState.parse(json,0);assertTrue(nativeState.reactionSupported());assertEquals(p(1800,1000,0),nativeState.impact());assertEquals(35,nativeState.targetKind());
  json.add("hit_position",JsonParser.parseString("[0,0]"));assertThrows(IOException.class,()->CombatState.parse(json,0));
 }
}
