package dev.dezva.re4bridge;

import java.util.ArrayDeque;

/** Preserve arrow/confirm ordering and leave a release interval between native pulses. */
public final class NativeActionQueue {
    public record Request(int generation,int id,int kind,long createdNs){}
    private final ArrayDeque<Request> waiting=new ArrayDeque<>();
    private int generation,counter;
    private Request active;
    private long sentAt,releasedAt;
    private boolean releasing;
    public synchronized boolean offer(int gen,int kind,long now){
        if(gen<=0 || kind<0 || kind>7)throw new IllegalArgumentException("Invalid native action");
        if(generation!=gen){clear();generation=gen;counter=0;}
        if(counter==Integer.MAX_VALUE || waiting.size()==16)return false;
        waiting.addLast(new Request(gen,++counter,kind,now));return true;
    }
    public synchronized Request next(int gen,long now,int ackGeneration,int acknowledgement){
        if(gen!=generation)return null;
        if(active!=null && (now-sentAt>=500_000_000L || ackGeneration==gen && acknowledgement>=active.id && now-sentAt>=150_000_000L)){
            active=null;releasing=true;releasedAt=now;
            return null; // publish an explicit zero-action frame before the next pulse
        }
        if(releasing && now-releasedAt<30_000_000L)return null;
        releasing=false;
        if(active==null){
            while(!waiting.isEmpty() && now-waiting.peekFirst().createdNs>=1_000_000_000L)waiting.removeFirst();
            if(waiting.isEmpty())return null;
            active=waiting.removeFirst();sentAt=now;
        }
        return new Request(active.generation,active.id,active.kind,sentAt);
    }
    public synchronized void clear(){waiting.clear();active=null;releasing=false;}
}
