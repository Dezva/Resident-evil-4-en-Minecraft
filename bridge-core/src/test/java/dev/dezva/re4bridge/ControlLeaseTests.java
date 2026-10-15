package dev.dezva.re4bridge;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
import static dev.dezva.re4bridge.ControlLease.Phase.*;
class ControlLeaseTests {
    @Test void movementEndsAt200msAndOnlyAnExistingSessionCanWaitUpToOneSecond(){
        assertEquals(LIVE,ControlLease.phase(199_999_999L,0,true,false));
        assertEquals(SUSPENDED,ControlLease.phase(200_000_000L,0,true,false));
        assertEquals(SUSPENDED,ControlLease.phase(999_999_999L,0,true,false));
        assertEquals(EXPIRED,ControlLease.phase(1_000_000_000L,0,true,false));
        assertEquals(EXPIRED,ControlLease.phase(200_000_000L,0,false,false));
        assertEquals(EXPIRED,ControlLease.phase(0,1,true,false));
        assertEquals(LIVE,ControlLease.phase(999_999_999L,0,true,true));
        assertEquals(EXPIRED,ControlLease.phase(1_000_000_000L,0,true,true));
    }
}
