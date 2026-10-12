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
    private final OverlayPixelPool pixelPool=new OverlayPixelPool();
    byte[] acquire(int bytes){return pixelPool.acquire(bytes);}
    void recycle(byte[] bytes){pixelPool.recycle(bytes);}
    private volatile boolean closed;
    private volatile Result latest;
    private volatile String failure;
    private volatile long encoded,dropped,lastEncodeNs;
    long droppedFrames(){return dropped;}
    String status(){var image=latest;return "codificadas="+encoded+"; descartadas="+dropped+"; encode-ms="+(lastEncodeNs/1_000_000)+"; edad-ms="+(image==null?-1:(System.nanoTime()-image.captured)/1_000_000);}
    private Thread worker;
    HudFrame latest(){var image=latest;return closed || image==null || System.nanoTime()-image.captured>=500_000_000L?null:image.frame;}
    String failure(){return failure;}
    // Caller transfers exclusive ownership of the freshly copied readback array.
    void submit(byte[] pixels,int width,int height,boolean menu){
        submit(pixels,width,height,menu,System.nanoTime());
    }
    void submit(byte[] pixels,int width,int height,boolean menu,long captured){
        if(closed){recycle(pixels);return;}
        if(width<32 || width>HudFrame.MAX_WIDTH || height<32 || height>HudFrame.MAX_HEIGHT || pixels.length!=width*height*4)throw new IllegalArgumentException("Overlay dimensions");
        if(worker==null){worker=new Thread(this::run,"re4bridge-overlay-encoder");worker.setDaemon(true);worker.start();}
        var replaced=pending.getAndSet(new Job(pixels,width,height,menu,captured));
        if(replaced==null)ready.release();else recycle(replaced.pixels);
    }
    private void run(){
        byte[] converted=null;
        try{
            while(!closed){
                ready.acquire();if(closed)return;var job=pending.getAndSet(null);if(job==null)continue;
                try{
                    long started=System.nanoTime();
                    if(converted==null || converted.length!=job.pixels.length)converted=new byte[job.pixels.length];
                    var encoded=MinecraftHud.encodeOverlay(ByteBuffer.wrap(job.pixels),job.width,job.height,job.menu,converted);
                    lastEncodeNs=System.nanoTime()-started;this.encoded++;
                    if(!closed)latest=new Result(encoded,job.captured);
                }catch(MinecraftHud.OverlayTooLarge dropped){
                    this.dropped++;
                    // A detailed transient frame must never disarm movement/camera. Expiry still hides stale visuals.
                }finally{recycle(job.pixels);}
            }
        }catch(InterruptedException stop){Thread.currentThread().interrupt();}
        catch(RuntimeException error){failure="Codificacion de interfaz: "+error.getClass().getSimpleName();var job=pending.getAndSet(null);if(job!=null)recycle(job.pixels);}
    }
    @Override public void close(){closed=true;var job=pending.getAndSet(null);if(job!=null)recycle(job.pixels);pixelPool.close();latest=null;if(worker!=null)worker.interrupt();}
}
