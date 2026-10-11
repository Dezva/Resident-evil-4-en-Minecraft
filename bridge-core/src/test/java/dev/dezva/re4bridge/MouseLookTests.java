package dev.dezva.re4bridge;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
class MouseLookTests {
    @Test void relativeYawWrapsAndAccumulatesAcrossSkippedNetworkSamples() {
        var look = new MouseLook();
        assertEquals(0, look.sample(179, 0, false, 160).totalYaw());
        assertEquals(2, look.sample(-179, 0, false, 160).totalYaw());
        look.sample(-169, 0, false, 160);
        assertEquals(22, look.sample(-159, 0, false, 160).totalYaw());
        assertEquals(85, look.sample(-159, 90, true, 160).pitch());
        look.reset();
        assertEquals(0, look.sample(40, 0, false, 160).totalYaw());
    }
    @Test void rejectsNonFiniteAnglesAndInvalidEyeHeight() {
        var look = new MouseLook();
        assertThrows(IllegalArgumentException.class, () -> look.sample(Double.NaN,0,false,160));
        assertThrows(IllegalArgumentException.class, () -> look.sample(0,Double.POSITIVE_INFINITY,false,160));
        assertThrows(IllegalArgumentException.class, () -> new MouseLook.Input(false,true,0,0,160));
        assertThrows(IllegalArgumentException.class, () -> new MouseLook.Input(true,true,0,0,0));
        assertEquals(1600, look.sample(0, 0, true, 1600).eyeHeight());
        assertEquals(-2000, look.sample(0, 0, true, -2000).eyeHeight());
        assertThrows(IllegalArgumentException.class, () -> look.sample(0,0,true,2501));
        assertThrows(IllegalArgumentException.class, () -> look.sample(0,0,true,-2501));
    }
}
