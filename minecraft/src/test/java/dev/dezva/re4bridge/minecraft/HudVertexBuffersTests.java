package dev.dezva.re4bridge.minecraft;

import net.minecraft.client.render.BufferBuilder;
import net.minecraft.client.render.RenderLayer;
import net.minecraft.client.render.VertexConsumer;
import net.minecraft.util.Identifier;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

class HudVertexBuffersTests {
    @Test void enchantedItemBaseAndGlintStayOpenTogether(){
        try(var buffers=new HudVertexBuffers()){
            var glint=(BufferBuilder)buffers.consumers.getBuffer(RenderLayer.getGlint());
            var base=(BufferBuilder)buffers.consumers.getBuffer(RenderLayer.getEntitySolid(Identifier.ofVanilla("textures/atlas/blocks.png")));
            // If glint were a fallback, requesting base would end it before these vertices.
            assertDoesNotThrow(()->{
                for(int i=0;i<4;i++){
                    glint.vertex(i%2,i/2,0).texture(i%2,i/2);
                    base.vertex(i%2,i/2,0).color(-1).texture(i%2,i/2).overlay(0).light(15728880).normal(0,0,1);
                }
            });
            try(var g=glint.end();var b=base.end()){assertNotNull(g);assertNotNull(b);}
        }
    }
}
