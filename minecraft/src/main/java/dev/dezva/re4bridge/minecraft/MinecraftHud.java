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
 * No game images are bundled. Readback is bounded and render-thread only; encoding runs off-thread. */
final class MinecraftHud implements AutoCloseable {
    private final OverlayEncoder encoder=new OverlayEncoder();private boolean requested;private String failure;private long renderedAt;
    private SimpleFramebuffer target;private int requestedWidth,requestedHeight;private boolean wasMenu;
    static boolean capturing;static net.minecraft.client.gl.Framebuffer activeTarget;
    HudFrame snapshot(MinecraftClient client,int viewportWidth,int viewportHeight){
        if(client.options.hudHidden && !inventory(client)){requested=false;return null;}
        if(failure!=null || encoder.failure()!=null)throw new IllegalStateException(failure!=null?failure:encoder.failure());
        requestedWidth=viewportWidth>0?viewportWidth:client.getWindow().getFramebufferWidth();
        requestedHeight=viewportHeight>0?viewportHeight:client.getWindow().getFramebufferHeight();
        requested=true;var frame=encoder.latest();return frame!=null && frame.menu()==inventory(client)?frame:null;
    }
    static boolean inventory(MinecraftClient client){return client.currentScreen instanceof net.minecraft.client.gui.screen.ingame.InventoryScreen || client.currentScreen instanceof net.minecraft.client.gui.screen.ingame.CreativeInventoryScreen;}
    void render(MinecraftClient client,RenderTickCounter ticks){
        boolean wanted=requested;requested=false;
        if(!wanted || failure!=null || client.player==null || client.world==null)return;
        boolean menu=inventory(client);long now=Util.getMeasuringTimeMs();
        if(menu==wasMenu && renderedAt!=0 && now-renderedAt<50)return;
        double reduction=Math.min(1.,Math.min((double)HudFrame.MAX_WIDTH/requestedWidth,(double)HudFrame.MAX_HEIGHT/requestedHeight));
        int width=Math.max(32,(int)Math.round(requestedWidth*reduction)),height=Math.max(32,(int)Math.round(requestedHeight*reduction));
        try(GuiRenderState saved=new GuiRenderState()){
            try{
                if(target!=null && (target.textureWidth!=width || target.textureHeight!=height)){target.delete();target=null;}
                if(target==null){target=new SimpleFramebuffer(width,height,true,MinecraftClient.IS_SYSTEM_MAC);target.setClearColor(0,0,0,0);}
                activeTarget=target;capturing=true;
                RenderSystem.disableScissor();RenderSystem.colorMask(true,true,true,true);RenderSystem.depthMask(true);
                target.clear(MinecraftClient.IS_SYSTEM_MAC);target.beginWrite(true);
                RenderSystem.setShaderColor(1,1,1,1);RenderSystem.getModelViewStack().identity();RenderSystem.applyModelViewMatrix();
                // Native scene geometry remains in RE4. Only the actual Minecraft hand/item is rendered here.
                if(!menu){
                    RenderSystem.setProjectionMatrix(new Matrix4f().perspective((float)Math.toRadians(client.options.getFov().getValue()),(float)width/height,.05f,1000),VertexSorter.BY_DISTANCE);
                    RenderSystem.enableDepthTest();RenderSystem.depthFunc(GL11.GL_LEQUAL);DiffuseLighting.enableForLevel();
                    try(var buffers=new HudVertexBuffers()){
                        client.gameRenderer.firstPersonRenderer.renderItem(ticks.getTickDelta(false),new net.minecraft.client.util.math.MatrixStack(),buffers.consumers,client.player,15728880);
                        buffers.consumers.draw();
                    }
                }
                RenderSystem.clear(GL11.GL_DEPTH_BUFFER_BIT,MinecraftClient.IS_SYSTEM_MAC);
                int guiWidth=client.getWindow().getScaledWidth(),guiHeight=client.getWindow().getScaledHeight();
                RenderSystem.setProjectionMatrix(new Matrix4f().setOrtho(0,guiWidth,guiHeight,0,1000,21000),VertexSorter.BY_Z);
                RenderSystem.getModelViewStack().identity().translate(0,0,-11000);RenderSystem.applyModelViewMatrix();DiffuseLighting.enableGuiDepthLighting();
                try(var buffers=new HudVertexBuffers()){
                    var context=new DrawContext(client,buffers.consumers);
                    if(menu){
                        int x=(int)(client.mouse.getX()*guiWidth/client.getWindow().getWidth()),y=(int)(client.mouse.getY()*guiHeight/client.getWindow().getHeight());
                        client.currentScreen.renderWithTooltip(context,x,y,ticks.getTickDelta(false));
                        cursor(context,x,y);
                    }else{
                        client.inGameHud.renderHotbar(context,ticks);
                        context.drawGuiTexture(Identifier.ofVanilla("hud/crosshair"),guiWidth/2-7,guiHeight/2-7,15,15);
                    }
                    context.draw();
                }
                target.beginWrite(false);ByteBuffer rgba=MemoryUtil.memAlloc(width*height*4);
                try{
                    GL11.glReadPixels(0,0,width,height,GL11.GL_RGBA,GL11.GL_UNSIGNED_BYTE,rgba);
                    byte[] pixels=new byte[width*height*4];rgba.get(pixels);encoder.submit(pixels,width,height,menu);
                }finally{MemoryUtil.memFree(rgba);}
                wasMenu=menu;renderedAt=now;
            }catch(RuntimeException error){close();throw error;}finally{capturing=false;activeTarget=null;}
        }catch(RuntimeException error){failure="No se pudo capturar GUI/mano: "+error.getClass().getSimpleName();}
    }
    private static void cursor(DrawContext context,int x,int y){
        // Own small cursor glyph; Windows' RE4 window may hide the OS cursor.
        for(int row=0;row<9;row++)for(int col=0;col<=row/2;col++)context.fill(x+col,y+row,x+col+1,y+row+1,col==0 || col==row/2 || row==8?0xFF151515:0xFFFFFFFF);
    }
    static HudFrame encodeOverlay(ByteBuffer rgba,int width,int height,boolean menu){
        if(width<32 || width>HudFrame.MAX_WIDTH || height<32 || height>HudFrame.MAX_HEIGHT || rgba.remaining()!=width*height*4)throw new IllegalArgumentException("Overlay dimensions");
        byte[] raw=new byte[width*height*4];int start=rgba.position();
        for(int y=height-1;y>=0;y--)for(int x=0;x<width;x++){
            int i=start+(y*width+x)*4,a=Byte.toUnsignedInt(rgba.get(i+3));if(a==0)continue;
            int out=((height-1-y)*width+x)*4;
            raw[out]=(byte)straight(Byte.toUnsignedInt(rgba.get(i+2)),a);raw[out+1]=(byte)straight(Byte.toUnsignedInt(rgba.get(i+1)),a);raw[out+2]=(byte)straight(Byte.toUnsignedInt(rgba.get(i)),a);raw[out+3]=(byte)a;
        }
        var compressor=new java.util.zip.Deflater(1);var output=new java.io.ByteArrayOutputStream();byte[] chunk=new byte[8192];
        try{
            compressor.setInput(raw);compressor.finish();
            while(!compressor.finished()){
                int n=compressor.deflate(chunk);output.write(chunk,0,n);
                if(output.size()>HudFrame.MAX_COMPRESSED)throw new IllegalArgumentException("Overlay demasiado detallado para el limite de transporte");
            }
        }finally{compressor.end();}
        return HudFrame.overlay(Base64.getEncoder().encodeToString(output.toByteArray()),width,height,menu);
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
    @Override public void close(){if(target!=null){target.delete();target=null;}encoder.close();requested=false;}
}
