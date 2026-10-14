package dev.dezva.re4bridge.minecraft;

import java.util.concurrent.atomic.AtomicReference;
import java.util.function.BooleanSupplier;
import net.minecraft.entity.player.PlayerEntity;

/** Physics belongs to RE4 only while the exact local follower has a live lease. */
public final class EditorPhysics {
    private record Lease(Object player,long stamp,BooleanSupplier allowed,boolean grounded) {}
    private static final AtomicReference<Lease> CLIENT=new AtomicReference<>(),SERVER=new AtomicReference<>();
    private static AtomicReference<Lease> slot(boolean client){return client?CLIENT:SERVER;}
    static void follow(Object player,boolean client,long stamp,BooleanSupplier allowed){follow(player,client,stamp,allowed,true);}
    static void follow(Object player,boolean client,long stamp,BooleanSupplier allowed,boolean grounded){slot(client).set(new Lease(player,stamp,allowed,grounded));}
    public static boolean grounded(PlayerEntity player){var lease=slot(player.getWorld().isClient).get();return active(player) && lease!=null && lease.grounded;}
    static void release(Object player,boolean client){
        var slot=slot(client);var lease=slot.get();if(lease!=null && lease.player==player)slot.compareAndSet(lease,null);
    }
    static boolean active(Object player,boolean client,long now){
        var lease=slot(client).get();return lease!=null && lease.player==player && now>=lease.stamp && now-lease.stamp<200_000_000L && lease.allowed.getAsBoolean();
    }
    public static boolean active(PlayerEntity player){return BridgePlayMode.supported(player) && player.isAlive() && active(player,player.getWorld().isClient,System.nanoTime());}
    private EditorPhysics() {}
}
