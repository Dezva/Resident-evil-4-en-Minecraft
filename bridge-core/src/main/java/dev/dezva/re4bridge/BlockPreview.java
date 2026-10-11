package dev.dezva.re4bridge;

import com.google.gson.JsonArray;
import com.google.gson.JsonObject;

/** One fixed native-world box, anchored explicitly ahead of the sampled player. */
public record BlockPreview(double x, double y, double z, double size) {
    public BlockPreview {
        if (!Double.isFinite(x+y+z+size) || Math.abs(x)>19_990_000 || Math.abs(y)>19_990_000
                || Math.abs(z)>19_990_000 || size<40 || size>2500)
            throw new IllegalArgumentException("Invalid preview box");
    }
    public static BlockPreview ahead(Re4Sample sample, double size) {
        return new BlockPreview(sample.x()+Math.sin(sample.yawRadians())*size*3,
            sample.y(),sample.z()+Math.cos(sample.yawRadians())*size*3,size);
    }
    public JsonObject toJson() {
        var bounds=new JsonArray();
        for (double value:new double[]{x-size/2,y,z-size/2,x+size/2,y+size,z+size/2}) bounds.add(value);
        var result=new JsonObject(); result.add("bounds",bounds); return result;
    }
}
