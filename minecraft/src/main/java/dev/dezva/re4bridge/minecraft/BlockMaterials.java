package dev.dezva.re4bridge.minecraft;

import dev.dezva.re4bridge.BlockScene;
import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.*;
import javax.imageio.ImageIO;
import net.minecraft.block.BlockState;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.render.model.BakedQuad;
import net.minecraft.util.Identifier;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.Direction;
import net.minecraft.util.math.random.Random;

/** Reads active local PNG resources only; no texture images are shipped with the bridge. */
final class BlockMaterials {
    private static final Direction[] SIDES={Direction.WEST,Direction.EAST,Direction.DOWN,Direction.UP,Direction.NORTH,Direction.SOUTH};
    private final Map<List<Object>,Integer> cache=new HashMap<>();
    private final List<String> tiles=new ArrayList<>();
    private int missing;
    private Object resourceEpoch;
    void resources(MinecraftClient client){var epoch=client.getBlockRenderManager().getModels().getModel(net.minecraft.block.Blocks.STONE.getDefaultState());if(resourceEpoch!=epoch){clear();resourceEpoch=epoch;}}
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
    void clear(){cache.clear();tiles.clear();missing=0;var bytes=ByteBuffer.allocate(1024).order(ByteOrder.LITTLE_ENDIAN);for(int i=0;i<256;i++)bytes.putInt(((i/16+i%16)/4%2)==0?0xFFB000B0:0xFF303030);tiles.add(Base64.getEncoder().encodeToString(bytes.array()));}
    List<String> tiles(){return List.copyOf(tiles);}
    int missing(){return missing;}
    void begin(){missing=0;}
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
                if(tile==null){if(tiles.size()==BlockScene.MAX_TILES)throw new IOException("Tile limit");tile=tiles.size();String image=compose(client,layers,tint);tiles.add(image);cache.put(List.copyOf(key),tile);}
                result[i]=new Face(tile,affine(quads.getFirst(),i/2));
            } catch(IOException | IllegalArgumentException error){missing++;result[i]=new Face(0,new double[]{1,0,0,0,-1,1});}
        }
        return result;
    }
    private static String compose(MinecraftClient client,List<BakedQuad> quads,List<Integer> tints)throws IOException {
        if(quads.size()>16)throw new IOException("Complex face");int[] pixels=new int[256];
        for(int layer=0;layer<quads.size();layer++) {
            var contents=quads.get(layer).getSprite().getContents();var id=contents.getId();
            var resource=client.getResourceManager().getResource(Identifier.of(id.getNamespace(),"textures/"+id.getPath()+".png"));
            if(resource.isEmpty())throw new IOException("Missing PNG");byte[] bytes;
            try(var stream=resource.get().getInputStream()){bytes=stream.readNBytes(2*1024*1024+1);}if(bytes.length>2*1024*1024)throw new IOException("PNG limit");
            var image=readPng(bytes);int w=contents.getWidth(),h=contents.getHeight(),frame=contents.getDistinctFrameCount().findFirst().orElse(0);
            if(w<1 || h<1 || w>image.getWidth() || h>image.getHeight())throw new IOException("Invalid sprite frame");
            int columns=image.getWidth()/w,x0=(frame%columns)*w,y0=(frame/columns)*h;if(y0+h>image.getHeight())throw new IOException("Invalid animation frame");
            for(int y=0;y<16;y++)for(int x=0;x<16;x++) {
                int color=image.getRGB(x0+Math.min(w-1,(2*x+1)*w/32),y0+Math.min(h-1,(2*y+1)*h/32));
                pixels[y*16+x]=over(pixels[y*16+x],tint(color,tints.get(layer)));
            }
        }
        var buffer=ByteBuffer.allocate(1024).order(ByteOrder.LITTLE_ENDIAN);for(int pixel:pixels)buffer.putInt(pixel);
        return Base64.getEncoder().encodeToString(buffer.array());
    }
    static java.awt.image.BufferedImage readPng(byte[] data)throws IOException {
        if(data.length<33 || ByteBuffer.wrap(data).getLong()!=0x89504E470D0A1A0AL)throw new IOException("Expected PNG");
        var header=ByteBuffer.wrap(data);int w=header.getInt(16),h=header.getInt(20);
        if(w<1 || h<1 || w>4096 || h>4096 || (long)w*h>4_194_304)throw new IOException("Image dimensions");
        var image=ImageIO.read(new ByteArrayInputStream(data));if(image==null || image.getWidth()!=w || image.getHeight()!=h)throw new IOException("Invalid PNG");return image;
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
