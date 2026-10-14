package dev.dezva.re4bridge.minecraft;

import dev.dezva.re4bridge.BlockPreview;
import dev.dezva.re4bridge.BlockScene;
import java.util.*;
import java.nio.*;
import net.minecraft.util.math.BlockPos;

/** Bounded textured debris drawn by RE4's world/depth renderer, excluded from physics. */
final class BlockParticles {
    private record Fragment(double x,double y,double z,double vx,double vy,double vz,double born,String tile){}
    private final ArrayDeque<Fragment> fragments=new ArrayDeque<>();private double time;
    void clear(){fragments.clear();time=0;}
    void tick(boolean running){if(running)time+=.05;fragments.removeIf(f->time-f.born>.8);}
    void burst(BlockPos p,String texture){
        var random=new Random(p.asLong()^Double.doubleToLongBits(time));
        for(int i=0;i<12;i++){
            if(fragments.size()==24)fragments.removeFirst();
            fragments.addLast(new Fragment(p.getX()+.15+random.nextDouble()*.7,p.getY()+.15+random.nextDouble()*.7,p.getZ()+.15+random.nextDouble()*.7,(random.nextDouble()-.5)*2.4,1+random.nextDouble()*1.3,(random.nextDouble()-.5)*2.4,time,crop(texture,random.nextInt(13),random.nextInt(13))));
        }
    }
    static String crop(String texture,int x0,int y0){
        var data=ByteBuffer.wrap(Base64.getDecoder().decode(texture)).order(ByteOrder.LITTLE_ENDIAN);var output=ByteBuffer.allocate(1024).order(ByteOrder.LITTLE_ENDIAN);
        for(int y=0;y<16;y++)for(int x=0;x<16;x++)output.putInt(data.getInt(4*((y0+y/4)*16+x0+x/4)));
        return Base64.getEncoder().encodeToString(output.array());
    }
    BlockScene decorate(BlockScene base,BlockPos anchor,BlockPreview origin,double yaw,double height){
        if(fragments.isEmpty())return base;
        var boxes=new ArrayList<BlockScene.Bounds>(base==null?List.of():base.boxes());var faces=new ArrayList<List<BlockScene.Face>>(base==null?List.of():base.faces());var tiles=new ArrayList<String>(base==null?List.of():base.tiles());
        int colliders=boxes.size();double scale=origin.size();
        for(var f:fragments){
            if(boxes.size()==BlockScene.MAX_BOXES)break;
            int tile=tiles.indexOf(f.tile);if(tile<0){if(tiles.size()==BlockScene.MAX_TILES)continue;tile=tiles.size();tiles.add(f.tile);}
            double t=time-f.born,x=(f.x+f.vx*t-anchor.getX()-.5)*scale,z=(f.z+f.vz*t-anchor.getZ()-.5)*scale;
            double y=(f.y+f.vy*t-3*t*t-anchor.getY())*scale,r=.045*scale;
            boxes.add(new BlockScene.Bounds(x-r,y-r,z-r,x+r,y+r,z+r));
            var face=new BlockScene.Face(tile,List.of(0.,0.,1.,0.,1.,1.,0.,1.));faces.add(Collections.nCopies(6,face));
        }
        if(boxes.size()==colliders)return base;
        return new BlockScene(origin.x(),origin.y()+height,origin.z(),yaw,boxes,3,base!=null && base.collisions(),tiles,faces,colliders);
    }
}
