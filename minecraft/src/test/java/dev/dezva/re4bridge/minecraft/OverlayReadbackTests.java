package dev.dezva.re4bridge.minecraft;
import java.util.*;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
class OverlayReadbackTests {
    private static class Gpu implements OverlayReadback.Gpu {
        int buffers,copies;long fences;Set<Long> ready=new HashSet<>(),liveFences=new HashSet<>();Set<Integer> liveBuffers=new HashSet<>();
        public int allocate(int bytes){liveBuffers.add(++buffers);return buffers;}
        public long capture(int b,int w,int h){liveFences.add(++fences);return fences;}
        public boolean ready(long f){return ready.contains(f);}
        public byte[] copy(int b,int bytes){copies++;byte[] pixels=new byte[bytes];pixels[0]=(byte)b;return pixels;}
        public void deleteFence(long f){liveFences.remove(f);}
        public void deleteBuffer(int b){liveBuffers.remove(b);}
    }
    @Test void stalledGpuNeverWaitsOrReusesPendingSlotsAndOnlyNewestCompletedImageIsCopied(){
        var gpu=new Gpu();var read=new OverlayReadback(gpu);
        read.capture(32,32,false,10);read.capture(32,32,false,20);read.capture(64,32,true,30);
        for(int i=0;i<100;i++){assertNull(read.poll());read.capture(32,32,false,40+i);}
        assertFalse(read.available());assertEquals(3,gpu.buffers);assertEquals(3,gpu.fences);assertEquals(0,gpu.copies);
        gpu.ready.addAll(Set.of(1L,2L,3L));var pixels=read.poll();assertEquals(30,pixels.captured());assertTrue(pixels.menu());assertEquals(64,pixels.width());assertEquals(3,pixels.bytes()[0]);assertEquals(1,gpu.copies);
        assertTrue(read.available());read.close();assertTrue(gpu.liveBuffers.isEmpty());assertTrue(gpu.liveFences.isEmpty());read.close();
    }
    @Test void outOfOrderGpuCompletionKeepsOtherTransfersLeasedUntilReady(){
        var gpu=new Gpu();var read=new OverlayReadback(gpu);
        read.capture(32,32,false,10);read.capture(64,32,true,20);gpu.ready.add(2L);
        assertEquals(20,read.poll().captured());assertEquals(Set.of(1L),gpu.liveFences);
        gpu.ready.add(1L);assertNull(read.poll());assertEquals(1,gpu.copies); // Late GPU completion cannot roll the hand backwards.
        read.capture(64,64,true,30);assertEquals(3,gpu.fences);assertEquals(Set.of(3L),gpu.liveFences);
        read.close();assertTrue(gpu.liveFences.isEmpty());assertTrue(gpu.liveBuffers.isEmpty());
    }
}
