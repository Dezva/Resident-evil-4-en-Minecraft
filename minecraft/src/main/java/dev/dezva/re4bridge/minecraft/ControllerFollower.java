package dev.dezva.re4bridge.minecraft;

import dev.dezva.re4bridge.BridgeConnection;
import dev.dezva.re4bridge.ControllerMapping;
import dev.dezva.re4bridge.TerrainSnapshot;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicBoolean;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.network.ClientPlayerEntity;
import net.minecraft.server.MinecraftServer;
import net.minecraft.server.network.ServerPlayerEntity;
import net.minecraft.util.math.Vec3d;
import net.minecraft.util.math.BlockPos;

/** Local creative editor follows RE4; no teleport packet changes mouse rotation. */
final class ControllerFollower {
    private record Task(MinecraftServer server,UUID id,String dimension,ControllerMapping.Point point,BridgeConnection bridge,boolean floor,ControllerMapping mapping,TerrainSnapshot terrain,int generation,boolean sprinting,float exhaustion,int fallHarm,boolean grounded,double groundY,long stamp) {}
    private volatile Task task;
    private final AtomicBoolean queued=new AtomicBoolean();
    private ClientPlayerEntity clientPlayer;
    private final EditorMode clientMode=new EditorMode();
    private EditorMode.Slot clientSlot;
    // Server-owned fields, accessed only in its executor.
    private ServerPlayerEntity serverPlayer;
    private final EditorMode serverMode=new EditorMode();
    private EditorMode.Slot serverSlot;
    private volatile MinecraftServer server;
    private final EditorFloor floor=new EditorFloor();
    private net.minecraft.server.world.ServerWorld floorWorld;
    private EditorFloor.World floorSlot;
    private TerrainSnapshot floorMesh;private ControllerMapping floorMap;private BlockPos floorFeet;
    private java.util.Map<Long,Integer> ground=java.util.Map.of();
    private ControllerMapping.Point lastTravel;private long travelStamp;private int travelGeneration;
    void claim(BlockPos pos){floor.claim(pos);}
    boolean helper(BlockPos pos){return floor.contains(pos);}
    private void floor(ServerPlayerEntity player,Task current){
        if(!current.floor()){floor.clear();floorWorld=null;floorSlot=null;return;}
        var world=player.getServerWorld();
        if(world!=floorWorld){floor.clear();floorWorld=world;floorSlot=new EditorFloor.World(){
            public net.minecraft.block.BlockState read(BlockPos pos){return world.getBlockState(pos);}
            public boolean loaded(BlockPos pos){return world.isChunkLoaded(pos);}
            public boolean write(BlockPos pos,net.minecraft.block.BlockState state){return world.setBlockState(pos,state,3);}
            public int bottom(){return world.getBottomY();}public int top(){return world.getTopY();}
        };}
        var feet=BlockPos.ofFloored(current.point().x(),current.groundY(),current.point().z());
        var mesh=current.terrain();var map=current.mapping();
        if(mesh==null || map==null || mesh.generation()!=current.generation() || !mesh.fresh(System.nanoTime())
            || !mesh.covered(dev.dezva.re4bridge.TerrainSnapshot.nativePoint(map,new dev.dezva.re4bridge.TerrainSnapshot.Point(current.point().x(),current.groundY()+3,current.point().z())))){floor.update(floorSlot,feet);floorMesh=null;return;}
        if(mesh!=floorMesh || !map.equals(floorMap) || !feet.equals(floorFeet)){
            var heights=new java.util.HashMap<Long,Integer>();
            for(int x=feet.getX()-EditorFloor.RADIUS;x<=feet.getX()+EditorFloor.RADIUS;x++)for(int z=feet.getZ()-EditorFloor.RADIUS;z<=feet.getZ()+EditorFloor.RADIUS;z++){
                Double height=mesh.groundHeight(map,x+.5,z+.5,feet.getY());
                if(height!=null)heights.put(BlockPos.asLong(x,0,z),net.minecraft.util.math.MathHelper.floor(height+.0001)-1);
            }
            ground=heights;floorMesh=mesh;floorMap=map;floorFeet=feet;
        }
        floor.update(floorSlot,feet,(x,z)->ground.get(BlockPos.asLong(x,0,z)));
    }
    private int physiologyGeneration,oldFallHarm;private float oldExhaustion;
    void follow(MinecraftClient client,ControllerMapping.Point point,BridgeConnection bridge,boolean support,ControllerMapping mapping,int generation){follow(client,point,bridge,support,mapping,generation,0,0,true,point.y());}
    void follow(MinecraftClient client,ControllerMapping.Point point,BridgeConnection bridge,boolean support,ControllerMapping mapping,int generation,float exhaustion,int fallHarm,boolean grounded,double groundY){
        if(!BridgePlayMode.supported(client.player) || !client.player.isAlive()){release();return;}
        if(clientPlayer!=client.player){release();clientPlayer=client.player;clientSlot=EditorMode.player(clientPlayer);}
        clientMode.take(clientSlot);
        EditorPhysics.follow(clientPlayer,true,System.nanoTime(),()->bridge.status().connected() && bridge.nativeStop()==0,grounded);
        clientPlayer.setOnGround(grounded);clientPlayer.setVelocity(Vec3d.ZERO);clientPlayer.setPosition(point.x(),point.y(),point.z());
        boolean sprinting=!MinecraftHud.menu(client) && client.options.sprintKey.isPressed() && (client.options.forwardKey.isPressed() || client.options.backKey.isPressed() || client.options.leftKey.isPressed() || client.options.rightKey.isPressed());
        server=client.getServer();task=new Task(server,clientPlayer.getUuid(),client.world.getRegistryKey().getValue().toString(),point,bridge,support,mapping,bridge.terrain(),generation,sprinting,exhaustion,fallHarm,grounded,groundY,System.nanoTime());schedule();
    }
    void release(){
        task=null;
        EditorPhysics.release(clientPlayer,true);
        clientMode.release();clientSlot=null;
        clientPlayer=null;schedule();
    }
    private void schedule(){
        var executor=server;
        if(executor==null || !queued.compareAndSet(false,true))return;
        executor.execute(()->{
            try {
                Task current=task;
                var player=current==null?null:executor.getPlayerManager().getPlayer(current.id());
                boolean valid=current!=null && current.server()==executor && BridgePlayMode.supported(player) && player.isAlive()
                    && player.getServerWorld().getRegistryKey().getValue().toString().equals(current.dimension())
                    && System.nanoTime()-current.stamp()<200_000_000L && current.bridge().status().connected() && current.bridge().nativeStop()==0;
                if(!valid || player!=serverPlayer){
                    EditorPhysics.release(serverPlayer,false);
                    if(serverPlayer!=null && serverPlayer.getServer()==executor){serverMode.release();serverPlayer.sendAbilitiesUpdate();serverSlot=null;}
                    serverPlayer=null;lastTravel=null;floor.clear();floorWorld=null;floorSlot=null;
                }
                if(valid && task==current){
                    if(serverPlayer==null){serverPlayer=player;serverSlot=EditorMode.player(player);}
                    serverMode.take(serverSlot);
                    EditorPhysics.follow(player,false,current.stamp(),()->task!=null && current.bridge().status().connected() && current.bridge().nativeStop()==0,current.grounded());
                    floor(player,current);
                    if(physiologyGeneration!=current.generation()){physiologyGeneration=current.generation();oldFallHarm=0;oldExhaustion=0;}
                    if(!player.isCreative()){
                        player.addExhaustion(Math.max(0,current.exhaustion()-oldExhaustion));
                        int damage=Math.max(0,current.fallHarm()-oldFallHarm);if(damage>0)player.damage(player.getDamageSources().fall(),damage);
                    }
                    oldExhaustion=Math.max(oldExhaustion,current.exhaustion());oldFallHarm=Math.max(oldFallHarm,current.fallHarm());
                    player.setOnGround(current.grounded());
                    if(lastTravel!=null && travelGeneration==current.generation() && !player.isCreative()){
                        double distance=Math.hypot(current.point().x()-lastTravel.x(),current.point().z()-lastTravel.z());
                        player.addExhaustion(BridgePlayMode.sprintExhaustion(distance,current.stamp()-travelStamp,current.sprinting()));
                    }
                    lastTravel=current.point();travelStamp=current.stamp();travelGeneration=current.generation();
                    // Normal movement packets retain rotation; no repeated position-look teleports.
                    player.setPosition(current.point().x(),current.point().y(),current.point().z());
                    player.setVelocity(Vec3d.ZERO);
                }
            } finally {
                queued.set(false);
                // A release racing the in-flight update must still restore server abilities.
                if(task==null && serverPlayer!=null)schedule();
            }
        });
    }
}
