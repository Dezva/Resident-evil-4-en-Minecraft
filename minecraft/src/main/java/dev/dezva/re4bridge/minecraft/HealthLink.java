package dev.dezva.re4bridge.minecraft;

import dev.dezva.re4bridge.BridgeConnection;
import dev.dezva.re4bridge.HealthState;
import dev.dezva.re4bridge.HealthRequest;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicBoolean;
import net.minecraft.client.MinecraftClient;
import net.minecraft.server.MinecraftServer;

/** Minecraft's integrated server owns hearts/food. Native hits/heals are applied once. */
final class HealthLink {
    private record Task(MinecraftServer server,UUID id,String dimension,BridgeConnection bridge,int generation,boolean armed,long stamp){}
    private volatile Task task;private final AtomicBoolean queued=new AtomicBoolean();
    private UUID owner;private BridgeConnection connection;private int generation;private long player,damage,healing;
    private boolean deathApplied;
    static float nextHealth(HealthState nativeHealth,float hearts,float maximum,long damageAck,long healingAck,boolean deathApplied,boolean creative){
        if(!Float.isFinite(hearts) || !Float.isFinite(maximum) || maximum<=0)throw new IllegalArgumentException("Invalid Minecraft health");
        if(creative || nativeHealth.mode()!=1)return hearts;
        if(nativeHealth.dead() && !deathApplied)return 0;
        return Math.clamp(hearts+((float)HealthState.delta(nativeHealth.healing(),healingAck)-HealthState.delta(nativeHealth.damage(),damageAck))/HealthState.UNIT*maximum,0,maximum);
    }
    void update(MinecraftClient client,BridgeConnection bridge,int generation,boolean armed){
        if(client.player==null || client.world==null || client.getServer()==null || !client.isInSingleplayer())return;
        task=new Task(client.getServer(),client.player.getUuid(),client.world.getRegistryKey().getValue().toString(),bridge,generation,armed,System.nanoTime());schedule();
    }
    void release(BridgeConnection bridge){task=null;if(bridge!=null)bridge.healthRequest(null);}
    private void schedule(){
        var current=task;if(current==null || !queued.compareAndSet(false,true))return;
        current.server().execute(()->{
            try{
                var t=task;if(t==null || t.server()!=current.server() || System.nanoTime()-t.stamp()>500_000_000L)return;
                var p=t.server().getPlayerManager().getPlayer(t.id());
                if(p==null || !p.getServerWorld().getRegistryKey().getValue().toString().equals(t.dimension()) || !BridgePlayMode.supported(p) || !t.bridge().status().connected())return;
                if(!t.id().equals(owner) || connection!=t.bridge() || generation!=t.generation()){
                    owner=t.id();connection=t.bridge();generation=t.generation();player=damage=healing=0;deathApplied=false;
                }
                var nativeHealth=t.bridge().healthState();
                if(nativeHealth!=null && nativeHealth.available() && nativeHealth.fresh() && nativeHealth.generation()==generation && nativeHealth.player()!=0
                    && (nativeHealth.active() || nativeHealth.dead())){
                    if(player!=0 && player!=nativeHealth.player())return; // never bind another native actor implicitly
                    player=nativeHealth.player();
                    long hurt=HealthState.delta(nativeHealth.damage(),damage);HealthState.delta(nativeHealth.healing(),healing);
                    if(!p.isCreative() && nativeHealth.mode()==1 && p.isAlive()){
                        float next=nextHealth(nativeHealth,p.getHealth(),p.getMaxHealth(),damage,healing,deathApplied,p.isCreative());
                        if(next<=0)p.damage(p.getDamageSources().genericKill(),p.getHealth()+p.getAbsorptionAmount()+1);
                        else {p.setHealth(next);if(hurt>0){p.addExhaustion(.1f);p.hurtTime=10;p.getServerWorld().sendEntityStatus(p,(byte)2);}}
                    }
                    damage=nativeHealth.damage();healing=nativeHealth.healing();
                    if(nativeHealth.dead())deathApplied=true;
                }
                if(task!=t || !t.armed() || t.bridge().nativeStop()!=0){t.bridge().healthRequest(null);return;}
                t.bridge().healthRequest(new HealthRequest(generation,p.isCreative()?2:1,Math.clamp(p.getHealth()/p.getMaxHealth(),0,1),damage,healing,System.nanoTime()));
            }catch(IllegalArgumentException error){current.bridge().healthRequest(null);}
            finally{queued.set(false);if(task!=current && task!=null)schedule();}
        });
    }
}
