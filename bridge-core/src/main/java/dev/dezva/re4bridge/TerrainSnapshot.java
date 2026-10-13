package dev.dezva.re4bridge;

import com.google.gson.*;
import java.io.IOException;
import java.util.*;

/** Bounded native collision surface snapshot. Immutable; unknown space is never considered free. */
public final class TerrainSnapshot {
    public record Point(double x,double y,double z){
        public double component(int i){return i==0?x:i==1?y:z;}
        public Point subtract(Point p){return new Point(x-p.x,y-p.y,z-p.z);}
        public Point add(Point p){return new Point(x+p.x,y+p.y,z+p.z);}
        public Point times(double n){return new Point(x*n,y*n,z*n);}
        public double dot(Point p){return x*p.x+y*p.y+z*p.z;}
        public Point cross(Point p){return new Point(y*p.z-z*p.y,z*p.x-x*p.z,x*p.y-y*p.x);}
    }
    public record Hit(double fraction,Point point,Point normal){}
    private record Triangle(Point a,Point b,Point c,double[] bounds,boolean support){}
    private final int revision,generation;private final long received;private final boolean complete;
    private final Point center;private final double radius;private final List<Triangle> triangles;
    private TerrainSnapshot(int revision,int generation,long received,boolean complete,Point center,double radius,List<Triangle> triangles){this.revision=revision;this.generation=generation;this.received=received;this.complete=complete;this.center=center;this.radius=radius;this.triangles=List.copyOf(triangles);}
    public int revision(){return revision;}public int generation(){return generation;}public int count(){return triangles.size();}
    public boolean fresh(long now){return complete && now-received>=0 && now-received<1_000_000_000L;}
    public static TerrainSnapshot parse(JsonObject json,long now) throws IOException {
        try{
            int revision=integer(json,"revision",1,2147483647),generation=integer(json,"generation",1,2147483647),age=integer(json,"age_ms",0,1000);
            if(!json.getAsJsonPrimitive("complete").isBoolean())throw new IllegalArgumentException();
            boolean complete=json.get("complete").getAsBoolean();var pos=json.getAsJsonArray("position");if(pos.size()!=3)throw new IllegalArgumentException();
            Point center=new Point(number(pos.get(0)),number(pos.get(1)),number(pos.get(2)));double radius=number(json.get("radius"));if(radius<1000 || radius>60000)throw new IllegalArgumentException();
            var rows=json.getAsJsonArray("triangles");if(rows.size()>1024 || !complete && !rows.isEmpty())throw new IllegalArgumentException();
            var kinds=json.has("kinds")?json.getAsJsonArray("kinds"):null;
            if(kinds!=null && kinds.size()!=rows.size())throw new IllegalArgumentException();
            var triangles=new ArrayList<Triangle>();
            for(var entry:rows){var row=entry.getAsJsonArray();if(row.size()!=9)throw new IllegalArgumentException();double[] p=new double[9];for(int i=0;i<9;i++)p[i]=number(row.get(i));
                var a=new Point(p[0],p[1],p[2]);var b=new Point(p[3],p[4],p[5]);var c=new Point(p[6],p[7],p[8]);var cross=b.subtract(a).cross(c.subtract(a));if(cross.dot(cross)<1e-8)throw new IllegalArgumentException();
                String kind=kinds==null?"other":kinds.get(triangles.size()).getAsString();
                if(!Set.of("floor","slope","wall","other").contains(kind))throw new IllegalArgumentException();
                boolean support=(kind.equals("floor") || kind.equals("slope")) && cross.y*cross.y>=cross.dot(cross)*.25;
                double[] bounds=new double[6];for(int i=0;i<3;i++){bounds[i]=Math.min(a.component(i),Math.min(b.component(i),c.component(i)));bounds[i+3]=Math.max(a.component(i),Math.max(b.component(i),c.component(i)));}triangles.add(new Triangle(a,b,c,bounds,support));}
            return new TerrainSnapshot(revision,generation,now-age*1_000_000L,complete,center,radius,triangles);
        }catch(RuntimeException error){throw new IOException("Invalid native terrain snapshot",error);}
    }
    private static double number(JsonElement e){if(!e.isJsonPrimitive() || !e.getAsJsonPrimitive().isNumber())throw new IllegalArgumentException();double v=e.getAsDouble();if(!Double.isFinite(v) || Math.abs(v)>20_000_000)throw new IllegalArgumentException();return v;}
    private static int integer(JsonObject j,String name,int min,int max){var e=j.getAsJsonPrimitive(name);int v=e.getAsInt();if(!e.isNumber() || !e.getAsString().equals(Integer.toString(v)) || v<min || v>max)throw new IllegalArgumentException();return v;}
    public boolean covered(Point point){return complete && Double.isFinite(point.x+point.y+point.z) && Math.abs(point.x-center.x)<=radius && Math.abs(point.y-center.y)<=radius && Math.abs(point.z-center.z)<=radius;}
    public Hit raycast(Point start,Point end){
        return raycast(start,end,false);
    }
    private Hit raycast(Point start,Point end,boolean supportOnly){
        if(!covered(start) || !covered(end))throw new IllegalArgumentException("Unknown terrain coverage");
        Point direction=end.subtract(start);if(direction.dot(direction)<1e-20)return null;Hit best=null;
        for(var t:triangles){
            if(supportOnly && !t.support)continue;
            boolean outside=false;for(int i=0;i<3;i++)if(Math.max(start.component(i),end.component(i))<t.bounds[i] || Math.min(start.component(i),end.component(i))>t.bounds[i+3]){outside=true;break;}if(outside)continue;
            var ab=t.b.subtract(t.a);var ac=t.c.subtract(t.a);var p=direction.cross(ac);double det=ab.dot(p);if(Math.abs(det)<1e-12*Math.sqrt(ab.dot(ab)*ac.dot(ac)*direction.dot(direction)))continue;
            var offset=start.subtract(t.a);double u=offset.dot(p)/det;if(u<-1e-9 || u>1+1e-9)continue;var q=offset.cross(ab);double v=direction.dot(q)/det;if(v<-1e-9 || u+v>1+1e-9)continue;
            double distance=ac.dot(q)/det;if(distance<0 || distance>1 || best!=null && distance>=best.fraction)continue;
            var normal=ab.cross(ac);normal=normal.times(1/Math.sqrt(normal.dot(normal)));if(normal.dot(direction)>0)normal=normal.times(-1);
            best=new Hit(distance,start.add(direction.times(distance)),normal);
        }return best;
    }
    /** Test in native space an oriented Minecraft AABB by transforming triangles into its grid. */
    public boolean intersects(ControllerMapping mapping,double[] bounds,double inset){
        return intersects(mapping,bounds,inset,false);
    }
    /** Floor/slope may enter a voxel's base; walls and unknown kinds remain solid.
     * Never raise a rejected proposal to a different cell. Native depth hides buried base. */
    public boolean placementIntersects(ControllerMapping mapping,double[] bounds,double inset){
        return intersects(mapping,bounds,inset,true);
    }
    private boolean intersects(ControllerMapping mapping,double[] bounds,double inset,boolean supportContact){
        if(bounds.length!=6 || inset<0 || !Double.isFinite(inset))throw new IllegalArgumentException();
        for(int i=0;i<3;i++)if(!Double.isFinite(bounds[i]+bounds[i+3]) || bounds[i+3]-bounds[i]<=2*inset)throw new IllegalArgumentException();
        double[] nativeBounds={Double.POSITIVE_INFINITY,Double.POSITIVE_INFINITY,Double.POSITIVE_INFINITY,Double.NEGATIVE_INFINITY,Double.NEGATIVE_INFINITY,Double.NEGATIVE_INFINITY};
        for(int mask=0;mask<8;mask++){var p=mapping.toNative(bounds[(mask&1)==0?0:3],bounds[(mask&2)==0?1:4],bounds[(mask&4)==0?2:5]);var point=new Point(p.x(),p.y(),p.z());if(!covered(point))throw new IllegalArgumentException("Unknown terrain coverage");for(int i=0;i<3;i++){nativeBounds[i]=Math.min(nativeBounds[i],point.component(i));nativeBounds[i+3]=Math.max(nativeBounds[i+3],point.component(i));}}
        Point center=new Point((bounds[0]+bounds[3])/2,(bounds[1]+bounds[4])/2,(bounds[2]+bounds[5])/2),half=new Point((bounds[3]-bounds[0])/2-inset,(bounds[4]-bounds[1])/2-inset,(bounds[5]-bounds[2])/2-inset);
        var basis=new Point[]{new Point(1,0,0),new Point(0,1,0),new Point(0,0,1)};
        for(var triangle:triangles){
            boolean outside=false;for(int i=0;i<3;i++)if(nativeBounds[i]>triangle.bounds[i+3] || nativeBounds[i+3]<triangle.bounds[i]){outside=true;break;}if(outside)continue;
            Point[] p={grid(mapping,triangle.a).subtract(center),grid(mapping,triangle.b).subtract(center),grid(mapping,triangle.c).subtract(center)};
            boolean separated=false;for(int i=0;i<3;i++)if(Math.min(p[0].component(i),Math.min(p[1].component(i),p[2].component(i)))>half.component(i) || Math.max(p[0].component(i),Math.max(p[1].component(i),p[2].component(i)))<-half.component(i)){separated=true;break;}if(separated)continue;
            Point[] edges={p[1].subtract(p[0]),p[2].subtract(p[1]),p[0].subtract(p[2])};var axes=new ArrayList<Point>();axes.add(edges[0].cross(edges[1]));for(var e:edges)for(var b:basis)axes.add(e.cross(b));
            for(var axis:axes){if(axis.dot(axis)<1e-20)continue;double r=Math.abs(axis.x)*half.x+Math.abs(axis.y)*half.y+Math.abs(axis.z)*half.z;
                double min=Math.min(p[0].dot(axis),Math.min(p[1].dot(axis),p[2].dot(axis))),max=Math.max(p[0].dot(axis),Math.max(p[1].dot(axis),p[2].dot(axis)));if(min>r+1e-9 || max<-r-1e-9){separated=true;break;}}
            if(!separated && !(supportContact && triangle.support && supportBelowTop(mapping,triangle,bounds,inset)))return true;
        }return false;
    }
    // Clip to the footprint before testing Y: a large slope's remote peak is irrelevant.
    private static boolean supportBelowTop(ControllerMapping map,Triangle t,double[] bounds,double inset){
        List<Point> polygon=new ArrayList<>(List.of(grid(map,t.a),grid(map,t.b),grid(map,t.c)));
        for(int axis:new int[]{0,2})for(int side=0;side<2;side++){
            double edge=bounds[axis+side*3]+(side==0?inset:-inset);var next=new ArrayList<Point>();
            if(polygon.isEmpty())return false;Point before=polygon.get(polygon.size()-1);
            for(var after:polygon){double da=(before.component(axis)-edge)*(side==0?1:-1),db=(after.component(axis)-edge)*(side==0?1:-1);
                if((da>=0)!=(db>=0))next.add(before.add(after.subtract(before).times(da/(da-db))));
                if(db>=0)next.add(after);before=after;
            }polygon=next;
        }
        return !polygon.isEmpty() && polygon.stream().allMatch(p->p.y<bounds[4]-inset);
    }
    /** Same quantized ground cell used by native placement and the temporary editor floor. */
    public Double groundHeight(ControllerMapping map,double x,double z,double referenceY){
        Point start=nativePoint(map,new Point(x,referenceY+3,z)),end=nativePoint(map,new Point(x,referenceY-6,z));
        if(!covered(start) || !covered(end))return null;
        Hit hit=raycast(start,end,true);return hit==null?null:grid(map,hit.point).y;
    }
    public static Point grid(ControllerMapping map,Point p){var q=map.toMinecraft(new Re4Sample(p.x,p.y,p.z,0,0));return new Point(q.x(),q.y(),q.z());}
    public static Point nativePoint(ControllerMapping map,Point p){var q=map.toNative(p.x,p.y,p.z);return new Point(q.x(),q.y(),q.z());}
}
