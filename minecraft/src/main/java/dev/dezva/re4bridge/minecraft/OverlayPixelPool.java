package dev.dezva.re4bridge.minecraft;
import java.util.ArrayDeque;

/** Exclusive ownership passes render -> encoder -> pool. No pending image can be reused. */
final class OverlayPixelPool implements AutoCloseable {
    private final ArrayDeque<byte[]> free=new ArrayDeque<>();
    private int size;private boolean closed;
    synchronized byte[] acquire(int bytes){
        if(closed)throw new IllegalStateException("Closed overlay pool");
        if(bytes<=0 || bytes>dev.dezva.re4bridge.HudFrame.MAX_WIDTH*dev.dezva.re4bridge.HudFrame.MAX_HEIGHT*4)throw new IllegalArgumentException("Invalid pixel capacity");
        if(bytes!=size){free.clear();size=bytes;}
        return free.isEmpty()?new byte[bytes]:free.removeFirst();
    }
    synchronized void recycle(byte[] bytes){if(!closed && bytes.length==size && free.size()<4)free.addLast(bytes);}
    @Override public synchronized void close(){closed=true;free.clear();}
}
