package dev.dezva.re4bridge.minecraft;

import org.junit.jupiter.api.Test;
import org.objectweb.asm.*;
import org.objectweb.asm.tree.*;
import static org.junit.jupiter.api.Assertions.*;

class EditorPhysicsTests {
    @Test void leaseCannotAffectForeignPlayersOrSurviveStopExpiryOrReplacement(){
        Object client=new Object(),server=new Object(),other=new Object();boolean[] allowed={true};long now=1_000_000_000L;
        EditorPhysics.follow(client,true,now,()->allowed[0]);EditorPhysics.follow(server,false,now,()->allowed[0]);
        assertTrue(EditorPhysics.active(client,true,now));assertTrue(EditorPhysics.active(server,false,now));
        assertFalse(EditorPhysics.active(other,true,now));assertFalse(EditorPhysics.active(client,false,now));
        assertFalse(EditorPhysics.active(client,true,now-1));assertFalse(EditorPhysics.active(client,true,now+200_000_000));
        allowed[0]=false;assertFalse(EditorPhysics.active(client,true,now));assertFalse(EditorPhysics.active(server,false,now));
        allowed[0]=true;EditorPhysics.follow(other,true,now,()->true);EditorPhysics.release(client,true);
        assertFalse(EditorPhysics.active(client,true,now));assertTrue(EditorPhysics.active(other,true,now)); // late cleanup cannot release a newer owner
        EditorPhysics.release(other,true);EditorPhysics.release(server,false);
        assertFalse(EditorPhysics.active(other,true,now));assertFalse(EditorPhysics.active(server,false,now));
    }
    @Test void injectionTargetsVanillasResetBeforeTheMovementTick() throws Exception {
        var player=Class.forName("net.minecraft.entity.player.PlayerEntity",false,getClass().getClassLoader());
        try(var input=player.getResourceAsStream("PlayerEntity.class")){
            assertNotNull(input);var tree=new ClassNode();new ClassReader(input).accept(tree,0);
            var tick=tree.methods.stream().filter(m->m.name.equals("tick") && m.desc.equals("()V")).findFirst().orElseThrow();
            int resets=0,resetIndex=-1,movementIndex=-1,index=0;
            for(var node:tick.instructions){
                if(node instanceof FieldInsnNode f && f.getOpcode()==Opcodes.PUTFIELD && f.owner.equals("net/minecraft/entity/player/PlayerEntity") && f.name.equals("noClip") && f.desc.equals("Z")){resets++;resetIndex=index;}
                if(node instanceof MethodInsnNode m && m.getOpcode()==Opcodes.INVOKESPECIAL && m.name.equals("tick") && m.owner.equals("net/minecraft/entity/LivingEntity"))movementIndex=index;
                index++;
            }
            assertEquals(1,resets,"Pinned injection must match the exact vanilla reset once");
            assertTrue(movementIndex>resetIndex,"Follower physics must be reapplied before vanilla movement");
        }
    }
}
