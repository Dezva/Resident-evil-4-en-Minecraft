package dev.dezva.re4bridge.minecraft;
import dev.dezva.re4bridge.*;
import java.util.*;
import java.util.concurrent.atomic.AtomicBoolean;
import net.minecraft.client.MinecraftClient;
import net.minecraft.entity.attribute.EntityAttributes;
import net.minecraft.entity.projectile.PersistentProjectileEntity;
import net.minecraft.item.ItemStack;
import net.minecraft.component.DataComponentTypes;
import net.minecraft.registry.tag.ItemTags;
import net.minecraft.sound.*;
import net.minecraft.util.Hand;
import net.minecraft.util.math.*;

/** Native targets; the integrated Minecraft server owns weapon damage and wear. */
public final class BridgeCombat {
 private record Context(BridgeConnection bridge,ControllerMapping map,int generation,UUID player,String dimension,Set<net.minecraft.util.math.BlockPos> placed,long stamp){}
 private record Pending(Context context,CombatRequest request,ItemStack weapon,int slot,int wear,boolean strong,long lease,PersistentProjectileEntity arrow){}
 private static volatile Context context;private static volatile Pending pending;private static final AtomicBoolean busy=new AtomicBoolean();
 private static final java.util.concurrent.atomic.AtomicLong epoch=new java.util.concurrent.atomic.AtomicLong();
 private static void release(long lease){if(epoch.get()==lease)busy.set(false);}
 private static int generation,counter;
 static void bind(BridgeConnection bridge,ControllerMapping map,int gen,UUID player,String dimension,Set<net.minecraft.util.math.BlockPos> placed){context=new Context(bridge,map,gen,player,dimension,Set.copyOf(placed),System.nanoTime());bridge.combatRequested(true,map.scale());}
 public static void clear(){CombatEffects.clear();epoch.incrementAndGet();var c=context;var p=pending;context=null;pending=null;busy.set(false);if(c!=null)c.bridge.clearCombat();if(p!=null){p.context.bridge.clearCombat();retire(MinecraftClient.getInstance(),p);}}
 private static boolean active(Context c){var live=context;return c!=null && live!=null && live.bridge==c.bridge && live.generation==c.generation && live.player.equals(c.player) && System.nanoTime()-live.stamp<200_000_000L && c.bridge.status().connected() && c.bridge.nativeArmed() && c.bridge.nativeStop()==0 && !c.bridge.scripted();}
 private static TerrainSnapshot.Point point(Vec3d p){return new TerrainSnapshot.Point(p.x,p.y,p.z);}
 static int wear(ItemStack stack){return stack.getItem() instanceof net.minecraft.item.SwordItem || stack.isIn(ItemTags.SWORDS)?1:stack.contains(DataComponentTypes.TOOL)?2:0;}
 record Aim(CombatState.Enemy enemy,TerrainSnapshot.Point start,TerrainSnapshot.Point end){}
 static Aim select(CombatState state,ControllerMapping map,Vec3d eye,Vec3d end,net.minecraft.util.hit.BlockHitResult block){
  var start=TerrainSnapshot.nativePoint(map,point(eye));var finish=TerrainSnapshot.nativePoint(map,point(end));var target=state.raycast(start,finish);if(target==null)return null;
  if(block!=null && eye.squaredDistanceTo(block.getPos())<=eye.squaredDistanceTo(end)*target.fraction()*target.fraction())return null;
  // Native RE4 raycasting checks world walls and destructible geometry at impact.
  // A cached construction mesh must not hide a crate or an already-broken door.
  double limit=block==null?1:eye.distanceTo(block.getPos())/eye.distanceTo(end);
  if(limit<1)finish=start.add(finish.subtract(start).times(Math.max(0,limit-.00001)));
  return new Aim(target.enemy(),start,finish);
 }
 public static boolean attack(MinecraftClient client){
  var c=context;if(!active(c) || client.player==null || client.world==null || !client.player.isAlive() || !BridgePlayMode.supported(client.player) || !client.isInSingleplayer() || client.currentScreen!=null || client.isPaused() || !client.isWindowFocused())return false;
  var state=c.bridge.combatState();if(state==null || !state.reactionSupported() || !state.fresh(System.nanoTime()) || state.generation()!=c.generation)return false;
  double reach=Math.min(6,client.player.getEntityInteractionRange());Vec3d eye=client.player.getPos().add(0,client.player.getEyeHeight(net.minecraft.entity.EntityPose.STANDING),0),end=eye.add(client.player.getRotationVec(1).multiply(reach));
  var block=TerrainInteraction.nearestPlaced(eye,end,c.placed,p->client.world.isChunkLoaded(p)?client.world.getBlockState(p).getCollisionShape(client.world,p):net.minecraft.util.shape.VoxelShapes.empty());
  Aim aim=select(state,c.map,eye,end,block);
  if(aim==null)return false;
  if(!busy.compareAndSet(false,true))return true;
  long lease=epoch.get();
  final var rayEnd=aim.end;var start=aim.start;var server=client.getServer();var chosen=aim.enemy;
  int selected=client.player.getInventory().selectedSlot;var held=client.player.getMainHandStack().copy();
  client.player.resetLastAttackedTicks();client.player.swingHand(Hand.MAIN_HAND);
  server.execute(()->{
   try{
    var p=server.getPlayerManager().getPlayer(c.player);
    if(!active(c) || p==null || !p.isAlive() || !BridgePlayMode.supported(p) || !p.getServerWorld().getRegistryKey().getValue().toString().equals(c.dimension) || System.nanoTime()-c.stamp>=200_000_000L){release(lease);return;}
    if(p.getInventory().selectedSlot!=selected || !ItemStack.areItemsAndComponentsEqual(p.getMainHandStack(),held)){release(lease);return;}
    double cooldown=p.getAttackCooldownProgress(.5f);int damage=(int)Math.round(50*CombatRequest.damage(p.getAttributeValue(EntityAttributes.GENERIC_ATTACK_DAMAGE),cooldown));
    p.resetLastAttackedTicks();if(damage<1){release(lease);return;}
    if(generation!=c.generation){generation=c.generation;counter=0;}if(counter==Integer.MAX_VALUE){release(lease);return;}
    var request=new CombatRequest(c.generation,++counter,chosen.slot(),chosen.guid(),damage,c.map.scale(),reach,start,rayEnd,System.nanoTime());var stack=p.getMainHandStack();
    if(epoch.get()!=lease || !active(c)){release(lease);return;}
    pending=new Pending(c,request,stack,p.getInventory().selectedSlot,wear(stack),cooldown>.9,lease,null);c.bridge.combatRequest(request);
   }catch(IllegalArgumentException error){release(lease);}
  });return true;
 }
 /** Called only from the integrated server's vanilla arrow tick. Bow/crossbow
  * charging, ammunition, enchantment base damage, gravity and wear stay vanilla. */
 public static boolean projectile(PersistentProjectileEntity arrow){
  var c=context;
  if(!(arrow instanceof net.minecraft.entity.projectile.ArrowEntity) && !(arrow instanceof net.minecraft.entity.projectile.SpectralArrowEntity))return false;
  var waiting=pending;if(waiting!=null && waiting.arrow!=null && waiting.arrow.getId()==arrow.getId())return true;
  if(arrow.getWorld().isClient || !active(c) || !TerrainInteraction.managedArrow(arrow))return false;
  var owner=arrow.getOwner();
  if(!(owner instanceof net.minecraft.server.network.ServerPlayerEntity player) || !player.isAlive() || !BridgePlayMode.supported(player))return false;
  var state=c.bridge.combatState();
  if(state==null || !state.reactionSupported() || !state.fresh(System.nanoTime()) || state.generation()!=c.generation)return true;
  Vec3d start=arrow.getPos(),end=start.add(arrow.getVelocity());double speed=arrow.getVelocity().length();
  if(speed<.00001 || speed>6 || arrow.getId()<1)return false;
  var ray=new net.minecraft.world.RaycastContext(start,end,net.minecraft.world.RaycastContext.ShapeType.COLLIDER,net.minecraft.world.RaycastContext.FluidHandling.NONE,arrow);
  var block=TerrainInteraction.projectile(arrow,arrow.getWorld(),start,end,arrow.getWorld().raycast(ray));
  var aim=select(state,c.map,start,end,block.getType()==net.minecraft.util.hit.HitResult.Type.BLOCK?block:null);
  if(aim==null)return false;
  if(!busy.compareAndSet(false,true))return true;
  long lease=epoch.get();
  try{
   if(generation!=c.generation){generation=c.generation;counter=0;}
   if(counter==Integer.MAX_VALUE || !active(c)){release(lease);return true;}
   int power=0;var weapon=arrow.getWeaponStack();
   if(weapon!=null){
    var entry=player.getRegistryManager().get(net.minecraft.registry.RegistryKeys.ENCHANTMENT).getEntry(net.minecraft.enchantment.Enchantments.POWER);
    if(entry.isPresent())power=net.minecraft.enchantment.EnchantmentHelper.getLevel(entry.get(),weapon);
   }
   int damage=projectileDamage(speed,powerDamage(arrow.getDamage(),power),arrow.isCritical(),arrow.getRandom());
   if(damage<1){release(lease);return false;}
   var request=new CombatRequest(c.generation,++counter,aim.enemy.slot(),aim.enemy.guid(),damage,c.map.scale(),Math.min(6,speed+.0001),aim.start,aim.end,System.nanoTime(),1,arrow.getId());
   if(epoch.get()!=lease || !active(c)){release(lease);return true;}
   pending=new Pending(c,request,ItemStack.EMPTY,0,0,false,lease,arrow);c.bridge.combatRequest(request);return true;
  }catch(IllegalArgumentException error){release(lease);return false;}
 }
 static int projectileDamage(double speed,double base,boolean critical,net.minecraft.util.math.random.Random random){
  if(!Double.isFinite(speed+base) || speed<0 || speed>6 || base<0 || base>1000)throw new IllegalArgumentException("Invalid arrow damage");
  int amount=(int)Math.ceil((float)speed*base); // vanilla PersistentProjectileEntity uses float speed
  if(critical)amount+=random.nextInt(amount/2+2);
  return Math.min(50000,amount*50);
 }
 /** Vanilla 1.21.1 Power's target-independent add(linear(.5)). Other
  * target/status effects require a native actor adapter and are not synthesized. */
 static double powerDamage(double base,int level){if(level<0 || level>255)throw new IllegalArgumentException("Invalid Power level");return base+(level==0?0:net.minecraft.enchantment.EnchantmentLevelBasedValue.linear(.5f).getValue(level));}
 public static int projectileGeneration(){var c=context;return active(c)?c.generation:0;}
 static double visualFov(){var c=context;return c==null?50:c.bridge.verticalFov();}
 static void tick(MinecraftClient client){
  var p=pending;if(p==null)return;var state=p.context.bridge.combatState();long now=System.nanoTime();
  if(!active(p.context) || now-p.request.createdNs()>1_000_000_000L){pending=null;p.context.bridge.combatRequest(null);retire(client,p);release(p.lease);return;}
  if(state==null || !state.available() || !state.fresh() || now-state.receivedNs()>=500_000_000L || state.generation()!=p.request.generation() || state.acknowledgement()!=p.request.id())return;
  pending=null;p.context.bridge.combatRequest(null);
  if(state.reason()==0 && state.hpAfter()<state.hpBefore())CombatEffects.hit(client,p.context.map,state,p.wear==1 && p.strong);
  var server=client.getServer();if(server==null){release(p.lease);return;}
  server.execute(()->{
   try{
    if(p.arrow!=null){
     // A rejected/stale native impact cannot turn into a later hit on another actor.
     // No second ammunition or durability charge: vanilla already paid at launch.
     p.arrow.discard();
     if(state.reason()==0 && state.hpAfter()<state.hpBefore())p.arrow.getWorld().playSound(null,p.arrow.getX(),p.arrow.getY(),p.arrow.getZ(),SoundEvents.ENTITY_ARROW_HIT,SoundCategory.PLAYERS,1,1);
     return;
    }
    var player=server.getPlayerManager().getPlayer(p.context.player);if(player==null || !player.isAlive() || !active(p.context) || !player.getServerWorld().getRegistryKey().getValue().toString().equals(p.context.dimension))return;
    boolean hit=state.reason()==0 && state.hpAfter()<state.hpBefore();
    if(hit){
     if(!player.isCreative()){
      player.addExhaustion(.1f);
      if(p.wear>0 && player.getInventory().getStack(p.slot)==p.weapon){p.weapon.damage(p.wear,player.getServerWorld(),player,item->{});player.getInventory().markDirty();}
     }
     player.getServerWorld().playSound(null,player.getX(),player.getY(),player.getZ(),p.strong?SoundEvents.ENTITY_PLAYER_ATTACK_STRONG:SoundEvents.ENTITY_PLAYER_ATTACK_WEAK,SoundCategory.PLAYERS,1,1);
    }else player.getServerWorld().playSound(null,player.getX(),player.getY(),player.getZ(),SoundEvents.ENTITY_PLAYER_ATTACK_NODAMAGE,SoundCategory.PLAYERS,.5f,1);
   }finally{release(p.lease);}
  });
 }
 private static void retire(MinecraftClient client,Pending p){if(p.arrow!=null && client.getServer()!=null)client.getServer().execute(p.arrow::discard);}
 private BridgeCombat(){}
}
