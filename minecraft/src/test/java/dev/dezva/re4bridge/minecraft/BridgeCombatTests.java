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
 @Test void vanillaArrowSpeedBaseDamageAndCriticalBonusUseOneFiftyHpConversion(){
  var random=net.minecraft.util.math.random.Random.create(42);
  assertEquals(300,BridgeCombat.projectileDamage(3,2,false,random));
  assertEquals(350,BridgeCombat.projectileDamage(3.15,2,false,random));
  assertEquals(100,BridgeCombat.projectileDamage(.8,2,false,random));
  assertEquals(600,BridgeCombat.projectileDamage(3,4,false,random));
  assertEquals(2,BridgeCombat.powerDamage(2,0));assertEquals(2.5,BridgeCombat.powerDamage(2,1));assertEquals(4.5,BridgeCombat.powerDamage(2,5));
  assertEquals(700,BridgeCombat.projectileDamage(3,BridgeCombat.powerDamage(2,5),false,random));
  for(int i=0;i<100;i++){int critical=BridgeCombat.projectileDamage(3,2,true,random);assertTrue(critical>=300 && critical<=500 && critical%50==0);}
  assertThrows(IllegalArgumentException.class,()->BridgeCombat.projectileDamage(Double.NaN,2,false,random));
  assertThrows(IllegalArgumentException.class,()->BridgeCombat.projectileDamage(7,2,false,random));
 }
 @Test void powerMatchesThePinnedGamesRegisteredDamageEffect(){
  var registries=net.minecraft.registry.BuiltinRegistries.createWrapperLookup();
  var power=registries.getWrapperOrThrow(net.minecraft.registry.RegistryKeys.ENCHANTMENT).getOrThrow(net.minecraft.enchantment.Enchantments.POWER).value();
  var effects=power.effects().get(net.minecraft.component.EnchantmentEffectComponentTypes.DAMAGE);assertNotNull(effects);assertEquals(1,effects.size());
  for(int level=1;level<=5;level++)assertEquals(effects.getFirst().effect().apply(level,net.minecraft.util.math.random.Random.create(42),2),BridgeCombat.powerDamage(2,level));
 }
 @Test void anArrowOnlySelectsTargetsAlongItsCurrentSegmentAndStopsAtEarlierTerrain(){
  var state=new CombatState(true,true,true,7,0,1,0,0,0,0,0,List.of(new CombatState.Enemy(0,10,18,1000,1000,new TerrainSnapshot.Point(14000,0,0),250,1800)),true,null,18);
  var from=new Vec3d(12,1,0);var to=new Vec3d(15,1,0);
  assertNotNull(BridgeCombat.select(state,MAP,from,to,null));
  assertNull(BridgeCombat.select(state,MAP,EYE,END,null));
  var wall=new BlockHitResult(new Vec3d(13,1,0),Direction.WEST,new BlockPos(13,0,0),false);
  assertNull(BridgeCombat.select(state,MAP,from,to,wall));
 }
 @org.junit.jupiter.api.BeforeAll static void bootstrap(){net.minecraft.SharedConstants.createGameVersion();net.minecraft.Bootstrap.initialize();}
 private static final ControllerMapping MAP=new ControllerMapping(new Re4Sample(0,0,0,0,0),0,0,0,0,1000);
 private static final Vec3d EYE=new Vec3d(0,1,0),END=new Vec3d(3,1,0);
 private static CombatState enemies(){return new CombatState(true,true,true,7,0,1,0,0,0,0,0,List.of(new CombatState.Enemy(0,10,18,1000,1000,new TerrainSnapshot.Point(2000,0,0),250,1800)),true,null,18);}
 @Test void placedBlocksPreventHitsButAVisibleEnemyStillWinsOverABlockBehindIt() throws Exception {
  assertNotNull(BridgeCombat.select(enemies(),MAP,EYE,END,null));
  var front=VoxelShapes.fullCube().raycast(EYE,END,new BlockPos(1,0,0));assertNotNull(front);assertNull(BridgeCombat.select(enemies(),MAP,EYE,END,front));
  var behind=new BlockHitResult(new Vec3d(2.8,1,0),Direction.WEST,new BlockPos(2,0,0),false);
  var aim=BridgeCombat.select(enemies(),MAP,EYE,END,behind);assertNotNull(aim);assertTrue(aim.end().x()<2800);assertTrue(aim.end().x()>1750);
 }
 @Test void nativeRayRemainsAuthoritativeWhenTheConstructionMeshIsUnavailable(){
  assertNotNull(BridgeCombat.select(enemies(),MAP,EYE,END,null));
  var front=new BlockHitResult(new Vec3d(1,1,0),Direction.WEST,new BlockPos(1,0,0),false);assertNull(BridgeCombat.select(enemies(),MAP,EYE,END,front)); // mining can still select its actual front block
 }
 @Test void vanillaSwordAndToolWearDiffersFromHeldBlocksAndFists(){
  assertEquals(1,BridgeCombat.wear(new ItemStack(Items.IRON_SWORD)));assertEquals(2,BridgeCombat.wear(new ItemStack(Items.IRON_AXE)));assertEquals(2,BridgeCombat.wear(new ItemStack(Items.DIAMOND_PICKAXE)));assertEquals(0,BridgeCombat.wear(new ItemStack(Items.DIRT)));assertEquals(0,BridgeCombat.wear(ItemStack.EMPTY));
 }
 @Test void verticalReachIsSharedWithNativeTargets() throws Exception {
  assertNull(BridgeCombat.select(enemies(),MAP,new Vec3d(0,4,0),new Vec3d(3,4,0),null));
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
 @Test void confirmedDamageUsesBoundedVanillaParticlesAndDoesNotGiveHeartsToCrates(){
  assertTrue(CombatEffects.living(0x12));assertTrue(CombatEffects.living(0x23));assertFalse(CombatEffects.living(0x2A));assertFalse(CombatEffects.living(0x53));assertFalse(CombatEffects.living(0x58));
  assertEquals(1,CombatEffects.hearts(.2));assertEquals(3,CombatEffects.hearts(7));assertEquals(8,CombatEffects.hearts(1000));
 }
}
