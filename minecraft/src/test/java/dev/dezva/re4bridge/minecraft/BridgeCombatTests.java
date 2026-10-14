package dev.dezva.re4bridge.minecraft;
import dev.dezva.re4bridge.*;
import com.google.gson.*;
import net.minecraft.item.*;
import net.minecraft.util.math.*;
import net.minecraft.util.hit.*;
import net.minecraft.util.shape.VoxelShapes;
import java.util.*;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
class BridgeCombatTests {
 @org.junit.jupiter.api.BeforeAll static void bootstrap(){net.minecraft.SharedConstants.createGameVersion();net.minecraft.Bootstrap.initialize();}
 private static final ControllerMapping MAP=new ControllerMapping(new Re4Sample(0,0,0,0,0),0,0,0,0,1000);
 private static final Vec3d EYE=new Vec3d(0,1,0),END=new Vec3d(3,1,0);
 private static CombatState enemies(){return new CombatState(true,true,true,7,0,1,0,0,0,0,0,List.of(new CombatState.Enemy(0,10,18,1000,1000,new TerrainSnapshot.Point(2000,0,0),250,1800)));}
 private static TerrainSnapshot terrain(boolean wall) throws Exception {
  var j=JsonParser.parseString("{\"revision\":1,\"generation\":7,\"age_ms\":0,\"complete\":true,\"position\":[0,0,0],\"radius\":12000,\"triangles\":[]}").getAsJsonObject();
  if(wall)j.add("triangles",JsonParser.parseString("[[1000,-1000,-5000,1000,5000,-5000,1000,-1000,5000],[1000,5000,5000,1000,-1000,5000,1000,5000,-5000]]"));
  return TerrainSnapshot.parse(j,0);
 }
 @Test void nativeWallAndPlacedBlockPreventHitsButAVisibleEnemyStillWinsOverABlockBehindIt() throws Exception {
  assertNotNull(BridgeCombat.select(enemies(),terrain(false),MAP,EYE,END,null));assertNull(BridgeCombat.select(enemies(),terrain(true),MAP,EYE,END,null));
  var front=VoxelShapes.fullCube().raycast(EYE,END,new BlockPos(1,0,0));assertNotNull(front);assertNull(BridgeCombat.select(enemies(),terrain(false),MAP,EYE,END,front));
  var behind=new BlockHitResult(new Vec3d(2.8,1,0),Direction.WEST,new BlockPos(2,0,0),false);
  var aim=BridgeCombat.select(enemies(),terrain(false),MAP,EYE,END,behind);assertNotNull(aim);assertTrue(aim.end().x()<2800);assertTrue(aim.end().x()>1750);
 }
 @Test void unknownTerrainCannotAuthorizeAnAttackThroughAnUnobservedWall(){
  assertThrows(IllegalArgumentException.class,()->BridgeCombat.select(enemies(),null,MAP,EYE,END,null));
  var front=new BlockHitResult(new Vec3d(1,1,0),Direction.WEST,new BlockPos(1,0,0),false);assertNull(BridgeCombat.select(enemies(),null,MAP,EYE,END,front)); // mining can still select its actual front block
 }
 @Test void vanillaSwordAndToolWearDiffersFromHeldBlocksAndFists(){
  assertEquals(1,BridgeCombat.wear(new ItemStack(Items.IRON_SWORD)));assertEquals(2,BridgeCombat.wear(new ItemStack(Items.IRON_AXE)));assertEquals(2,BridgeCombat.wear(new ItemStack(Items.DIAMOND_PICKAXE)));assertEquals(0,BridgeCombat.wear(new ItemStack(Items.DIRT)));assertEquals(0,BridgeCombat.wear(ItemStack.EMPTY));
 }
 @Test void verticalReachIsSharedWithNativeTargets() throws Exception {
  assertNull(BridgeCombat.select(enemies(),terrain(false),MAP,new Vec3d(0,4,0),new Vec3d(3,4,0),null));
 }
 @Test void realVanillaWeaponAttributesDriveBothDamageAndRecovery(){
  Item[] items={Items.WOODEN_SWORD,Items.IRON_SWORD,Items.DIAMOND_SWORD,Items.IRON_AXE};double[] expected={4,6,7,9};
  for(int i=0;i<items.length;i++){
   var damage=new net.minecraft.entity.attribute.EntityAttributeInstance(net.minecraft.entity.attribute.EntityAttributes.GENERIC_ATTACK_DAMAGE,a->{});damage.setBaseValue(1);
   var speed=new net.minecraft.entity.attribute.EntityAttributeInstance(net.minecraft.entity.attribute.EntityAttributes.GENERIC_ATTACK_SPEED,a->{});speed.setBaseValue(4);
   var modifiers=new ItemStack(items[i]).get(net.minecraft.component.DataComponentTypes.ATTRIBUTE_MODIFIERS);assertNotNull(modifiers);
   modifiers.applyModifiers(net.minecraft.entity.EquipmentSlot.MAINHAND,(attribute,modifier)->{if(attribute.equals(net.minecraft.entity.attribute.EntityAttributes.GENERIC_ATTACK_DAMAGE))damage.addTemporaryModifier(modifier);if(attribute.equals(net.minecraft.entity.attribute.EntityAttributes.GENERIC_ATTACK_SPEED))speed.addTemporaryModifier(modifier);});
   assertEquals(expected[i],CombatRequest.damage(damage.getValue(),1),1e-6);assertTrue(CombatRequest.damage(damage.getValue(),0)<damage.getValue());assertEquals(i==3?.9:1.6,speed.getValue(),1e-5);
  }
 }
}
