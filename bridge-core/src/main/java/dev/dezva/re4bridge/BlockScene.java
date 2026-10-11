package dev.dezva.re4bridge;

import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import java.util.List;

/** A fixed rigid grid. Bounds are in native units relative to its origin. */
public record BlockScene(double x,double y,double z,double yaw,List<Bounds> boxes,int style) {
    public static final int MAX_BOXES=128;
    public BlockScene(double x,double y,double z,double yaw,List<Bounds> boxes) {this(x,y,z,yaw,boxes,0);}
    public record Bounds(double minX,double minY,double minZ,double maxX,double maxY,double maxZ) {
        public Bounds {
            double[] v={minX,minY,minZ,maxX,maxY,maxZ};
            for (double n:v) if (!Double.isFinite(n) || Math.abs(n)>20_000_000)
                throw new IllegalArgumentException("Invalid scene coordinates");
            for (int i=0;i<3;i++) if (v[i+3]-v[i]<1 || v[i+3]-v[i]>7500)
                throw new IllegalArgumentException("Invalid scene shape");
        }
        public JsonArray toJson() {
            var result=new JsonArray();
            for (double n:new double[]{minX,minY,minZ,maxX,maxY,maxZ}) result.add(n);
            return result;
        }
    }
    public BlockScene {
        for (double n:new double[]{x,y,z}) if (!Double.isFinite(n) || Math.abs(n)>20_000_000)
            throw new IllegalArgumentException("Invalid scene origin");
        if (!Double.isFinite(yaw) || Math.abs(yaw)>Math.PI) throw new IllegalArgumentException("Invalid grid yaw");
        boxes=List.copyOf(boxes);
        if (boxes.isEmpty() || boxes.size()>MAX_BOXES) throw new IllegalArgumentException("Scene shape limit");
        if (style<0 || style>1) throw new IllegalArgumentException("Invalid scene style");
    }
    public JsonObject toJson() {
        var result=new JsonObject();var origin=new JsonArray();
        for (double n:new double[]{x,y,z,yaw}) origin.add(n);
        result.add("origin",origin);var shapes=new JsonArray();
        for (var box:boxes) shapes.add(box.toJson());result.add("boxes",shapes);
        result.addProperty("style",style);
        return result;
    }
}
