package dev.dezva.re4bridge;

import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import java.util.List;
import java.util.Base64;

/** A fixed rigid grid. Bounds are in native units relative to its origin. */
public record BlockScene(double x,double y,double z,double yaw,List<Bounds> boxes,int style,
                         boolean collisions,List<String> tiles,List<List<Face>> faces,int colliderCount) {
    public BlockScene(double x,double y,double z,double yaw,List<Bounds> boxes,int style,boolean collisions,List<String> tiles,List<List<Face>> faces){this(x,y,z,yaw,boxes,style,collisions,tiles,faces,boxes.size());}
    public static final int MAX_BOXES=128,MAX_TILES=64;
    public BlockScene(double x,double y,double z,double yaw,List<Bounds> boxes) {this(x,y,z,yaw,boxes,0);}
    public BlockScene(double x,double y,double z,double yaw,List<Bounds> boxes,int style) {this(x,y,z,yaw,boxes,style,false,List.of(),List.of());}
    public record Face(int tile,List<Double> uv) {
        public Face {
            uv=List.copyOf(uv);
            if(tile<0 || tile>=MAX_TILES || uv.size()!=8) throw new IllegalArgumentException("Invalid face material");
            for(double v:uv)if(!Double.isFinite(v) || v<0 || v>1)throw new IllegalArgumentException("Invalid face UV");
        }
        JsonArray toJson() {var a=new JsonArray();a.add(tile);uv.forEach(a::add);return a;}
    }
    public record Bounds(double minX,double minY,double minZ,double maxX,double maxY,double maxZ) {
        public Bounds {
            double[] v={minX,minY,minZ,maxX,maxY,maxZ};
            for(double n:v)if(!Double.isFinite(n) || Math.abs(n)>20_000_000)throw new IllegalArgumentException("Invalid scene coordinates");
            for(int i=0;i<3;i++)if(v[i+3]-v[i]<1 || v[i+3]-v[i]>7500)throw new IllegalArgumentException("Invalid scene shape");
        }
        public JsonArray toJson() {var a=new JsonArray();for(double v:new double[]{minX,minY,minZ,maxX,maxY,maxZ})a.add(v);return a;}
    }
    public BlockScene {
        for(double n:new double[]{x,y,z})if(!Double.isFinite(n) || Math.abs(n)>20_000_000)throw new IllegalArgumentException("Invalid scene origin");
        if(!Double.isFinite(yaw) || Math.abs(yaw)>Math.PI)throw new IllegalArgumentException("Invalid grid yaw");
        boxes=List.copyOf(boxes);tiles=List.copyOf(tiles);faces=faces.stream().map(List::copyOf).toList();
        if(colliderCount<0 || colliderCount>boxes.size())throw new IllegalArgumentException("Invalid collision prefix");
        if(boxes.isEmpty() || boxes.size()>MAX_BOXES || style<0 || style>3)throw new IllegalArgumentException("Invalid scene");
        if(style==3) {
            if(tiles.isEmpty() || tiles.size()>MAX_TILES || faces.size()!=boxes.size())throw new IllegalArgumentException("Invalid materials");
            for(String t:tiles)if(t.length()!=1368 || Base64.getDecoder().decode(t).length!=1024)throw new IllegalArgumentException("Invalid tile");
            for(var group:faces) {
                if(group.size()!=6)throw new IllegalArgumentException("Expected six faces");
                for(var face:group)if(face.tile()>=tiles.size())throw new IllegalArgumentException("Missing tile");
            }
        } else if(!tiles.isEmpty() || !faces.isEmpty())throw new IllegalArgumentException("Materials only apply to style 3");
    }
    public JsonObject toJson() {
        var result=new JsonObject();var origin=new JsonArray();for(double n:new double[]{x,y,z,yaw})origin.add(n);
        result.add("origin",origin);var shapes=new JsonArray();boxes.forEach(b->shapes.add(b.toJson()));result.add("boxes",shapes);
        if(colliderCount!=boxes.size())result.addProperty("collider_count",colliderCount);result.addProperty("style",style);if(collisions)result.addProperty("collisions",true);
        if(style==3) {
            var t=new JsonArray();tiles.forEach(t::add);result.add("tiles",t);var f=new JsonArray();
            for(var group:faces){var a=new JsonArray();group.forEach(face->a.add(face.toJson()));f.add(a);}result.add("faces",f);
        }
        return result;
    }
}
