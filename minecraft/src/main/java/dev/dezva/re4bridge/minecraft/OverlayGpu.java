package dev.dezva.re4bridge.minecraft;

import org.lwjgl.opengl.GL11;
import org.lwjgl.opengl.GL15;
import org.lwjgl.opengl.GL21;
import org.lwjgl.opengl.GL30;
import org.lwjgl.opengl.GL32;

/** Used only inside GuiRenderState's pack-buffer lease, on the render thread. */
final class OverlayGpu implements OverlayReadback.Gpu {
    private final java.util.function.IntFunction<byte[]> allocatePixels;
    OverlayGpu(java.util.function.IntFunction<byte[]> allocatePixels){this.allocatePixels=allocatePixels;}
    public int allocate(int bytes){int buffer=GL15.glGenBuffers();if(buffer==0)throw new IllegalStateException("GPU buffer unavailable");GL15.glBindBuffer(GL21.GL_PIXEL_PACK_BUFFER,buffer);GL15.glBufferData(GL21.GL_PIXEL_PACK_BUFFER,bytes,GL15.GL_STREAM_READ);return buffer;}
    public long capture(int buffer,int width,int height){
        GL15.glBindBuffer(GL21.GL_PIXEL_PACK_BUFFER,buffer);
        GL11.glReadPixels(0,0,width,height,GL11.GL_RGBA,GL11.GL_UNSIGNED_BYTE,0L);
        long fence=GL32.glFenceSync(GL32.GL_SYNC_GPU_COMMANDS_COMPLETE,0);GL11.glFlush();return fence;
    }
    public boolean ready(long fence){
        int result=GL32.glClientWaitSync(fence,0,0L);
        if(result==GL32.GL_WAIT_FAILED)throw new IllegalStateException("GPU fence failed");
        return result==GL32.GL_ALREADY_SIGNALED || result==GL32.GL_CONDITION_SATISFIED;
    }
    public byte[] copy(int buffer,int bytes){
        GL15.glBindBuffer(GL21.GL_PIXEL_PACK_BUFFER,buffer);
        var mapped=GL30.glMapBufferRange(GL21.GL_PIXEL_PACK_BUFFER,0,bytes,GL30.GL_MAP_READ_BIT);
        if(mapped==null)throw new IllegalStateException("GPU readback unavailable");
        byte[] result=allocatePixels.apply(bytes);
        try{mapped.get(result);}finally{if(!GL15.glUnmapBuffer(GL21.GL_PIXEL_PACK_BUFFER))throw new IllegalStateException("GPU readback invalidated");}
        return result;
    }
    public void deleteFence(long fence){GL32.glDeleteSync(fence);}
    public void deleteBuffer(int buffer){GL15.glDeleteBuffers(buffer);}
}
