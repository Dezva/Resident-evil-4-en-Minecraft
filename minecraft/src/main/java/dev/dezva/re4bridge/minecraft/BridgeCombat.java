package dev.dezva.re4bridge.minecraft;
import dev.dezva.re4bridge.*;
import java.util.*;
import java.util.concurrent.atomic.AtomicBoolean;
import net.minecraft.client.MinecraftClient;
import net.minecraft.entity.attribute.EntityAttributes;
import net.minecraft.item.ItemStack;
import net.minecraft.component.DataComponentTypes;
import net.minecraft.registry.tag.ItemTags;
import net.minecraft.sound.*;
import net.minecraft.text.Text;
import net.minecraft.util.Hand;
import net.minecraft.util.math.*;

/** Native targets; the integrated Minecraft server owns weapon damage and wear. */
public final class BridgeCombat {
 private record Context(BridgeConnection bridge,ControllerMapping map,int generation,UUID player,String dimension,Set<net.minecraft.util.math.BlockPos> placed,long stamp){}
 private record Pending(Context context,CombatRequest request,ItemStack weapon,int slot,int wear,boolean strong,long lease){}
 private static volatile Context context;private static volatile Pending pending;private static final AtomicBoolean busy=new AtomicBoolean();
 private static final java.util.concurrent.atomic.AtomicLong epoch=new java.util.concurrent.atomic.AtomicLong();
 private static void release(long lease){if(epoch.get()==lease)busy.set(false);}
 private static int generation,counter;private static long messageAt;
 static void bind(BridgeConnection bridge,ControllerMapping map,int gen,UUID player,String dimension,Set<net.minecraft.util.math.BlockPos> placed){context=new Context(bridge,map,gen,player,dimension,Set.copyOf(placed),System.nanoTime());bridge.combatRequested(true,map.scale());}
 public static void clear(){epoch.incrementAndGet();var c=context;var p=pending;context=null;pending=null;busy.set(false);if(c!=null)c.bridge.clearCombat();if(p!=null)p.context.bridge.clearCombat();}
 private static boolean active(Context c){var live=context;return c!=null && live!=null && live.bridge==c.bridge && live.generation==c.generation && live.player.equals(c.player) && System.nanoTime()-live.stamp<200_000_000L && c.bridge.status().connected() && c.bridge.nativeArmed() && c.bridge.nativeStop()==0 && !c.bridge.scripted();}
 private static TerrainSnapshot.Point point(Vec3d p){return new TerrainSnapshot.Point(p.x,p.y,p.z);}
 static int wear(ItemStack stack){return stack.getItem() instanceof net.minecraft.item.SwordItem || stack.isIn(ItemTags.SWORDS)?1:stack.contains(DataComponentTypes.TOOL)?2:0;}
 record Aim(CombatState.Enemy enemy,TerrainSnapshot.Point start,TerrainSnapshot.Point end){}
 static Aim select(CombatState state,TerrainSnapshot mesh,ControllerMapping map,Vec3d eye,Vec3d end,net.minecraft.util.hit.BlockHitResult block){
  var start=TerrainSnapshot.nativePoint(map,point(eye));var finish=TerrainSnapshot.nativePoint(map,point(end));var target=state.raycast(start,finish);if(target==null)return null;
  if(block!=null && eye.squaredDistanceTo(block.getPos())<=eye.squaredDistanceTo(end)*target.fraction()*target.fraction())return null;
  if(mesh==null || !mesh.covered(start) || !mesh.covered(finish))throw new IllegalArgumentException("Unknown combat terrain");
  var wall=mesh.raycast(start,finish);if(wall!=null && wall.fraction()<=target.fraction()+.00001)return null;
  double limit=wall==null?1:wall.fraction();if(block!=null)limit=Math.min(limit,eye.distanceTo(block.getPos())/eye.distanceTo(end));
  if(limit<1)finish=start.add(finish.subtract(start).times(Math.max(0,limit-.00001)));
  return new Aim(target.enemy(),start,finish);
 }
 public static boolean attack(MinecraftClient client){
  var c=context;if(!active(c) || client.player==null || client.world==null || !client.player.isAlive() || !BridgePlayMode.supported(client.player) || !client.isInSingleplayer() || client.currentScreen!=null || client.isPaused() || !client.isWindowFocused())return false;
  var state=c.bridge.combatState();if(state==null || !state.fresh(System.nanoTime()) || state.generation()!=c.generation)return false;
  double reach=Math.min(6,client.player.getEntityInteractionRange());Vec3d eye=client.player.getPos().add(0,client.player.getEyeHeight(net.minecraft.entity.EntityPose.STANDING),0),end=eye.add(client.player.getRotationVec(1).multiply(reach));
  var block=TerrainInteraction.nearestPlaced(eye,end,c.placed,p->client.world.isChunkLoaded(p)?client.world.getBlockState(p).getCollisionShape(client.world,p):net.minecraft.util.shape.VoxelShapes.empty());
  var mesh=c.bridge.combatTerrain();if(mesh==null || mesh.generation()!=c.generation || !mesh.fresh(System.nanoTime()))mesh=c.bridge.terrain();if(mesh!=null && (mesh.generation()!=c.generation || !mesh.fresh(System.nanoTime())))mesh=null;
  Aim aim;
  try{aim=select(state,mesh,c.map,eye,end,block);}catch(IllegalArgumentException unknown){
   if(System.nanoTime()-messageAt>2_000_000_000L){messageAt=System.nanoTime();client.player.sendMessage(Text.literal("Combate: esperando geometria RE4 para comprobar paredes."),true);}return false;
  }
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
    pending=new Pending(c,request,stack,p.getInventory().selectedSlot,wear(stack),cooldown>.9,lease);c.bridge.combatRequest(request);
   }catch(IllegalArgumentException error){release(lease);}
  });return true;
 }
 static void tick(MinecraftClient client){
  var p=pending;if(p==null)return;var state=p.context.bridge.combatState();long now=System.nanoTime();
  if(!active(p.context) || now-p.request.createdNs()>1_000_000_000L){pending=null;p.context.bridge.combatRequest(null);release(p.lease);return;}
  if(state==null || !state.available() || !state.fresh() || now-state.receivedNs()>=500_000_000L || state.generation()!=p.request.generation() || state.acknowledgement()!=p.request.id())return;
  pending=null;p.context.bridge.combatRequest(null);
  var server=client.getServer();if(server==null){release(p.lease);return;}
  server.execute(()->{
   try{
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
 private BridgeCombat(){}
}
