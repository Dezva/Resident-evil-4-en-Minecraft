package dev.dezva.re4bridge.minecraft;

import dev.dezva.re4bridge.HudFrame;
import java.nio.ByteBuffer;
import java.util.concurrent.Semaphore;
import java.util.concurrent.atomic.AtomicReference;

/** Single latest-frame queue. CPU conversion/compression never blocks input/render. */
final class OverlayEncoder implements AutoCloseable {
    private record Job(byte[] pixels,int width,int height,boolean menu,long captured){}
    private record Result(HudFrame frame,long captured){}
    private final AtomicReference<Job> pending=new AtomicReference<>();
    private final Semaphore ready=new Semaphore(0);
    private volatile boolean closed;
    private volatile Result latest;
    private volatile String failure;
    private Thread worker;
    HudFrame latest(){var image=latest;return closed || image==null || System.nanoTime()-image.captured>=500_000_000L?null:image.frame;}
    String failure(){return failure;}
    // Caller transfers exclusive ownership of the freshly copied readback array.
    void submit(byte[] pixels,int width,int height,boolean menu){
        if(closed)return;
        if(width<32 || width>HudFrame.MAX_WIDTH || height<32 || height>HudFrame.MAX_HEIGHT || pixels.length!=width*height*4)throw new IllegalArgumentException("Overlay dimensions");
        if(worker==null){worker=new Thread(this::run,"re4bridge-overlay-encoder");worker.setDaemon(true);worker.start();}
        if(pending.getAndSet(new Job(pixels,width,height,menu,System.nanoTime()))==null)ready.release();
    }
    private void run(){
        try{
            while(!closed){
                ready.acquire();if(closed)return;var job=pending.getAndSet(null);if(job==null)continue;
                var encoded=MinecraftHud.encodeOverlay(ByteBuffer.wrap(job.pixels),job.width,job.height,job.menu);
                if(!closed)latest=new Result(encoded,job.captured);
            }
        }catch(InterruptedException stop){Thread.currentThread().interrupt();}
        catch(RuntimeException error){failure="Codificacion de interfaz: "+error.getClass().getSimpleName();pending.set(null);}
    }
    @Override public void close(){closed=true;pending.set(null);latest=null;if(worker!=null)worker.interrupt();}
}
