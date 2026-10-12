package dev.dezva.re4bridge.minecraft;

import com.mojang.blaze3d.systems.RenderSystem;
import com.mojang.blaze3d.systems.VertexSorter;
import dev.dezva.re4bridge.HudFrame;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Base64;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.gl.SimpleFramebuffer;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.client.render.DiffuseLighting;
import net.minecraft.client.render.RenderTickCounter;
import net.minecraft.client.texture.Sprite;
import net.minecraft.item.BlockItem;
import net.minecraft.item.ItemStack;
import net.minecraft.util.Identifier;
import net.minecraft.util.Util;
import org.joml.Matrix4f;
import org.lwjgl.opengl.GL11;
import org.lwjgl.system.MemoryUtil;

/** Render the installed game's own hotbar, including GUI item models/overlays.
 * No game images are bundled. Readback is small, cached, and render-thread only. */
final class MinecraftHud implements AutoCloseable {
    private final ItemStack[] previous=new ItemStack[10];
    private Sprite epoch;
    private int selected=-1;
    private Object arm;
    private HudFrame cached;
    private boolean requested;
    private String failure;
    private long renderedAt;
    private SimpleFramebuffer target;

    // Called by the tick publisher: never issue GL commands from here.
    HudFrame snapshot(MinecraftClient client){
        if(client.options.hudHidden){requested=false;return null;}
        if(failure!=null)throw new IllegalStateException(failure);
        requested=true;
        return cached;
    }
    void render(MinecraftClient client,RenderTickCounter ticks){
        boolean wanted=requested;requested=false;
        if(!wanted || failure!=null || client.player==null || client.world==null)return;
        var background=client.getGuiAtlasManager().getSprite(Identifier.ofVanilla("hud/hotbar"));
        var inventory=client.player.getInventory();int slot=inventory.selectedSlot;
        var currentArm=client.player.getMainArm();
        boolean dirty=cached==null || epoch!=background || slot!=selected || arm!=currentArm;
        boolean animated=false;
        ItemStack[] stacks=new ItemStack[10];
        for(int i=0;i<10;i++){
            var stack=i==9?client.player.getOffHandStack():inventory.getStack(i);stacks[i]=stack;
            if(previous[i]==null || !ItemStack.areEqual(previous[i],stack))dirty=true;
            if(!stack.isEmpty() && (!(stack.getItem() instanceof BlockItem) || stack.hasGlint()
                    || stack.getBobbingAnimationTime()>0
                    || client.getItemRenderer().getModel(stack,client.world,client.player,i).getParticleSprite().getContents().animation!=null))animated=true;
        }
        long now=Util.getMeasuringTimeMs();
        // Static building inventories do not stall the GPU on every frame.
        if(!dirty && (!animated || now-renderedAt<100))return;
        try(GuiRenderState saved=new GuiRenderState()){
            RenderSystem.assertOnRenderThread();
            try{
                if(target==null){target=new SimpleFramebuffer(HudFrame.WIDTH,HudFrame.HEIGHT,true,MinecraftClient.IS_SYSTEM_MAC);target.setClearColor(0,0,0,0);}
                RenderSystem.disableScissor();RenderSystem.colorMask(true,true,true,true);RenderSystem.depthMask(true);
                target.clear(MinecraftClient.IS_SYSTEM_MAC);target.beginWrite(true);
                RenderSystem.setProjectionMatrix(new Matrix4f().setOrtho(0,HudFrame.WIDTH,HudFrame.HEIGHT,0,1000,21000),VertexSorter.BY_Z);
                RenderSystem.getModelViewStack().identity().translate(0,0,-11000);RenderSystem.applyModelViewMatrix();
                RenderSystem.setShaderColor(1,1,1,1);DiffuseLighting.enableGuiDepthLighting();
                try(var buffers=new HudVertexBuffers()){
                    var consumers=buffers.consumers;
                    var context=new DrawContext(client,consumers){
                        @Override public int getScaledWindowWidth(){return HudFrame.WIDTH;}
                        @Override public int getScaledWindowHeight(){return HudFrame.HEIGHT;}
                    };
                    client.inGameHud.renderHotbar(context,ticks);context.draw();
                }
                target.beginWrite(false); // read our target, never Minecraft's world framebuffer
                ByteBuffer bytes=MemoryUtil.memAlloc(HudFrame.BYTES);
                try{
                    GL11.glReadPixels(0,0,HudFrame.WIDTH,HudFrame.HEIGHT,GL11.GL_RGBA,GL11.GL_UNSIGNED_BYTE,bytes);
                    cached=encode(bytes,HudFrame.WIDTH,HudFrame.HEIGHT);
                }finally{MemoryUtil.memFree(bytes);}
                for(int i=0;i<10;i++)previous[i]=stacks[i].copy();
                epoch=background;selected=slot;arm=currentArm;renderedAt=now;
            }catch(RuntimeException error){close();throw error;} // dispose before restoring GL bindings
        }catch(RuntimeException error){
            cached=null;failure="No se pudo renderizar la barra vanilla: "+error.getClass().getSimpleName();
        }
    }
    // FBO RGBA rows are bottom-up and blended over transparent black.
    static HudFrame encode(ByteBuffer rgba,int width,int height){
        if(width!=HudFrame.WIDTH || height!=HudFrame.HEIGHT || rgba.remaining()!=HudFrame.BYTES)throw new IllegalArgumentException("HUD dimensions");
        var bytes=ByteBuffer.allocate(HudFrame.BYTES).order(ByteOrder.LITTLE_ENDIAN);int start=rgba.position();
        for(int y=height-1;y>=0;y--)for(int x=0;x<width;x++){
            int i=start+(y*width+x)*4,a=Byte.toUnsignedInt(rgba.get(i+3));
            int r=straight(Byte.toUnsignedInt(rgba.get(i)),a),g=straight(Byte.toUnsignedInt(rgba.get(i+1)),a),b=straight(Byte.toUnsignedInt(rgba.get(i+2)),a);
            bytes.putInt((a<<24)|(r<<16)|(g<<8)|b);
        }
        return new HudFrame(Base64.getEncoder().encodeToString(bytes.array()));
    }
    private static int straight(int color,int alpha){return alpha==0?0:Math.min(255,(color*255+alpha/2)/alpha);}
    @Override public void close(){if(target!=null){target.delete();target=null;}cached=null;requested=false;}
}
