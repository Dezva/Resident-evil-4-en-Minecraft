package dev.dezva.re4bridge;
import java.util.*;
import java.nio.*;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
class WorldMeshTests {
 @Test void preservesNativePositionTextureCoordinatesColorAndImmutability(){
  var vertex=new WorldMesh.Vertex(100,200,-300,.25f,.5f,0xFF112233);var vertices=new ArrayList<>(Collections.nCopies(4,vertex));
  var mesh=new WorldMesh(7,vertices,"eA==",1);vertices.clear();var json=mesh.toJson();var bytes=ByteBuffer.wrap(Base64.getDecoder().decode(json.get("vertices").getAsString())).order(ByteOrder.LITTLE_ENDIAN);
  assertEquals(96,bytes.remaining());assertEquals(100,bytes.getFloat());assertEquals(200,bytes.getFloat());assertEquals(-300,bytes.getFloat());assertEquals(.25f,bytes.getFloat());assertEquals(.5f,bytes.getFloat());assertEquals(0xFF112233,bytes.getInt());
  json.addProperty("encoding","corrupted");assertEquals("world-quads-v1",mesh.toJson().get("encoding").getAsString());
 }
 @Test void anOversizedVisualFrameCannotDisconnectOrTruncateControlAndScene() throws Exception {
  var frame=new com.google.gson.JsonObject();frame.addProperty("protocol",1);frame.addProperty("type","re4_control");frame.addProperty("armed",true);frame.addProperty("seq",8);frame.addProperty("preview","x".repeat(LineProtocol.MAX_FRAME_BYTES-160));
  var mesh=new WorldMesh(7,Collections.nCopies(4,new WorldMesh.Vertex(1,2,3,0,0,0)),"eA==",1);frame.add("world_mesh",mesh.toJson());
  assertThrows(java.io.IOException.class,()->LineProtocol.encode(frame));
  byte[] encoded=LineProtocol.encodeControl(frame);assertTrue(encoded.length<=LineProtocol.MAX_FRAME_BYTES+1);
  var received=LineProtocol.read(new java.io.ByteArrayInputStream(encoded));assertFalse(received.has("world_mesh"));assertTrue(frame.has("world_mesh"));assertEquals(frame.get("preview"),received.get("preview"));assertTrue(received.get("armed").getAsBoolean());assertEquals(8,received.get("seq").getAsInt());
 }
 @Test void rejectsUnboundedNonfiniteAndIncompleteGeometry(){
  var vertex=new WorldMesh.Vertex(0,0,0,0,0,0);
  for(int count:new int[]{0,3,513,516})assertThrows(IllegalArgumentException.class,()->new WorldMesh(1,Collections.nCopies(count,vertex),"eA==",1));
  assertThrows(IllegalArgumentException.class,()->new WorldMesh.Vertex(Double.NaN,0,0,0,0,0));assertThrows(IllegalArgumentException.class,()->new WorldMesh.Vertex(0,0,0,1.1f,0,0));
  assertThrows(IllegalArgumentException.class,()->new WorldMesh(0,Collections.nCopies(4,vertex),"eA==",1));
 }
}
