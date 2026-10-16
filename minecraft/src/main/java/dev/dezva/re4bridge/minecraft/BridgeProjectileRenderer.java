package dev.dezva.re4bridge.minecraft;

import com.mojang.blaze3d.systems.RenderSystem;
import com.mojang.blaze3d.systems.VertexSorter;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.render.DiffuseLighting;
import net.minecraft.client.render.VertexConsumerProvider;
import net.minecraft.client.util.math.MatrixStack;
import net.minecraft.entity.projectile.ArrowEntity;
import net.minecraft.entity.projectile.SpectralArrowEntity;
import org.joml.Matrix4f;

/** Vanilla arrow models in the existing transparent overlay; no extra framebuffer. */
final class BridgeProjectileRenderer {
 static void render(MinecraftClient client,float delta,int width,int height,double fov,VertexConsumerProvider.Immediate consumers){
  if(client.world==null || client.player==null || BridgeCombat.projectileGeneration()==0)return;
  var camera=client.gameRenderer.getCamera();var eye=camera.getPos();
  var projection=new Matrix4f(RenderSystem.getProjectionMatrix());var sorter=RenderSystem.getVertexSorting();
  var model=RenderSystem.getModelViewStack();model.pushMatrix();
  var dispatcher=client.getEntityRenderDispatcher();
  try{
   RenderSystem.setProjectionMatrix(new Matrix4f().perspective((float)Math.toRadians(fov),(float)width/height,.05f,128),VertexSorter.BY_DISTANCE);
   model.identity().rotateX((float)Math.toRadians(camera.getPitch())).rotateY((float)Math.toRadians(camera.getYaw()+180));RenderSystem.applyModelViewMatrix();
   RenderSystem.enableDepthTest();DiffuseLighting.enableForLevel();client.gameRenderer.getLightmapTextureManager().enable();
   int count=0;
   for(var entity:client.world.getEntities()){
    if(!(entity instanceof net.minecraft.entity.projectile.PersistentProjectileEntity arrow) || !(arrow instanceof BridgeArrow managed) || !managed.re4bridge$managed() || (!(arrow instanceof ArrowEntity) && !(arrow instanceof SpectralArrowEntity)) || arrow.isRemoved() || arrow.getOwner()!=client.player || arrow.squaredDistanceTo(eye)>64*64)continue;
    if(++count>16)break;
    var position=arrow.getLerpedPos(delta).subtract(eye);
    dispatcher.render(arrow,position.x,position.y,position.z,arrow.getYaw(delta),delta,new MatrixStack(),consumers,15728880);
   }
   consumers.draw();
  }finally{
   client.gameRenderer.getLightmapTextureManager().disable();RenderSystem.depthMask(true);RenderSystem.disableBlend();
   model.popMatrix();RenderSystem.applyModelViewMatrix();RenderSystem.setProjectionMatrix(projection,sorter);
  }
 }
 private BridgeProjectileRenderer(){}
}
