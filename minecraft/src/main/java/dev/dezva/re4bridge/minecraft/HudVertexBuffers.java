package dev.dezva.re4bridge.minecraft;

import java.util.LinkedHashMap;
import net.minecraft.client.render.RenderLayer;
import net.minecraft.client.render.VertexConsumerProvider;
import net.minecraft.client.util.BufferAllocator;

/** Separate glint layers must stay open alongside their base item layer.
 * A single fallback buffer would flush an enchanted item's glint too early. */
final class HudVertexBuffers implements AutoCloseable {
    private final BufferAllocator fallback=new BufferAllocator(1<<20);
    private final LinkedHashMap<RenderLayer,BufferAllocator> layers=new LinkedHashMap<>();
    final VertexConsumerProvider.Immediate consumers;
    HudVertexBuffers(){
        try{
            for(var layer:new RenderLayer[]{RenderLayer.getArmorEntityGlint(),RenderLayer.getGlintTranslucent(),RenderLayer.getGlint(),RenderLayer.getEntityGlint(),RenderLayer.getDirectEntityGlint()})layers.put(layer,new BufferAllocator(4096));
            consumers=VertexConsumerProvider.immediate(layers,fallback);
        }catch(RuntimeException error){close();throw error;}
    }
    @Override public void close(){for(var buffer:layers.values())buffer.close();fallback.close();}
}
