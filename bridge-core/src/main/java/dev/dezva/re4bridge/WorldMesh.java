package dev.dezva.re4bridge;

import com.google.gson.JsonObject;
import java.nio.*;
import java.util.*;

/** Bounded textured quads in native world space, separate from GUI pixels. */
public final class WorldMesh {
 public static final int MAX_QUADS=128,TEXTURE_SIZE=128,VERTEX_BYTES=24;
 public record Vertex(double x,double y,double z,float u,float v,int color){
  public Vertex {if(!Double.isFinite(x+y+z) || Math.abs(x)>20000000 || Math.abs(y)>20000000 || Math.abs(z)>20000000 || !Float.isFinite(u+v) || u<0 || u>1 || v<0 || v>1)throw new IllegalArgumentException("Invalid world vertex");}
 }
 private final int generation;private final long createdNs;private final JsonObject json;
 public WorldMesh(int generation,List<Vertex> vertices,String texture,long createdNs){
  if(generation<1 || vertices.isEmpty() || vertices.size()%4!=0 || vertices.size()>MAX_QUADS*4 || texture==null || Base64.getDecoder().decode(texture).length>32768)throw new IllegalArgumentException("Invalid world mesh");
  this.generation=generation;this.createdNs=createdNs;
  var bytes=ByteBuffer.allocate(vertices.size()*VERTEX_BYTES).order(ByteOrder.LITTLE_ENDIAN);
  for(var vertex:vertices)bytes.putFloat((float)vertex.x).putFloat((float)vertex.y).putFloat((float)vertex.z).putFloat(vertex.u).putFloat(vertex.v).putInt(vertex.color);
  json=new JsonObject();json.addProperty("encoding","world-quads-v1");json.addProperty("vertices",Base64.getEncoder().encodeToString(bytes.array()));json.addProperty("texture",texture);
 }
 public int generation(){return generation;}public long createdNs(){return createdNs;}
 public JsonObject toJson(){return json.deepCopy();}
}
