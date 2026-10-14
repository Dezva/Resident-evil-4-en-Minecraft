package dev.dezva.re4bridge.minecraft;

import dev.dezva.re4bridge.BlockScene;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.*;
import net.minecraft.block.BlockState;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.render.model.BakedQuad;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.Direction;
import net.minecraft.util.math.random.Random;

/** Samples the sprites Minecraft already decoded; never reopens PNGs or accesses the GPU. */
final class BlockMaterials {
    private static final Direction[] SIDES={Direction.WEST,Direction.EAST,Direction.DOWN,Direction.UP,Direction.NORTH,Direction.SOUTH};
    private final Map<List<Object>,Integer> cache=new HashMap<>();
    private final List<String> tiles=new ArrayList<>();
    private int missing;
    private Object resourceEpoch;
    private long revision;private final int[][] cracks=new int[10][];
    private String lastError="ninguno";
    private final Set<String> reported=new HashSet<>();
    long revision(){return revision;}
    String lastError(){return lastError;}
    void resources(MinecraftClient client){var epoch=client.getBlockRenderManager().getModels().getModel(net.minecraft.block.Blocks.STONE.getDefaultState());if(resourceEpoch!=epoch){clear();java.util.Arrays.fill(cracks,null);resourceEpoch=epoch;}}
    record Face(int tile,double[] map) {
        Face {map=map.clone();if(map.length!=6)throw new IllegalArgumentException("Invalid UV map");for(double v:map)if(!Double.isFinite(v) || Math.abs(v)>1024)throw new IllegalArgumentException("Invalid UV transform");}
        BlockScene.Face bounds(Box box,int axis) {
            int u=(axis+1)%3,v=(axis+2)%3;
            double[] low={box.minX,box.minY,box.minZ},high={box.maxX,box.maxY,box.maxZ};
            int[][] corners={{0,0},{1,0},{1,1},{0,1}};var uv=new ArrayList<Double>();
            for(var corner:corners) {
                double a=corner[0]==0?low[u]:high[u],b=corner[1]==0?low[v]:high[v];
                uv.add(clamp(map[0]*a+map[1]*b+map[2]));uv.add(clamp(map[3]*a+map[4]*b+map[5]));
            }
            return new BlockScene.Face(tile,uv);
        }
    }
    BlockMaterials(){clear();}
    void clear(){revision++;cache.clear();tiles.clear();missing=0;lastError="ninguno";reported.clear();var bytes=ByteBuffer.allocate(1024).order(ByteOrder.LITTLE_ENDIAN);for(int i=0;i<256;i++)bytes.putInt(((i/16+i%16)/4%2)==0?0xFFB000B0:0xFF303030);tiles.add(Base64.getEncoder().encodeToString(bytes.array()));}
    List<String> tiles(){return List.copyOf(tiles);}
    int missing(){return missing;}
    void begin(){clear();}
    static double clamp(double value){if(!Double.isFinite(value))throw new IllegalArgumentException("Invalid model UV");return Math.max(0,Math.min(1,value));}
    Face[] faces(MinecraftClient client,BlockPos pos,BlockState state) {
        Face[] result=new Face[6];var model=client.getBlockRenderManager().getModels().getModel(state);
        for(int i=0;i<6;i++) {
            var quads=new ArrayList<>(model.getQuads(state,SIDES[i],Random.create(state.getRenderingSeed(pos))));
            if(quads.isEmpty())for(var q:model.getQuads(state,null,Random.create(state.getRenderingSeed(pos))))if(q.getFace()==SIDES[i])quads.add(q);
            try {
                if(quads.isEmpty() || quads.size()>16)throw new IOException("Unsupported face model");
                var key=new ArrayList<Object>();var tint=new ArrayList<Integer>();var layers=new ArrayList<BakedQuad>();
                for(var q:quads){int color=q.hasColor()?client.getBlockColors().getColor(state,client.world,pos,q.getColorIndex()):0xFFFFFF;boolean duplicate=false;for(int l=0;l<layers.size();l++)if(layers.get(l).getSprite()==q.getSprite() && tint.get(l)==color)duplicate=true;if(duplicate)continue;layers.add(q);tint.add(color);key.add(q.getSprite());key.add(color);}
                Integer tile=cache.get(key);
                if(tile==null){if(tiles.size()==BlockScene.MAX_TILES)throw new IOException("Tile limit");tile=tiles.size();String image=compose(layers,tint);tiles.add(image);cache.put(List.copyOf(key),tile);}
                result[i]=new Face(tile,affine(quads.getFirst(),i/2));
            } catch(IOException | IllegalArgumentException | IllegalStateException error){
                missing++;lastError=state.getBlock().getTranslationKey()+" / "+SIDES[i]+": "+error.getMessage();
                if(reported.size()<16 && reported.add(lastError))org.slf4j.LoggerFactory.getLogger("re4bridge.materials").warn("RE4 material: {}",lastError);
                result[i]=new Face(0,new double[]{1,0,0,0,-1,1});
            }
        }
        return result;
    }
    Face[] mining(MinecraftClient client,Face[] faces,int stage){
        if(stage<0 || stage>9)return faces;
        if(cracks[stage]==null){
            try(var stream=client.getResourceManager().getResourceOrThrow(net.minecraft.util.Identifier.ofVanilla("textures/block/destroy_stage_"+stage+".png")).getInputStream();var image=net.minecraft.client.texture.NativeImage.read(stream)){
                var pixels=new int[256];for(int y=0;y<16;y++)for(int x=0;x<16;x++){int c=image.getColor(Math.min(image.getWidth()-1,x*image.getWidth()/16),Math.min(image.getHeight()-1,y*image.getHeight()/16));pixels[y*16+x]=(c&0xFF00FF00)|((c&255)<<16)|((c>>>16)&255);}cracks[stage]=pixels;
            }catch(IOException error){return faces;}
        }
        Face[] result=new Face[6];for(int i=0;i<6;i++){var original=faces[i];var key=List.<Object>of("crack",tiles.get(original.tile),stage);Integer id=cache.get(key);if(id==null && tiles.size()<BlockScene.MAX_TILES){id=tiles.size();tiles.add(crackTile(tiles.get(original.tile),cracks[stage]));cache.put(key,id);}result[i]=id==null?original:new Face(id,original.map);}
        return result;
    }
    static String crackTile(String base,int[] crack){
        var source=ByteBuffer.wrap(Base64.getDecoder().decode(base)).order(ByteOrder.LITTLE_ENDIAN);var out=ByteBuffer.allocate(1024).order(ByteOrder.LITTLE_ENDIAN);
        for(int i=0;i<256;i++){int color=source.getInt(),c=crack[i],alpha=c>>>24;int result=color&0xFF000000;for(int shift:new int[]{0,8,16}){int factor=((c>>>shift)&255);int channel=(color>>>shift)&255;result|=(channel*(255-alpha)+channel*factor*alpha/255)/255<<shift;}out.putInt(result);}return Base64.getEncoder().encodeToString(out.array());
    }
    static String compose(List<BakedQuad> quads,List<Integer> tints)throws IOException {
        if(quads.size()>16 || quads.size()!=tints.size())throw new IOException("Complex face");
        int[] pixels=new int[256];
        for(int layer=0;layer<quads.size();layer++) {
            var contents=quads.get(layer).getSprite().getContents();
            // Borrow the actual decoded sprite, including atlas-generated/resource-pack sprites.
            // Minecraft owns this image: never close, upload, mutate or retain it past a reload.
            var image=contents.image;
            // In 1.21.1 the static sprite stream contains 1, not a zero-based frame index.
            int w=contents.getWidth(),h=contents.getHeight();
            int frame=contents.animation==null?0:contents.getDistinctFrameCount().findFirst().orElse(0);
            if(w<1 || h<1 || w>image.getWidth() || h>image.getHeight())throw new IOException("Invalid sprite frame: "+contents.getId());
            int columns=image.getWidth()/w,x0=(frame%columns)*w,y0=(frame/columns)*h;
            if(frame<0 || y0+h>image.getHeight())throw new IOException("Invalid animation frame: "+contents.getId());
            for(int y=0;y<16;y++)for(int x=0;x<16;x++) {
                int abgr=image.getColor(x0+Math.min(w-1,(2*x+1)*w/32),y0+Math.min(h-1,(2*y+1)*h/32));
                int argb=(abgr&0xFF00FF00)|((abgr&255)<<16)|((abgr>>>16)&255);
                pixels[y*16+x]=over(pixels[y*16+x],tint(argb,tints.get(layer)));
            }
        }
        var buffer=ByteBuffer.allocate(1024).order(ByteOrder.LITTLE_ENDIAN);for(int pixel:pixels)buffer.putInt(pixel);
        return Base64.getEncoder().encodeToString(buffer.array());
    }
    static int tint(int color,int tint){int r=((color>>16)&255)*((tint>>16)&255)/255,g=((color>>8)&255)*((tint>>8)&255)/255,b=(color&255)*(tint&255)/255;return(color&0xFF000000)|(r<<16)|(g<<8)|b;}
    static int over(int base,int top){int a=top>>>24,b=base>>>24,out=a+b*(255-a)/255;if(out==0)return 0;int result=out<<24;for(int shift:new int[]{0,8,16}){int c=(((top>>>shift)&255)*a+((base>>>shift)&255)*b*(255-a)/255)/out;result|=c<<shift;}return result;}
    static double[] affine(BakedQuad quad,int axis) {
        int[] data=quad.getVertexData();int stride=data.length/4;if(stride<6)throw new IllegalArgumentException("Invalid quad");
        double[][] points=new double[4][4];var sprite=quad.getSprite();
        for(int i=0;i<4;i++){points[i][0]=Float.intBitsToFloat(data[i*stride+(axis+1)%3]);points[i][1]=Float.intBitsToFloat(data[i*stride+(axis+2)%3]);points[i][2]=(Float.intBitsToFloat(data[i*stride+4])-sprite.getMinU())/(sprite.getMaxU()-sprite.getMinU());points[i][3]=(Float.intBitsToFloat(data[i*stride+5])-sprite.getMinV())/(sprite.getMaxV()-sprite.getMinV());}
        return affine(points);
    }
    static double[] affine(double[][] p) {
        for(int j=1;j<3;j++)for(int k=j+1;k<4;k++) {
            double a=p[j][0]-p[0][0],b=p[j][1]-p[0][1],c=p[k][0]-p[0][0],d=p[k][1]-p[0][1],det=a*d-b*c;
            if(Math.abs(det)<1e-8)continue;double[] result=new double[6];
            for(int uv=0;uv<2;uv++){double e=p[j][uv+2]-p[0][uv+2],f=p[k][uv+2]-p[0][uv+2];result[uv*3]=(e*d-b*f)/det;result[uv*3+1]=(a*f-e*c)/det;result[uv*3+2]=p[0][uv+2]-result[uv*3]*p[0][0]-result[uv*3+1]*p[0][1];}
            return result;
        }
        throw new IllegalArgumentException("Degenerate face quad");
    }
}
