package dev.dezva.re4bridge.minecraft;

import com.mojang.blaze3d.systems.RenderSystem;
import com.mojang.blaze3d.platform.GlStateManager;
import com.mojang.blaze3d.systems.VertexSorter;
import net.minecraft.client.gl.ShaderProgram;
import org.joml.Matrix4f;
import org.joml.Vector3f;
import org.lwjgl.opengl.GL11;
import org.lwjgl.opengl.GL13;
import org.lwjgl.opengl.GL14;
import org.lwjgl.opengl.GL15;
import org.lwjgl.opengl.GL20;
import org.lwjgl.opengl.GL21;
import org.lwjgl.opengl.GL30;

/** A render-thread lease: offscreen hotbar work must leave vanilla's GUI intact. */
final class GuiRenderState implements AutoCloseable {
    private final Matrix4f projection=new Matrix4f(RenderSystem.getProjectionMatrix()),textureMatrix=new Matrix4f(RenderSystem.getTextureMatrix());
    private final VertexSorter sorting=RenderSystem.getVertexSorting();
    private final ShaderProgram shader=RenderSystem.getShader(),activeProgram=ShaderProgram.activeProgram;
    private final int activeProgramRef=ShaderProgram.activeProgramGlRef;
    private final float[] color=RenderSystem.getShaderColor().clone();
    private final Vector3f light0=new Vector3f(RenderSystem.shaderLightDirections[0]),light1=new Vector3f(RenderSystem.shaderLightDirections[1]);
    private final int read=GL11.glGetInteger(GL30.GL_READ_FRAMEBUFFER_BINDING),draw=GL11.glGetInteger(GL30.GL_DRAW_FRAMEBUFFER_BINDING);
    private final int program=GL11.glGetInteger(GL20.GL_CURRENT_PROGRAM),vao=GL11.glGetInteger(GL30.GL_VERTEX_ARRAY_BINDING),array=GL11.glGetInteger(GL15.GL_ARRAY_BUFFER_BINDING);
    private final int packBuffer=GL11.glGetInteger(GL21.GL_PIXEL_PACK_BUFFER_BINDING),packAlignment=GL11.glGetInteger(GL11.GL_PACK_ALIGNMENT),packRowLength=GL11.glGetInteger(GL11.GL_PACK_ROW_LENGTH),packSkipRows=GL11.glGetInteger(GL11.GL_PACK_SKIP_ROWS),packSkipPixels=GL11.glGetInteger(GL11.GL_PACK_SKIP_PIXELS);
    private final int[] viewport=new int[4],scissor=new int[4],textures=new int[12],shaderTextures=new int[12];
    private final float[] clearColor=new float[4];
    private final double clearDepth=GL11.glGetDouble(GL11.GL_DEPTH_CLEAR_VALUE);
    private final boolean scissorOn=GL11.glIsEnabled(GL11.GL_SCISSOR_TEST),blend=GL11.glIsEnabled(GL11.GL_BLEND),depth=GL11.glIsEnabled(GL11.GL_DEPTH_TEST),cull=GL11.glIsEnabled(GL11.GL_CULL_FACE),depthMask=GL11.glGetBoolean(GL11.GL_DEPTH_WRITEMASK);
    private final int srcRgb=GL11.glGetInteger(GL14.GL_BLEND_SRC_RGB),dstRgb=GL11.glGetInteger(GL14.GL_BLEND_DST_RGB),srcAlpha=GL11.glGetInteger(GL14.GL_BLEND_SRC_ALPHA),dstAlpha=GL11.glGetInteger(GL14.GL_BLEND_DST_ALPHA),equation=GL11.glGetInteger(GL20.GL_BLEND_EQUATION_RGB),equationAlpha=GL11.glGetInteger(GL20.GL_BLEND_EQUATION_ALPHA),depthFunc=GL11.glGetInteger(GL11.GL_DEPTH_FUNC),active=GL11.glGetInteger(GL13.GL_ACTIVE_TEXTURE);
    private final boolean[] colorMask=new boolean[4];
    GuiRenderState(){
        GL11.glGetIntegerv(GL11.GL_VIEWPORT,viewport);GL11.glGetIntegerv(GL11.GL_SCISSOR_BOX,scissor);GL11.glGetFloatv(GL11.GL_COLOR_CLEAR_VALUE,clearColor);
        var mask=org.lwjgl.BufferUtils.createByteBuffer(4);GL11.glGetBooleanv(GL11.GL_COLOR_WRITEMASK,mask);for(int i=0;i<4;i++)colorMask[i]=mask.get(i)!=0;
        for(int i=0;i<12;i++){RenderSystem.activeTexture(GL13.GL_TEXTURE0+i);textures[i]=GL11.glGetInteger(GL11.GL_TEXTURE_BINDING_2D);shaderTextures[i]=RenderSystem.getShaderTexture(i);}
        RenderSystem.activeTexture(active);RenderSystem.getModelViewStack().pushMatrix();
        // Readback must not inherit a PBO or an unusual pack layout from another mod.
        GL15.glBindBuffer(GL21.GL_PIXEL_PACK_BUFFER,0);GL11.glPixelStorei(GL11.GL_PACK_ALIGNMENT,1);GL11.glPixelStorei(GL11.GL_PACK_ROW_LENGTH,0);GL11.glPixelStorei(GL11.GL_PACK_SKIP_ROWS,0);GL11.glPixelStorei(GL11.GL_PACK_SKIP_PIXELS,0);
    }
    @Override public void close(){
        RenderSystem.getModelViewStack().popMatrix();RenderSystem.applyModelViewMatrix();RenderSystem.setProjectionMatrix(projection,sorting);RenderSystem.setTextureMatrix(textureMatrix);
        RenderSystem.setShader(()->shader);RenderSystem.setShaderColor(color[0],color[1],color[2],color[3]);RenderSystem.setShaderLights(light0,light1);
        for(int i=0;i<12;i++){RenderSystem.setShaderTexture(i,shaderTextures[i]);RenderSystem.activeTexture(GL13.GL_TEXTURE0+i);RenderSystem.bindTexture(textures[i]);}RenderSystem.activeTexture(active);
        RenderSystem.blendFuncSeparate(srcRgb,dstRgb,srcAlpha,dstAlpha);GL20.glBlendEquationSeparate(equation,equationAlpha);
        if(blend)RenderSystem.enableBlend();else RenderSystem.disableBlend();if(depth)RenderSystem.enableDepthTest();else RenderSystem.disableDepthTest();if(cull)RenderSystem.enableCull();else RenderSystem.disableCull();
        RenderSystem.depthFunc(depthFunc);RenderSystem.depthMask(depthMask);RenderSystem.colorMask(colorMask[0],colorMask[1],colorMask[2],colorMask[3]);
        RenderSystem.clearDepth(clearDepth);RenderSystem.clearColor(clearColor[0],clearColor[1],clearColor[2],clearColor[3]);
        if(scissorOn)RenderSystem.enableScissor(scissor[0],scissor[1],scissor[2],scissor[3]);else RenderSystem.disableScissor();
        GL30.glBindFramebuffer(GL30.GL_READ_FRAMEBUFFER,read);GL30.glBindFramebuffer(GL30.GL_DRAW_FRAMEBUFFER,draw);RenderSystem.viewport(viewport[0],viewport[1],viewport[2],viewport[3]);
        net.minecraft.client.render.BufferRenderer.resetCurrentVertexBuffer();
        ShaderProgram.activeProgram=activeProgram;ShaderProgram.activeProgramGlRef=activeProgramRef;
        RenderSystem.glBindVertexArray(vao);RenderSystem.glBindBuffer(GL15.GL_ARRAY_BUFFER,array);GlStateManager._glUseProgram(program);
        GL15.glBindBuffer(GL21.GL_PIXEL_PACK_BUFFER,packBuffer);GL11.glPixelStorei(GL11.GL_PACK_ALIGNMENT,packAlignment);GL11.glPixelStorei(GL11.GL_PACK_ROW_LENGTH,packRowLength);GL11.glPixelStorei(GL11.GL_PACK_SKIP_ROWS,packSkipRows);GL11.glPixelStorei(GL11.GL_PACK_SKIP_PIXELS,packSkipPixels);
    }
}
