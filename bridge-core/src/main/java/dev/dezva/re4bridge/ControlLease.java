package dev.dezva.re4bridge;

/** Input stops at 200 ms; a short client stall can recover while the native
 * session receives only a frozen, zero-input menu frame. Never renew the tick. */
public final class ControlLease {
    public enum Phase { LIVE, SUSPENDED, EXPIRED }
    public static Phase phase(long now,long tick,boolean armed,boolean menu){
        if(now<tick)return Phase.EXPIRED;
        long age=now-tick;
        if(age<(menu?1_000_000_000L:200_000_000L))return Phase.LIVE;
        return armed && !menu && age<1_000_000_000L?Phase.SUSPENDED:Phase.EXPIRED;
    }
    private ControlLease(){}
}
