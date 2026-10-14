package dev.dezva.re4bridge.minecraft;

import dev.dezva.re4bridge.ControllerMapping;
import dev.dezva.re4bridge.TerrainSnapshot;
import java.util.List;
import net.minecraft.util.math.Box;

/** Minecraft's 20 Hz jump/gravity for the local bridge view and editor. RE4 owns horizontal movement. */
final class BridgeVertical {
    private boolean initialized,grounded;private int generation,token,fallHarm;
    private double y,velocity,fallen,exhaustion;
    private ControllerMapping.Point point;
    void clear(){initialized=grounded=false;generation=token=fallHarm=0;y=velocity=fallen=exhaustion=0;point=null;}
    void hold(){velocity=0;}
    ControllerMapping.Point point(){return point;}
    int token(){return token;}boolean grounded(){return grounded;}float exhaustion(){return (float)exhaustion;}int fallHarm(){return fallHarm;}
    double lift(ControllerMapping.Point base,double scale){return Math.max(0,y-base.y())*scale;}
    ControllerMapping.Point step(ControllerMapping.Point base,int gen,boolean jump,boolean sprint,boolean paused,List<Box> boxes,TerrainSnapshot terrain,ControllerMapping map){
        if(gen!=generation){clear();generation=gen;}
        if(!initialized){y=base.y();velocity=fallen=0;grounded=true;initialized=true;}
        if(!paused){
            if(grounded && jump){velocity=.42;grounded=false;exhaustion+=sprint?.2:.05;}
            double next=y+velocity;boolean landed=false;
            if(velocity>0){
                for(var b:boxes)if(footprint(base,b) && y+1.8<=b.minY+.00001 && next+1.8>b.minY){next=Math.min(next,b.minY-1.8);velocity=0;}
                if(terrain!=null && map!=null){
                    for(double dx:new double[]{-.29,0,.29})for(double dz:new double[]{-.29,0,.29}){
                        var a=map.toNative(base.x()+dx,y+1.8,base.z()+dz);var b=map.toNative(base.x()+dx,next+1.8,base.z()+dz);
                        if(next>y){var hit=terrain.raycast(new TerrainSnapshot.Point(a.x(),a.y(),a.z()),new TerrainSnapshot.Point(b.x(),b.y(),b.z()));if(hit!=null){next=Math.min(next,TerrainSnapshot.grid(map,hit.point()).y()-1.8001);velocity=0;}}
                    }
                }
            }else{
                double floor=base.y();
                for(var b:boxes)if(footprint(base,b) && y>=b.maxY-.00001 && next<=b.maxY+.00001)floor=Math.max(floor,b.maxY);
                if(next<=floor+.00001){next=floor;landed=true;}
            }
            if(next<y)fallen+=y-next;
            if(landed){if(!grounded)fallHarm+=Math.max(0,(int)Math.ceil(fallen-3-1e-7));fallen=0;velocity=0;grounded=true;}
            else{grounded=false;velocity=(velocity-.08)*.98;}
            y=Math.max(base.y(),next);
        }
        if(!Double.isFinite(y) || y-base.y()>128)throw new IllegalArgumentException("Vertical bridge range");
        point=new ControllerMapping.Point(base.x(),y,base.z());token=token==Integer.MAX_VALUE?1:token+1;return point;
    }
    private static boolean footprint(ControllerMapping.Point p,Box b){return p.x()+.3>b.minX && p.x()-.3<b.maxX && p.z()+.3>b.minZ && p.z()-.3<b.maxZ;}
}
