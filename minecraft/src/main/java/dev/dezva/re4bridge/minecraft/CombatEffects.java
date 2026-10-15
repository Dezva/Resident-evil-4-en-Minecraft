package dev.dezva.re4bridge.minecraft;

import com.mojang.blaze3d.systems.RenderSystem;
import com.mojang.blaze3d.systems.VertexSorter;
import dev.dezva.re4bridge.*;
import java.util.*;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.particle.*;
import net.minecraft.client.render.*;
import net.minecraft.particle.ParticleTypes;
import org.joml.Matrix4f;

/** Only confirmed native impacts; use Minecraft's factories, sprites and interpolation.
 * Keep these references separate so the RE4 overlay never renders superflat weather. */
final class CombatEffects {
    private static final ArrayDeque<Particle> particles=new ArrayDeque<>();
    private static Object world;
    static void clear(){for(var p:particles)p.markDead();particles.clear();world=null;}
    static boolean living(int kind){return kind<0x50 && kind!=0x2A;}
    static int hearts(double damage){return Math.max(1,Math.min(8,(int)(damage*.5)));}
    private static void add(Particle p){if(p==null)return;if(particles.size()==64)particles.removeFirst().markDead();particles.addLast(p);}
    static void hit(MinecraftClient client,ControllerMapping map,CombatState state,boolean sword){
        if(client.world==null || state.impact()==null)return;
        if(world!=client.world){clear();world=client.world;}
        var hit=state.impact();var pos=map.toMinecraft(new Re4Sample(hit.x(),hit.y(),hit.z(),0,System.nanoTime()));
        if(living(state.targetKind())){
            int count=hearts((state.hpBefore()-state.hpAfter())/50.);
            for(int i=0;i<count;i++)add(client.particleManager.addParticle(ParticleTypes.DAMAGE_INDICATOR,pos.x(),pos.y()+.1,pos.z(),(Math.random()-.5)*.15,.1+Math.random()*.15,(Math.random()-.5)*.15));
        }
        if(sword)add(client.particleManager.addParticle(ParticleTypes.SWEEP_ATTACK,pos.x(),pos.y(),pos.z(),0,0,0));
    }
    static void render(MinecraftClient client,float delta,int width,int height,double fov){
        if(world!=client.world){clear();return;}
        particles.removeIf(p->!p.isAlive());if(particles.isEmpty())return;
        var camera=client.gameRenderer.getCamera();var light=client.gameRenderer.getLightmapTextureManager();
        var projection=new Matrix4f(RenderSystem.getProjectionMatrix());var sorter=RenderSystem.getVertexSorting();
        var matrices=RenderSystem.getModelViewStack();matrices.pushMatrix();
        try{
            RenderSystem.setProjectionMatrix(new Matrix4f().perspective((float)Math.toRadians(fov),(float)width/height,.05f,1000),VertexSorter.BY_DISTANCE);
            matrices.identity().rotateX((float)Math.toRadians(camera.getPitch())).rotateY((float)Math.toRadians(camera.getYaw()+180));RenderSystem.applyModelViewMatrix();
            light.enable();RenderSystem.enableDepthTest();
            for(var sheet:List.of(ParticleTextureSheet.PARTICLE_SHEET_OPAQUE,ParticleTextureSheet.PARTICLE_SHEET_TRANSLUCENT,ParticleTextureSheet.PARTICLE_SHEET_LIT)){
                if(particles.stream().noneMatch(p->p.getType()==sheet))continue;
                RenderSystem.setShader(GameRenderer::getParticleProgram);
                var builder=sheet.begin(Tessellator.getInstance(),client.getTextureManager());if(builder==null)continue;
                for(var p:particles)if(p.getType()==sheet)p.buildGeometry(builder,camera,delta);
                var built=builder.endNullable();if(built!=null)BufferRenderer.drawWithGlobalProgram(built);
            }
        }finally{
            RenderSystem.depthMask(true);RenderSystem.disableBlend();light.disable();
            matrices.popMatrix();RenderSystem.applyModelViewMatrix();RenderSystem.setProjectionMatrix(projection,sorter);
        }
    }
}
