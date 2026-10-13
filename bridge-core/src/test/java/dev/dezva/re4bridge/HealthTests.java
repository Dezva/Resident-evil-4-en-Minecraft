package dev.dezva.re4bridge;
import com.google.gson.JsonParser;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
class HealthTests {
    private static HealthState sample(String replacement) throws Exception {
        return HealthState.parse(JsonParser.parseString(("{\"available\":true,\"active\":true,\"fresh\":true,\"reason\":0,\"generation\":7,\"player\":4096,\"hp\":800,\"maximum\":1000,\"damage\":200000,\"healing\":0,\"dead\":false,\"mode\":1}").replace("\"damage\":200000",replacement)).getAsJsonObject());
    }
    @Test void cumulativeCountersAreConsumedOnceAndSurviveWrap() throws Exception {
        var state=sample("\"damage\":200000");assertEquals(800,state.hp());assertEquals(200000,HealthState.delta(state.damage(),0));
        assertEquals(0,HealthState.delta(state.damage(),200000));assertEquals(100000,HealthState.delta(99989,0xffffffffL-10));
        assertThrows(IllegalArgumentException.class,()->HealthState.delta(1,2));
    }
    @Test void healthProtocolRejectsTornOrInvalidValuesAndBoundsCommands() throws Exception {
        assertThrows(java.io.IOException.class,()->sample("\"damage\":-1"));
        assertThrows(java.io.IOException.class,()->sample("\"damage\":true"));
        assertThrows(java.io.IOException.class,()->sample("\"damage\":0.5"));
        assertThrows(java.io.IOException.class,()->sample("\"damage\":4294967296"));
        assertFalse(HealthState.parse(JsonParser.parseString("{\"available\":false}").getAsJsonObject()).available());
        var request=new HealthRequest(7,1,.8f,200000,0,123);assertEquals(200000,request.toJson().get("damage_ack").getAsLong());
        assertThrows(IllegalArgumentException.class,()->new HealthRequest(7,1,Float.NaN,0,0,0));
        assertThrows(IllegalArgumentException.class,()->new HealthRequest(0,1,1,0,0,0));
    }
}
