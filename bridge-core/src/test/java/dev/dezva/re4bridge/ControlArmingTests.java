package dev.dezva.re4bridge;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
import static dev.dezva.re4bridge.ControlArming.Screen.*;

class ControlArmingTests {
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
}
