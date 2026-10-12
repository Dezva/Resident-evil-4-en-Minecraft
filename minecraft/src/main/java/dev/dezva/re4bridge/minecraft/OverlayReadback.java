package dev.dezva.re4bridge.minecraft;

/** Three bounded GPU transfers. Never wait for the GPU and never overwrite a pending transfer. */
final class OverlayReadback implements AutoCloseable {
    interface Gpu {
        int allocate(int bytes);
        long capture(int buffer,int width,int height);
        boolean ready(long fence);
        byte[] copy(int buffer,int bytes);
        void deleteFence(long fence);
        void deleteBuffer(int buffer);
    }
    record Pixels(byte[] bytes,int width,int height,boolean menu,long captured){}
    private static final class Slot {int buffer,bytes,width,height;long fence,captured;boolean menu;}
    private final Slot[] slots={new Slot(),new Slot(),new Slot()};
    private final Gpu gpu;
    private long delivered;
    private boolean hasDelivered;
    OverlayReadback(Gpu gpu){this.gpu=gpu;}
    Pixels poll(){
        Slot newest=null;
        for(var slot:slots)if(slot.fence!=0 && gpu.ready(slot.fence)){
            gpu.deleteFence(slot.fence);slot.fence=0;
            if(newest==null || slot.captured-newest.captured>0)newest=slot;
        }
        if(newest==null || hasDelivered && newest.captured-delivered<=0)return null;
        delivered=newest.captured;hasDelivered=true;
        return new Pixels(gpu.copy(newest.buffer,newest.bytes),newest.width,newest.height,newest.menu,newest.captured);
    }
    boolean available(){for(var slot:slots)if(slot.fence==0)return true;return false;}
    void capture(int width,int height,boolean menu,long captured){
        for(var slot:slots)if(slot.fence==0){
            int bytes=Math.multiplyExact(Math.multiplyExact(width,height),4);
            if(slot.buffer==0 || slot.bytes!=bytes){if(slot.buffer!=0)gpu.deleteBuffer(slot.buffer);slot.buffer=gpu.allocate(bytes);slot.bytes=bytes;}
            slot.width=width;slot.height=height;slot.menu=menu;slot.captured=captured;
            slot.fence=gpu.capture(slot.buffer,width,height);
            if(slot.fence==0)throw new IllegalStateException("GPU fence unavailable");
            return;
        }
    }
    @Override public void close(){for(var slot:slots){if(slot.fence!=0)gpu.deleteFence(slot.fence);if(slot.buffer!=0)gpu.deleteBuffer(slot.buffer);slot.fence=0;slot.buffer=0;slot.bytes=0;}}
}
