package dev.dezva.re4bridge.minecraft;

import dev.dezva.re4bridge.BridgeConnection;
import dev.dezva.re4bridge.ControllerMapping;
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
    private record Task(MinecraftServer server,UUID id,String dimension,ControllerMapping.Point point,BridgeConnection bridge,boolean floor,long stamp) {}
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
        floor.update(floorSlot,BlockPos.ofFloored(current.point().x(),current.point().y(),current.point().z()));
    }
    void follow(MinecraftClient client,ControllerMapping.Point point,BridgeConnection bridge,boolean support){
        if(!client.player.isCreative()){release();return;}
        if(clientPlayer!=client.player){release();clientPlayer=client.player;clientSlot=EditorMode.player(clientPlayer);}
        clientMode.take(clientSlot);
        clientPlayer.setVelocity(Vec3d.ZERO);clientPlayer.setPosition(point.x(),point.y(),point.z());
        server=client.getServer();task=new Task(server,clientPlayer.getUuid(),client.world.getRegistryKey().getValue().toString(),point,bridge,support,System.nanoTime());schedule();
    }
    void release(){
        task=null;
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
                boolean valid=current!=null && current.server()==executor && player!=null && player.isCreative()
                    && player.getServerWorld().getRegistryKey().getValue().toString().equals(current.dimension())
                    && System.nanoTime()-current.stamp()<200_000_000L && current.bridge().status().connected() && current.bridge().nativeStop()==0;
                if(!valid || player!=serverPlayer){
                    if(serverPlayer!=null && serverPlayer.getServer()==executor){serverMode.release();serverPlayer.sendAbilitiesUpdate();serverSlot=null;}
                    serverPlayer=null;floor.clear();floorWorld=null;floorSlot=null;
                }
                if(valid && task==current){
                    if(serverPlayer==null){serverPlayer=player;serverSlot=EditorMode.player(player);}
                    serverMode.take(serverSlot);
                    floor(player,current);
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
