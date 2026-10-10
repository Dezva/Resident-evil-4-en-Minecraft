package dev.dezva.re4bridge;

import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.ByteArrayInputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.time.Duration;
import java.net.ServerSocket;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.BooleanSupplier;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;
import static org.junit.jupiter.api.Assertions.*;

class BridgeTests {
    @TempDir Path temp;

    @Test void configIsReusedAndInvalidUserFileIsPreserved() throws Exception {
        Path file = temp.resolve("config/re4bridge.json");
        BridgeConfig first = BridgeConfig.loadOrCreate(file);
        assertEquals(64, first.token().length());
        assertEquals(first, BridgeConfig.loadOrCreate(file));
        Files.writeString(file, "{\"protocol\":2}");
        assertThrows(java.io.IOException.class, () -> BridgeConfig.loadOrCreate(file));
        assertEquals("{\"protocol\":2}", Files.readString(file));
    }

    @Test void frameLimitsVersionsAndEncodingAreEnforced() throws Exception {
        for (String bad : new String[]{"{\"protocol\":true}\n", "{\"protocol\":2}\n", "[]\n", "{\"protocol\":1}",
                "{protocol:1}\n", "{\"protocol\":1} {}\n", "{\"protocol\":1,\"x\":NaN}\n"}) {
            assertThrows(java.io.IOException.class, () -> LineProtocol.read(new ByteArrayInputStream(bad.getBytes(StandardCharsets.UTF_8))));
        }
        assertThrows(java.io.IOException.class, () -> LineProtocol.read(new ByteArrayInputStream(new byte[]{(byte)0xff, '\n'})));
        byte[] oversized = new byte[LineProtocol.MAX_FRAME_BYTES + 1];
        assertThrows(java.io.IOException.class, () -> LineProtocol.read(new ByteArrayInputStream(oversized)));
        JsonObject hello = JsonParser.parseString("{\"protocol\":1,\"type\":\"hello\"}").getAsJsonObject();
        assertEquals(hello, LineProtocol.read(new ByteArrayInputStream(LineProtocol.encode(hello))));
    }

    @Test void javaClientExchangesPoseCollisionAndHeartbeatWithPythonSimulator() throws Exception {
        Path configPath = temp.resolve("re4bridge.json");
        BridgeConfig generated = BridgeConfig.loadOrCreate(configPath);
        int port;
        try (ServerSocket available = new ServerSocket(0, 1, java.net.InetAddress.getLoopbackAddress())) {
            port = available.getLocalPort();
        }
        BridgeConfig config = new BridgeConfig(1, port, generated.token());
        Files.writeString(configPath, new com.google.gson.Gson().toJson(config));
        Path report = temp.resolve("report.json");
        Path log = temp.resolve("host.log");
        String python = System.getenv().getOrDefault("RE4BRIDGE_PYTHON", "python3");
        Process host = new ProcessBuilder(python, Path.of("../tools/fake_re4_host.py").toAbsolutePath().toString(),
                "--config", configPath.toString(), "--once", "--report", report.toString())
                .redirectErrorStream(true).redirectOutput(log.toFile()).start();
        try (BridgeConnection bridge = new BridgeConnection(config)) {
            await(() -> { try { return Files.readString(log).contains("READY"); } catch (Exception e) { return false; } });
            bridge.start();
            await(() -> bridge.status().connected());
            bridge.publishPose(json("{\"type\":\"pose\",\"sample_ns\":1,\"dimension\":\"minecraft:overworld\","
                    + "\"feet\":[1,64,2],\"eye\":[1,65.62,2],\"yaw_deg\":90,\"pitch_deg\":0}"));
            bridge.publishCollision(json("{\"type\":\"collision_snapshot\",\"sample_ns\":2,\"dimension\":\"minecraft:overworld\","
                    + "\"center\":[1,64,2],\"complete\":true,\"blocks\":[{\"pos\":[1,63,2],"
                    + "\"state\":\"minecraft:stone_slab[type=bottom]\",\"collision_complete\":true,\"boxes\":[[0,0,0,1,0.5,1]]}]}"));
            await(() -> bridge.status().acknowledgedSequence() >= 2);
            // Heartbeat must keep a paused local world alive without fake pose data.
            await(() -> bridge.status().acknowledgedSequence() >= 3);
            bridge.close();
            assertTrue(host.waitFor(5, java.util.concurrent.TimeUnit.SECONDS));
            assertEquals(0, host.exitValue(), Files.readString(log));
            JsonObject result = json(Files.readString(report));
            assertEquals(1, result.get("poses").getAsInt());
            assertEquals(1, result.get("snapshots").getAsInt());
            assertFalse(result.has("error"), result.toString());
            assertTrue(result.get("disconnected").getAsBoolean());
            assertEquals(0.5, result.getAsJsonObject("last_collision").getAsJsonArray("blocks").get(0)
                    .getAsJsonObject().getAsJsonArray("boxes").get(0).getAsJsonArray().get(4).getAsDouble());
            assertFalse(Files.readString(log).contains(config.token()));
            assertFalse(Files.readString(report).contains(config.token()));
        } finally {
            host.destroy();
            if (!host.waitFor(2, java.util.concurrent.TimeUnit.SECONDS)) host.destroyForcibly();
        }
    }

    @Test void wrongSessionIsRejectedAndConnectionCanBeClosed() throws Exception {
        AtomicReference<Throwable> serverFailure = new AtomicReference<>();
        try (ServerSocket server = new ServerSocket(0, 1, java.net.InetAddress.getLoopbackAddress())) {
            Thread peer = new Thread(() -> {
                try (var socket = server.accept()) {
                    socket.setSoTimeout(2000);
                    LineProtocol.read(socket.getInputStream());
                    socket.getOutputStream().write(LineProtocol.encode(json("{\"protocol\":1,\"type\":\"welcome\",\"session\":\"wrong\",\"mode\":\"simulator\"}")));
                } catch (Throwable t) { serverFailure.set(t); }
            });
            peer.start();
            try (BridgeConnection bridge = new BridgeConnection(new BridgeConfig(1, server.getLocalPort(), "a".repeat(64)))) {
                bridge.start();
                await(() -> bridge.status().message().startsWith("connection failed"));
                assertFalse(bridge.status().connected());
            }
            peer.join(3000);
            assertFalse(peer.isAlive());
            assertNull(serverFailure.get());
        }
    }

    private static JsonObject json(String text) { return JsonParser.parseString(text).getAsJsonObject(); }
    private static void await(BooleanSupplier condition) throws Exception {
        long deadline = System.nanoTime() + Duration.ofSeconds(5).toNanos();
        while (!condition.getAsBoolean() && System.nanoTime() < deadline) Thread.sleep(20);
        assertTrue(condition.getAsBoolean(), "Timed out waiting for the peer");
    }
}
