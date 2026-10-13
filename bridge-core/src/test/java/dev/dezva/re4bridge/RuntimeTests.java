package dev.dezva.re4bridge;
import com.google.gson.JsonParser;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
class RuntimeTests {
    @Test void movieTelemetryExpiresAndDoesNotInventAPlayer() throws Exception {
        var json=JsonParser.parseString("{\"available\":true,\"fresh\":true,\"scene\":1,\"flags\":268435456,\"action_available\":true,\"enemies_available\":true,\"enemy_contacts\":3,\"enemy_corrections\":2,\"actions\":1}").getAsJsonObject();
        var state=RuntimeState.parse(json,1000);assertTrue(state.scripted(1001));assertFalse(state.scripted(500001000));assertEquals(2,state.corrections());
        json.addProperty("scene",3);assertThrows(java.io.IOException.class,()->RuntimeState.parse(json,1000));
        assertFalse(RuntimeState.parse(JsonParser.parseString("{\"available\":false,\"fresh\":false}").getAsJsonObject(),1000).scripted(1001));
    }
}
