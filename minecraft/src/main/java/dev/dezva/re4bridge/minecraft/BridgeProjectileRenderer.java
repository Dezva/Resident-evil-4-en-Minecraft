package dev.dezva.re4bridge.minecraft;

import dev.dezva.re4bridge.WorldMesh;
import java.nio.*;
import java.util.*;
import java.util.zip.Deflater;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.render.*;
import net.minecraft.client.texture.NativeImage;
import net.minecraft.client.util.math.MatrixStack;
import net.minecraft.entity.projectile.*;
import net.minecraft.util.Identifier;

/** Capture vanilla model vertices, without drawing projectiles into GUI pixels. */
final class BridgeProjectileRenderer {
 private record Tile(int slot){}
 private static Object resources;
 private static final Map<Identifier,Tile> tiles=new HashMap<>();
 private static final int[] pixels=new int[128*128];
 private static String texture;
 private static Tile tile(MinecraftClient client,Identifier id){
  var epoch=client.getBlockRenderManager().getModels().getModel(net.minecraft.block.Blocks.STONE.getDefaultState());
  if(resources!=epoch){resources=epoch;tiles.clear();Arrays.fill(pixels,0);texture=null;}
  if(tiles.containsKey(id))return tiles.get(id);
  if(tiles.size()==4)return null;
  try(var stream=client.getResourceManager().getResourceOrThrow(id).getInputStream();var image=NativeImage.read(stream)){
   int slot=tiles.size(),ox=(slot%2)*64,oy=(slot/2)*64;
   for(int y=0;y<64;y++)for(int x=0;x<64;x++){
    int c=image.getColor(x*image.getWidth()/64,y*image.getHeight()/64);
    pixels[(oy+y)*128+ox+x]=(c&0xFF00FF00)|((c&255)<<16)|((c>>>16)&255);
   }
   var tile=new Tile(slot);tiles.put(id,tile);texture=null;return tile;
  }catch(java.io.IOException error){return null;}
 }
 private static String texture(){
  if(texture!=null)return texture;
  var bytes=ByteBuffer.allocate(pixels.length*4).order(ByteOrder.LITTLE_ENDIAN);for(int c:pixels)bytes.putInt(c);
  var deflater=new Deflater(1);try{deflater.setInput(bytes.array());deflater.finish();byte[] result=new byte[32768];int size=deflater.deflate(result);if(!deflater.finished())return null;return texture=Base64.getEncoder().encodeToString(Arrays.copyOf(result,size));}finally{deflater.end();}
 }
 static void render(MinecraftClient client,float delta,int width,int height,double fov,VertexConsumerProvider.Immediate unused){
  var map=BridgeCombat.visualMapping();int gen=BridgeCombat.projectileGeneration();
  if(client.world==null || client.player==null || map==null || gen==0){BridgeCombat.visuals(null);return;}
  var vertices=new ArrayList<WorldMesh.Vertex>();int count=0;
  for(var entity:client.world.getEntities()){
   if(!(entity instanceof PersistentProjectileEntity arrow) || !(arrow instanceof BridgeArrow managed) || !managed.re4bridge$managed() || !managed.re4bridge$visible() || (!(arrow instanceof ArrowEntity) && !(arrow instanceof SpectralArrowEntity)) || arrow.isRemoved() || arrow.getOwner()!=client.player || arrow.squaredDistanceTo(client.player)>64*64)continue;
   if(++count>16)break;
   var renderer=client.getEntityRenderDispatcher().getRenderer(arrow);var tile=tile(client,renderer.getTexture(arrow));if(tile==null)continue;
   var matrices=new MatrixStack();var pos=arrow.getLerpedPos(delta);matrices.translate(pos.x,pos.y,pos.z);
   var collector=new Collector(vertices,map,tile.slot());var discard=new Collector(new ArrayList<>(),map,tile.slot());var layer=RenderLayer.getEntityCutout(renderer.getTexture(arrow));
   renderer.render(arrow,arrow.getYaw(delta),delta,matrices,wanted->wanted==layer?collector:discard,15728880);
   collector.finish();
   if(managed.re4bridge$markRendered())ProjectileTrace.event(gen,"world-submit",arrow.getId(),"vanilla quads");
  }
  String atlas=vertices.isEmpty()?null:texture();
  BridgeCombat.visuals(atlas==null?null:new WorldMesh(gen,vertices,atlas,System.nanoTime()));
 }
 static final class Collector implements VertexConsumer {
  final List<WorldMesh.Vertex> output;final dev.dezva.re4bridge.ControllerMapping map;final int tile;
  float x,y,z,u,v;int color=0xFFFFFFFF;boolean pending;
  Collector(List<WorldMesh.Vertex> output,dev.dezva.re4bridge.ControllerMapping map,int tile){this.output=output;this.map=map;this.tile=tile;}
  void finish(){if(pending){if(output.size()<WorldMesh.MAX_QUADS*4){var p=map.toNative(x,y,z);output.add(new WorldMesh.Vertex(p.x(),p.y(),p.z(),(tile%2+u)*.5f,(tile/2+v)*.5f,color));}pending=false;}}
  public VertexConsumer vertex(float x,float y,float z){finish();this.x=x;this.y=y;this.z=z;u=v=0;color=0xFFFFFFFF;pending=true;return this;}
  public VertexConsumer color(int r,int g,int b,int a){color=(a<<24)|(r<<16)|(g<<8)|b;return this;}
  public VertexConsumer texture(float u,float v){this.u=u;this.v=v;return this;}
  public VertexConsumer overlay(int u,int v){return this;}
  public VertexConsumer light(int u,int v){return this;}
  public VertexConsumer normal(float x,float y,float z){return this;}
 }
 private BridgeProjectileRenderer(){}
}
