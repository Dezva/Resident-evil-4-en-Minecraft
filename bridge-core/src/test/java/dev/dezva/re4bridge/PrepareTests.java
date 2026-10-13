package dev.dezva.re4bridge;
import com.google.gson.JsonObject;
import org.junit.jupiter.api.Test;
import java.net.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicReference;
import static org.junit.jupiter.api.Assertions.*;
class PrepareTests {
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
