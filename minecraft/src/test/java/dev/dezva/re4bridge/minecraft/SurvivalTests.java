package dev.dezva.re4bridge.minecraft;
import org.junit.jupiter.api.Test;
import org.objectweb.asm.*;
import org.objectweb.asm.tree.*;
import static org.junit.jupiter.api.Assertions.*;

class SurvivalTests {
    @Test void focusCallbackExistsBeforeVanillaMayPauseAndDiscardKeys() throws Exception {
        assertNotNull(net.minecraft.client.MinecraftClient.class.getDeclaredMethod("onWindowFocusChanged",boolean.class));
        var calls=new java.util.ArrayList<Boolean>();var previous=BridgeKeys.focus;
        try{BridgeKeys.focus=calls::add;BridgeKeys.focus(false);BridgeKeys.focus(true);assertEquals(java.util.List.of(false,true),calls);}finally{BridgeKeys.focus=previous;}
    }
    @Test void survivalDifficultyFixesPeacefulAndPreservesTheChosenDifficulty(){
        var normal=net.minecraft.world.Difficulty.NORMAL;
        assertEquals(normal,BridgePlayMode.survivalDifficulty(net.minecraft.world.Difficulty.PEACEFUL));
        for(var difficulty:new net.minecraft.world.Difficulty[]{net.minecraft.world.Difficulty.EASY,normal,net.minecraft.world.Difficulty.HARD})assertEquals(difficulty,BridgePlayMode.survivalDifficulty(difficulty));
    }
    @Test void nativeFollowerOwnsStandingPoseInsteadOfVanillaRoofCrawling() throws Exception {
        var player=Class.forName("net.minecraft.entity.player.PlayerEntity",false,getClass().getClassLoader());
        assertNotNull(player.getDeclaredMethod("updatePose"));
        // All the automatic pose fallbacks live in the method our lease cancels.
        try(var input=player.getResourceAsStream("PlayerEntity.class")){
            var tree=new ClassNode();new ClassReader(input).accept(tree,0);
            var method=tree.methods.stream().filter(m->m.name.equals("updatePose")).findFirst().orElseThrow();
            var names=new java.util.HashSet<String>();for(var node:method.instructions)if(node instanceof FieldInsnNode field && field.owner.equals("net/minecraft/entity/EntityPose"))names.add(field.name);
            assertTrue(names.containsAll(java.util.Set.of("STANDING","CROUCHING","SWIMMING")));
        }
    }
    private static dev.dezva.re4bridge.HealthState health(long damage,long healing,boolean dead,int mode){return new dev.dezva.re4bridge.HealthState(true,true,true,0,7,4096,800,1000,damage,healing,dead,mode);}
    @Test void nativeHitsAndHealsScaleToMinecraftHeartsAndCannotBeReplayedAfterDeath(){
        var hit=health(200000,0,false,1);assertEquals(16,HealthLink.nextHealth(hit,20,20,0,0,false,false));
        assertEquals(16,HealthLink.nextHealth(hit,16,20,200000,0,false,false));
        assertEquals(32,HealthLink.nextHealth(hit,40,40,0,0,false,false));
        assertEquals(19,HealthLink.nextHealth(health(200000,150000,false,1),16,20,200000,0,false,false));
        assertEquals(20,HealthLink.nextHealth(health(200000,900000,false,1),16,20,200000,0,false,false));
        assertEquals(0,HealthLink.nextHealth(health(1000000,0,true,1),20,20,0,0,false,false));
        assertEquals(20,HealthLink.nextHealth(health(1000000,0,true,1),20,20,1000000,0,true,false)); // respawn outside old session
        assertEquals(20,HealthLink.nextHealth(hit,20,20,0,0,false,true));
        assertEquals(20,HealthLink.nextHealth(health(200000,0,false,2),20,20,0,0,false,false)); // late creative hit when F5 already changed mode
    }
    @Test void f5SwitchesOncePerPressOnlyInsideBridgeAndDoesNotCyclePerspective(){
        var toggles=new java.util.concurrent.atomic.AtomicInteger();
        assertFalse(BridgeKeys.consumeModeKey(false,false,294,1,toggles::incrementAndGet));
        assertFalse(BridgeKeys.consumeModeKey(true,true,294,1,toggles::incrementAndGet));
        assertFalse(BridgeKeys.consumeModeKey(true,false,256,1,toggles::incrementAndGet)); // Esc belongs to vanilla
        assertTrue(BridgeKeys.consumeModeKey(true,false,294,1,toggles::incrementAndGet));
        for(int i=0;i<100;i++)assertTrue(BridgeKeys.consumeModeKey(true,false,294,2,toggles::incrementAndGet));
        assertTrue(BridgeKeys.consumeModeKey(true,false,294,0,toggles::incrementAndGet));assertEquals(1,toggles.get());
    }
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
 @Test void pickupDialogKeysConfirmOnceAndLeaveOtherKeysToMinecraft(){
        var sent=new java.util.ArrayList<Integer>();
        assertFalse(BridgeKeys.consumeDialogKey(false,257,1,sent::add));
        for(int key:new int[]{257,335,82,259})for(int phase:new int[]{1,2,2,0})assertTrue(BridgeKeys.consumeDialogKey(true,key,phase,sent::add));
        assertEquals(java.util.List.of(6,6,6,7),sent);sent.clear();
        for(int key:new int[]{265,264,263,262})assertTrue(BridgeKeys.consumeDialogKey(true,key,1,sent::add));
        assertEquals(java.util.List.of(2,3,4,5),sent);
        for(int i=0;i<20;i++)assertTrue(BridgeKeys.consumeDialogKey(true,262,2,sent::add));
        assertEquals(4,sent.size());assertFalse(BridgeKeys.consumeDialogKey(true,256,1,sent::add));
        assertFalse(BridgeKeys.consumeDialogKey(true,87,1,sent::add));
    }
 @Test void contextualAndSkipKeysProduceOneRequestPerPressAndKeepEscFree(){
        var action=new java.util.concurrent.atomic.AtomicInteger();var skip=new java.util.concurrent.atomic.AtomicInteger();
        for(int phase:new int[]{1,2,2,0}){
            assertTrue(BridgeKeys.consumeActionKey(82,phase,action::incrementAndGet,skip::incrementAndGet));
            assertTrue(BridgeKeys.consumeActionKey(295,phase,action::incrementAndGet,skip::incrementAndGet));
        }
        assertEquals(1,action.get());assertEquals(1,skip.get());assertFalse(BridgeKeys.consumeActionKey(256,1,action::incrementAndGet,skip::incrementAndGet));
    }
}
