package dev.dezva.re4bridge;
import com.google.gson.JsonObject;
import org.junit.jupiter.api.Test;
import java.net.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicReference;
import static org.junit.jupiter.api.Assertions.*;
class PrepareTests {
    @Test void preparationModesAreAuthenticatedBoundedAndAcknowledged() throws Exception {
        var config=config();var selected=new AtomicReference<PrepareServer.Mode>();
        try(var server=new PrepareServer(config,(mode,future)->{selected.set(mode);future.complete("Listo");})){
            for(var mode:PrepareServer.Mode.values()){
                var request=request(config.token());request.addProperty("game_mode",mode.wire());
                try(var peer=new Socket("127.0.0.1",PrepareServer.port(config))){peer.setSoTimeout(2000);peer.getOutputStream().write(LineProtocol.encode(request));var reply=LineProtocol.read(peer.getInputStream());assertTrue(reply.get("ok").getAsBoolean());assertEquals(mode.wire(),reply.get("game_mode").getAsString());assertEquals(mode,selected.get());}
            }
            selected.set(null);var invalid=request(config.token());invalid.addProperty("game_mode","spectator; give items");
            try(var peer=new Socket("127.0.0.1",PrepareServer.port(config))){peer.setSoTimeout(2000);peer.getOutputStream().write(LineProtocol.encode(invalid));assertFalse(LineProtocol.read(peer.getInputStream()).get("ok").getAsBoolean());}
            assertNull(selected.get());
        }
    }
    BridgeConfig config() throws Exception {try(var s=new ServerSocket(0,1,InetAddress.getLoopbackAddress())){return new BridgeConfig(1,s.getLocalPort()-1,"a".repeat(64));}}
    JsonObject request(String token){var j=new JsonObject();j.addProperty("protocol",1);j.addProperty("type","prepare");j.addProperty("token",token);return j;}
    @Test void authenticatedPrepareAndInvalidToken() throws Exception {
        var config=config();var calls=new java.util.concurrent.atomic.AtomicInteger();
        try(var server=new PrepareServer(config,f->{calls.incrementAndGet();f.complete("Listo: F9");})){
            try(var peer=new Socket("127.0.0.1",PrepareServer.port(config))){peer.setSoTimeout(2000);peer.getOutputStream().write(LineProtocol.encode(request("b".repeat(64))));assertFalse(LineProtocol.read(peer.getInputStream()).get("ok").getAsBoolean());}
            assertEquals(0,calls.get());
            try(var peer=new Socket("127.0.0.1",PrepareServer.port(config))){peer.setSoTimeout(2000);peer.getOutputStream().write(LineProtocol.encode(request(config.token())));assertEquals("Listo: F9",LineProtocol.read(peer.getInputStream()).get("message").getAsString());}
            assertEquals(1,calls.get());
        }
    }
    @Test void closingViewCancelsPendingPreparation() throws Exception {
        var config=config();var called=new CountDownLatch(1);var future=new AtomicReference<CompletableFuture<String>>();
        try(var server=new PrepareServer(config,f->{future.set(f);called.countDown();})){
            var peer=new Socket("127.0.0.1",PrepareServer.port(config));peer.getOutputStream().write(LineProtocol.encode(request(config.token())));
            assertTrue(called.await(2,TimeUnit.SECONDS));peer.close();
            assertThrows(CancellationException.class,()->future.get().get(2,TimeUnit.SECONDS));
        }
    }
}
