package dev.dezva.re4bridge;

import com.google.gson.Gson;
import java.net.ServerSocket;
import java.nio.file.Files;
import java.nio.file.Path;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;
import static org.junit.jupiter.api.Assertions.*;

class ControlTests {
    @TempDir Path temp;
    @Test void controlStartsDisarmedTransmitsOnlyFreshInputAndDisarmsAfterSamplingStops() throws Exception {
        int port;
        try (var socket = new ServerSocket(0)) { port = socket.getLocalPort(); }
        var config = new BridgeConfig(1, port, "a".repeat(64));
        var file = temp.resolve("config.json"); var log = temp.resolve("peer.log");
        Files.writeString(file, new Gson().toJson(config));
        var peer = new ProcessBuilder(System.getenv().getOrDefault("RE4BRIDGE_PYTHON", "python3"),
                Path.of("../tools/fixtures/control_peer.py").toAbsolutePath().toString(), "--config",file.toString())
                .redirectErrorStream(true).redirectOutput(log.toFile()).start();
        try (var bridge = new BridgeConnection(config, BridgeConnection.Mode.CONTROL)) {
            await(() -> { try { return Files.readString(log).contains("READY"); } catch (Exception e) { return false; } });
            bridge.start();
            await(() -> bridge.nativeTicks() == 123);
            assertFalse(bridge.nativeArmed());
            bridge.publishControl(true, 5, 1);
            await(bridge::nativeArmed);
            await(() -> bridge.nativeButtons() == 5);
            assertEquals("activo", bridge.nativeReason());
            assertTrue(Files.readString(log).contains("INPUT armed=True buttons=5"));
            // The network worker remains running; absence of new client ticks must still stop input.
            await(() -> !bridge.nativeArmed());
            assertTrue(bridge.status().connected());
            bridge.publishControl(true, 2, 2);
            await(bridge::nativeArmed);
            bridge.publishControl(false, 0, 2);
            await(() -> !bridge.nativeArmed());
            bridge.close();
            assertTrue(peer.waitFor(5, java.util.concurrent.TimeUnit.SECONDS));
            assertEquals(0, peer.exitValue(), Files.readString(log));
            assertFalse(Files.readString(log).contains(config.token()));
        } finally { peer.destroyForcibly(); }
    }
    private static void await(java.util.function.BooleanSupplier condition) throws Exception {
        long end=System.nanoTime()+5_000_000_000L;
        while (!condition.getAsBoolean() && System.nanoTime()<end) Thread.sleep(10);
        assertTrue(condition.getAsBoolean(), "Timed out");
    }
}
