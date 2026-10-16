package dev.dezva.re4bridge.minecraft;

import org.junit.jupiter.api.Test;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.Opcodes;
import org.objectweb.asm.tree.*;
import static org.junit.jupiter.api.Assertions.*;

class BridgeArrowLifecycleTests {
 private ClassNode bytecode(String name) throws Exception {
  try(var input=getClass().getClassLoader().getResourceAsStream(name.replace('.','/')+".class")){
   assertNotNull(input);var result=new ClassNode();new ClassReader(input).accept(result,0);return result;
  }
 }
 private static AbstractInsnNode previousCode(AbstractInsnNode node){do{node=node.getPrevious();}while(node!=null && node.getOpcode()<0);return node;}
 @Test void bothRealWorldDispatchersAdmitTheFirstArrowTick() throws Exception {
  for(var world:new String[]{"net.minecraft.server.world.ServerWorld","net.minecraft.client.world.ClientWorld"}){
   var tick=bytecode(world).methods.stream().filter(m->m.name.equals("tickEntity") && m.desc.equals("(Lnet/minecraft/entity/Entity;)V")).findFirst().orElseThrow();
   boolean ageWritten=false,ticked=false;
   for(var node:tick.instructions){
    if(node instanceof FieldInsnNode field && field.getOpcode()==Opcodes.PUTFIELD && field.name.equals("age")){
     var add=previousCode(node);assertEquals(Opcodes.IADD,add.getOpcode());
     var increment=previousCode(add);assertEquals(Opcodes.ICONST_1,increment.getOpcode());ageWritten=true;
    }
    if(node instanceof MethodInsnNode call && call.name.equals("tick") && call.desc.equals("()V")){
     assertTrue(ageWritten,"World increments age BEFORE the arrow's tick HEAD: "+world);
     int ageAtHead=0+1; // initial spawn age plus the verified vanilla increment
     assertEquals(7,BridgeArrow.launchGeneration(ageAtHead,7,true),world);ticked=true;break;
    }
   }
   assertTrue(ticked,world);
  }
 }
 @Test void oldForeignAndUnarmedArrowsAreNotAdopted(){
  assertEquals(7,BridgeArrow.launchGeneration(0,7,true));
  assertEquals(0,BridgeArrow.launchGeneration(2,7,true));assertEquals(0,BridgeArrow.launchGeneration(80,7,true));
  assertEquals(0,BridgeArrow.launchGeneration(1,0,true));assertEquals(0,BridgeArrow.launchGeneration(1,7,false));
  assertEquals(0,BridgeArrow.launchGeneration(-1,7,true));
 }
 @Test void theActualArrowHookUsesTheTestedAdmissionAndLatchesItsSpawnCheck() throws Exception {
  var hook=bytecode("dev.dezva.re4bridge.minecraft.mixin.TerrainArrowMixin").methods.stream().filter(m->m.name.equals("re4bridge$unknownTerrain")).findFirst().orElseThrow();
  boolean checked=false,admitted=false;
  for(var node:hook.instructions){
   if(node instanceof FieldInsnNode field && field.getOpcode()==Opcodes.PUTFIELD && field.name.equals("re4bridge$spawnChecked"))checked=true;
   if(node instanceof MethodInsnNode call && call.owner.equals("dev/dezva/re4bridge/minecraft/BridgeArrow") && call.name.equals("launchGeneration")){
    assertTrue(checked,"Latch even an unarmed first tick; never adopt an old arrow later");admitted=true;
   }
  }
  assertTrue(admitted);
 }
}
