package dev.dezva.re4bridge.minecraft;
import org.junit.jupiter.api.Test;
import org.objectweb.asm.*;
import org.objectweb.asm.tree.*;
import static org.junit.jupiter.api.Assertions.*;

class SurvivalTests {
    @Test void escapeAndPauseOptionsAreMenusButChatDoesNotKeepControl(){
        assertTrue(MinecraftHud.pause(new net.minecraft.client.gui.screen.GameMenuScreen(true)));
        assertTrue(MinecraftHud.pause(new net.minecraft.client.gui.screen.Screen(net.minecraft.text.Text.literal("Options")){}));
        assertFalse(MinecraftHud.pause(new net.minecraft.client.gui.screen.ChatScreen("")));
        assertFalse(MinecraftHud.pause((net.minecraft.client.gui.screen.Screen)null));
    }
    @Test void exhaustionAndFoodUseVanillaHungerManager(){
        var hunger=new net.minecraft.entity.player.HungerManager();hunger.setFoodLevel(8);hunger.setSaturationLevel(0);
        hunger.addExhaustion(BridgePlayMode.sprintExhaustion(.5,50_000_000,true));assertEquals(.05f,hunger.getExhaustion(),1e-7);
        hunger.eat(new net.minecraft.component.type.FoodComponent.Builder().nutrition(5).saturationModifier(.6f).build());
        assertEquals(13,hunger.getFoodLevel());assertEquals(6,hunger.getSaturationLevel(),1e-7);
    }
    @Test void sprintExhaustionUsesActualTravelAndRejectsTeleportsIdleAndStaleSteps(){
        assertEquals(.05f,BridgePlayMode.sprintExhaustion(.5,50_000_000,true),1e-7);
        assertEquals(0,BridgePlayMode.sprintExhaustion(0,50_000_000,true));
        assertEquals(0,BridgePlayMode.sprintExhaustion(.5,50_000_000,false));
        assertEquals(0,BridgePlayMode.sprintExhaustion(20,50_000_000,true));
        assertEquals(0,BridgePlayMode.sprintExhaustion(Double.NaN,50_000_000,true));
        assertEquals(0,BridgePlayMode.sprintExhaustion(1,200_000_000,true));
    }
    @Test void capturedVanillaHudIncludesRealHealthFoodArmorAirAndExperience() throws Exception {
        var hud=Class.forName("net.minecraft.client.gui.hud.InGameHud",false,getClass().getClassLoader());
        try(var input=hud.getResourceAsStream("InGameHud.class")){
            var tree=new ClassNode();new ClassReader(input).accept(tree,0);
            var main=tree.methods.stream().filter(m->m.name.equals("renderMainHud")).findFirst().orElseThrow();
            var status=tree.methods.stream().filter(m->m.name.equals("renderStatusBars")).findFirst().orElseThrow();
            var calls=new java.util.HashSet<String>();for(var node:main.instructions)if(node instanceof MethodInsnNode m)calls.add(m.name);
            assertTrue(calls.containsAll(java.util.Set.of("renderHotbar","renderExperienceBar","renderStatusBars")));
            calls.clear();for(var node:status.instructions)if(node instanceof MethodInsnNode m)calls.add(m.name);
            assertTrue(calls.containsAll(java.util.Set.of("renderHealthBar","renderFood","renderArmor","getAir")));
            assertNotNull(hud.getDeclaredMethod("renderExperienceLevel",net.minecraft.client.gui.DrawContext.class,net.minecraft.client.render.RenderTickCounter.class));
        }
    }
    @Test void groundedMiningRedirectMatchesExactlyOneVanillaPenaltyCheck() throws Exception {
        var player=Class.forName("net.minecraft.entity.player.PlayerEntity",false,getClass().getClassLoader());
        try(var input=player.getResourceAsStream("PlayerEntity.class")){
            var tree=new ClassNode();new ClassReader(input).accept(tree,0);
            var method=tree.methods.stream().filter(m->m.name.equals("getBlockBreakingSpeed")).findFirst().orElseThrow();
            int checks=0;for(var node:method.instructions)if(node instanceof MethodInsnNode call && call.owner.equals("net/minecraft/entity/player/PlayerEntity") && call.name.equals("isOnGround") && call.desc.equals("()Z"))checks++;
            assertEquals(1,checks);
        }
    }
}
