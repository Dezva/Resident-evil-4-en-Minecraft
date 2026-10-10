package dev.dezva.re4bridge;

import com.google.gson.Gson;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.net.ServerSocket;
import java.nio.file.Files;
import java.nio.file.Path;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;
import static org.junit.jupiter.api.Assertions.*;

class Re4Tests {
    @TempDir Path temp;

    @Test void samplesRejectNonNumericInvalidAndNonFiniteDataAndExpireLocally() throws Exception {
        var good = JsonParser.parseString("{\"state\":\"player\",\"position_raw\":[1,2,3],\"angles_raw\":[0,1,0]}").getAsJsonObject();
        var sample = Re4Sample.parse(good, 100);
        assertTrue(sample.fresh(101));
        assertFalse(sample.fresh(99));
        assertFalse(sample.fresh(1_000_000_100L));
        assertNull(Re4Sample.parse(JsonParser.parseString("{\"state\":\"unavailable\"}").getAsJsonObject(), 100));
        for (String bad : new String[]{"{\"state\":\"unknown\"}", "{\"state\":\"player\",\"position_raw\":[1,2],\"angles_raw\":[0,0,0]}",
                "{\"state\":\"player\",\"position_raw\":[\"1\",2,3],\"angles_raw\":[0,0,0]}",
                "{\"state\":\"player\",\"position_raw\":[1e309,2,3],\"angles_raw\":[0,0,0]}"}) {
            assertThrows(IOException.class, () -> Re4Sample.parse(JsonParser.parseString(bad).getAsJsonObject(), 100));
        }
    }

    @Test void mappingAnchorsPositionScalesFlipsElevationAndWrapsYaw() {
        var origin = new Re4Sample(-10000, -200, 300, 3, 100);
        var calibration = new Re4Calibration(origin, 10, 64, 20, 175, 100, -1, 1);
        var start = calibration.map(origin);
        assertEquals(10, start.x()); assertEquals(64, start.y()); assertEquals(175, start.yaw());
        var target = calibration.map(new Re4Sample(-9800, -300, 200, 3 + Math.PI / 2, 200));
        assertEquals(12, target.x()); assertEquals(65, target.y()); assertEquals(19, target.z());
        assertEquals(-95, target.yaw(), 0.001);
        assertThrows(IllegalArgumentException.class, () -> new Re4Calibration(origin, 0, 0, 0, 0, 0, 1, 1));
        assertThrows(IllegalArgumentException.class, () -> calibration.map(new Re4Sample(4e9, 0, 0, 0, 100)));
    }

    @Test void javaPollsPythonReaderModeAndClearsPoseWhenPlayerDisappears() throws Exception {
        var token = "a".repeat(64);
        int port;
        try (var available = new ServerSocket(0)) { port = available.getLocalPort(); }
        var config = new BridgeConfig(1, port, token);
        Path file = temp.resolve("config.json"), log = temp.resolve("host.log");
        Files.writeString(file, new Gson().toJson(config));
        Process host = new ProcessBuilder(System.getenv().getOrDefault("RE4BRIDGE_PYTHON", "python3"),
                Path.of("../tools/fixtures/re4_peer.py").toAbsolutePath().toString(), "--config", file.toString())
                .redirectErrorStream(true).redirectOutput(log.toFile()).start();
        try (var bridge = new BridgeConnection(config, true)) {
            await(() -> { try { return Files.readString(log).contains("READY"); } catch (Exception e) { return false; } });
            bridge.start();
            await(() -> bridge.re4Sample() != null);
            assertTrue(bridge.status().connected());
            assertEquals(100, bridge.re4Sample().x());
            assertEquals(1.5, bridge.re4Sample().yawRadians());
            await(() -> bridge.status().acknowledgedSequence() >= 3);
            assertNull(bridge.re4Sample());
            assertTrue(bridge.status().connected());
            bridge.close();
            assertTrue(host.waitFor(5, java.util.concurrent.TimeUnit.SECONDS));
            assertEquals(0, host.exitValue(), Files.readString(log));
            assertFalse(Files.readString(log).contains(token));
        } finally { host.destroyForcibly(); }
    }

    @Test void readerCommandRejectsSimulatorWelcome() throws Exception {
        try (var server = new ServerSocket(0)) {
            var peer = new Thread(() -> {
                try (var socket = server.accept()) {
                    var hello = LineProtocol.read(socket.getInputStream());
                    hello.remove("token");
                    hello.addProperty("type", "welcome"); hello.addProperty("mode", "simulator");
                    socket.getOutputStream().write(LineProtocol.encode(hello));
                } catch (IOException e) { throw new RuntimeException(e); }
            });
            peer.start();
            try (var bridge = new BridgeConnection(new BridgeConfig(1, server.getLocalPort(), "a".repeat(64)), true)) {
                bridge.start();
                await(() -> bridge.status().message().startsWith("connection failed"));
                assertFalse(bridge.status().connected()); assertNull(bridge.re4Sample());
            }
            peer.join(3000);
            assertFalse(peer.isAlive());
        }
    }

    private static void await(java.util.function.BooleanSupplier test) throws Exception {
        long deadline = System.nanoTime() + 5_000_000_000L;
        while (!test.getAsBoolean() && System.nanoTime() < deadline) Thread.sleep(10);
        assertTrue(test.getAsBoolean(), "Timed out waiting for peer");
    }
}
