package dev.dezva.re4bridge.minecraft;

import dev.dezva.re4bridge.HudFrame;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Base64;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.texture.Sprite;
import net.minecraft.item.ItemStack;
import net.minecraft.util.Identifier;

/** Initial HUD: real GUI hotbar/selection sprites, flat material icons and actual counts. */
final class MinecraftHud {
    private final ItemStack[] previous=new ItemStack[9];
    private Sprite epoch;
    private int selected=-1;
    private HudFrame cached;
    HudFrame snapshot(MinecraftClient client){
        var background=client.getGuiAtlasManager().getSprite(Identifier.ofVanilla("hud/hotbar"));
        var inventory=client.player.getInventory();int slot=inventory.selectedSlot;
        boolean dirty=cached==null || epoch!=background || slot!=selected;
        for(int i=0;i<9;i++)if(previous[i]==null || !ItemStack.areEqual(previous[i],inventory.getStack(i)))dirty=true;
        if(!dirty)return cached;
        int[] pixels=new int[HudFrame.WIDTH*HudFrame.HEIGHT];
        sprite(pixels,background,37,9,182,22);
        sprite(pixels,client.getGuiAtlasManager().getSprite(Identifier.ofVanilla("hud/hotbar_selection")),36+slot*20,8,24,24);
        for(int i=0;i<9;i++){
            var stack=inventory.getStack(i);previous[i]=stack.copy();
            if(stack.isEmpty())continue;
            sprite(pixels,client.getItemRenderer().getModel(stack,client.world,client.player,0).getParticleSprite(),40+i*20,12,16,16);
            if(stack.getCount()>1)number(pixels,Math.min(999,stack.getCount()),55+i*20,25);
        }
        var bytes=ByteBuffer.allocate(HudFrame.BYTES).order(ByteOrder.LITTLE_ENDIAN);for(int pixel:pixels)bytes.putInt(pixel);
        cached=new HudFrame(Base64.getEncoder().encodeToString(bytes.array()));epoch=background;selected=slot;return cached;
    }
    static void sprite(int[] pixels,Sprite sprite,int x,int y,int width,int height){
        var contents=sprite.getContents();var image=contents.image;int w=contents.getWidth(),h=contents.getHeight();
        int frame=contents.animation==null?0:contents.getDistinctFrameCount().findFirst().orElse(0),columns=image.getWidth()/w;
        int x0=(frame%columns)*w,y0=(frame/columns)*h;
        for(int v=0;v<height;v++)for(int u=0;u<width;u++){
            int px=x+u,py=y+v;if(px<0 || py<0 || px>=HudFrame.WIDTH || py>=HudFrame.HEIGHT)continue;
            int abgr=image.getColor(x0+u*w/width,y0+v*h/height),argb=(abgr&0xFF00FF00)|((abgr&255)<<16)|((abgr>>>16)&255);
            int i=py*HudFrame.WIDTH+px;pixels[i]=BlockMaterials.over(pixels[i],argb);
        }
    }
    // Original small digit glyphs: no font/game image is distributed.
    private static final String[] DIGITS={"111101101101111","010110010010111","111001111100111","111001111001111","101101111001001","111100111001111","111100111101111","111001001001001","111101111101111","111101111001111"};
    static void number(int[] pixels,int count,int right,int top){
        String text=Integer.toString(count);int left=right-text.length()*4+1;
        for(int n=0;n<text.length();n++)for(int y=0;y<5;y++)for(int x=0;x<3;x++)if(DIGITS[text.charAt(n)-'0'].charAt(y*3+x)=='1'){
            put(pixels,left+n*4+x+1,top+y+1,0xFF202020);put(pixels,left+n*4+x,top+y,0xFFFFFFFF);
        }
    }
    private static void put(int[] pixels,int x,int y,int color){if(x>=0 && y>=0 && x<HudFrame.WIDTH && y<HudFrame.HEIGHT)pixels[y*HudFrame.WIDTH+x]=color;}
}
