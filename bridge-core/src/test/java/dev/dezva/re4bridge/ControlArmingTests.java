package dev.dezva.re4bridge;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
import static dev.dezva.re4bridge.ControlArming.Screen.*;

class ControlArmingTests {
    @Test void transientFocusSuspendsAnExistingSessionButNeverRearmsAfterExpiryOrNativeStop(){
        var gate=new ControlArming();gate.request(0);gate.tick(1,false,true,NONE,true,false,0);int generation=gate.generation();
        gate.tick(1_100_000_000L,false,false,NONE,true,true,0);assertTrue(gate.active());assertTrue(gate.focusSuspended());
        gate.tick(1_299_000_000L,false,true,NONE,true,true,0);assertTrue(gate.active());assertFalse(gate.focusSuspended());assertEquals(generation,gate.generation());
        gate.tick(1_300_000_000L,false,false,NONE,true,true,0);gate.tick(1_500_000_000L,false,false,NONE,true,true,0);assertFalse(gate.active());
        gate.tick(1_501_000_000L,false,true,NONE,true,true,0);assertFalse(gate.active());
        gate.request(2_000_000_000L);gate.tick(2_000_000_001L,false,true,NONE,true,false,0);
        gate.tick(2_000_000_002L,false,false,NONE,true,true,1);assertFalse(gate.active());
        gate.request(3_000_000_000L);gate.tick(3_000_000_001L,false,true,NONE,true,false,0);
        gate.lostFocus(3_100_000_000L);gate.tick(3_300_000_000L,false,true,NONE,true,true,0);assertFalse(gate.active()); // callback records time even if unfocused client ticks pause
    }
    @Test void closingPauseBeforeMinecraftRecomputesItsPauseFlagDoesNotDropArming(){
        var gate=new ControlArming();gate.request(0);gate.tick(1,false,true,NONE,true,false,0);
        gate.tick(1_100_000_000L,true,true,PAUSE,true,true,0);int generation=gate.generation();
        gate.tick(1_150_000_000L,true,true,NONE,true,true,0);assertTrue(gate.active());assertEquals(generation,gate.generation());
        gate.tick(1_200_000_000L,false,true,NONE,true,true,0);assertTrue(gate.active());
        gate.tick(1_400_000_000L,true,true,NONE,true,true,0);assertFalse(gate.active()); // unrelated pause still stops
    }
    @Test void escapePauseKeepsHeartbeatAndGenerationUntilExplicitStopOrLossOfFocus(){
        var gate=new ControlArming();gate.request(0);gate.tick(1,false,true,NONE,true,false,0);int generation=gate.generation();
        for(long tick=2;tick<202;tick++)gate.tick(tick*50_000_000L,true,true,PAUSE,true,true,0);
        assertTrue(gate.active());assertEquals(generation,gate.generation());assertTrue(gate.reason().contains("pausa"));
        gate.tick(11_000_000_000L,false,true,NONE,true,true,0);assertTrue(gate.active());assertEquals(generation,gate.generation());
        gate.tick(11_000_000_001L,true,true,PAUSE,true,true,1);assertFalse(gate.active());
        gate.tick(11_000_000_002L,false,true,NONE,true,true,0);assertFalse(gate.active());
        gate.request(12_000_000_000L);gate.tick(12_000_000_001L,false,true,NONE,true,false,0);
        gate.tick(12_000_000_002L,true,false,PAUSE,true,true,0);assertTrue(gate.active());
        gate.tick(12_200_000_002L,true,false,PAUSE,true,true,0);assertFalse(gate.active());
    }
    @Test void commandChatDoesNotSendArmedInputAndDeadlineStartsAfterClosure() {
        var gate = new ControlArming();
        gate.request(0);
        gate.tick(800_000_000L, false, true, CHAT, true, false, 0);
        assertTrue(gate.pending());
        assertFalse(gate.active());
        gate.tick(900_000_000L, false, true, NONE, true, false, 0);
        assertTrue(gate.active());
        gate.tick(1_200_000_000L, false, true, NONE, true, false, 0);
        assertTrue(gate.active());
        gate.tick(1_300_000_000L, false, true, NONE, true, true, 0);
        assertEquals("activo", gate.reason());
        assertEquals(1, gate.generation());
    }
    @Test void focusLossOrNewScreenRequiresAnotherExplicitArm() {
        var gate = new ControlArming();
        gate.request(0);
        gate.tick(1, false, false, CHAT, true, false, 0);
        assertFalse(gate.pending());
        assertFalse(gate.active());
        gate.tick(2, false, true, NONE, true, true, 0);
        assertFalse(gate.active());
        gate.request(3);
        gate.tick(4, false, true, NONE, true, false, 0);
        assertTrue(gate.active());
        gate.tick(5, false, true, CHAT, true, true, 0);
        assertFalse(gate.active());
        assertEquals(2, gate.generation());
    }
    @Test void stalledAcknowledgementAndExpiredChatDisarmWithReasons() {
        var gate = new ControlArming();
        gate.request(0);
        gate.tick(1, false, true, NONE, true, false, 0);
        gate.tick(1_000_000_001L, false, true, NONE, true, false, 0);
        assertFalse(gate.active());
        assertTrue(gate.reason().contains("host rechazo"));
        gate.request(2_000_000_000L);
        gate.tick(4_000_000_000L, false, true, CHAT, true, false, 0);
        assertFalse(gate.pending());
        assertTrue(gate.reason().contains("chat no cerrado"));
    }
    @Test void pauseAndStaleSamplesCancelPendingArmAndDoNotResume() {
        for (boolean paused : new boolean[] {true, false}) {
            var gate = new ControlArming();
            gate.request(0);
            gate.tick(1, paused, true, NONE, paused, false, 0);
            assertFalse(gate.pending());
            assertFalse(gate.active());
            gate.tick(2, false, true, NONE, true, true, 0);
            assertFalse(gate.active());
        }
    }
    @Test void inventoryPreservesGenerationButFocusOrStopCannotResume() {
        var gate=new ControlArming();gate.request(0);gate.tick(1,false,true,NONE,true,false,0);
        int generation=gate.generation();gate.tick(1_000_000_002L,false,true,INVENTORY,true,true,0);
        assertTrue(gate.active());assertEquals(generation,gate.generation());assertTrue(gate.reason().contains("inventario"));
        gate.tick(1_000_000_003L,false,true,NONE,true,true,0);assertTrue(gate.active());
        gate.tick(1_000_000_004L,false,false,INVENTORY,true,true,0);assertTrue(gate.active());
        gate.tick(1_200_000_004L,false,false,INVENTORY,true,true,0);assertFalse(gate.active());
        gate.tick(1_200_000_005L,false,true,NONE,true,true,0);assertFalse(gate.active());
        gate.request(2_000_000_000L);gate.tick(2_000_000_001L,false,true,NONE,true,false,0);
        gate.tick(3_000_000_002L,false,true,INVENTORY,true,true,1);assertFalse(gate.active());
    }
}
