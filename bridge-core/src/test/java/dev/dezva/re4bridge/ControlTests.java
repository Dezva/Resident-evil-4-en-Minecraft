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
            await(bridge::previewAvailable);
            await(bridge::sceneAvailable);
            await(bridge::surfaceAvailable);
            await(bridge::textureAvailable);
            assertFalse(bridge.nativeArmed());
            bridge.publishControl(false,0,0,null,new BlockPreview(1,2,3003,1000));
            await(() -> bridge.status().acknowledgedSequence()>=2);
            assertFalse(bridge.previewRequested());
            assertFalse(Files.readString(log).contains("PREVIEW enabled=True"));
            bridge.publishControl(true, 5, 1);
            await(bridge::nativeArmed);
            await(() -> bridge.nativeButtons() == 5);
            assertEquals("activo", bridge.nativeReason());
            assertEquals(7, bridge.nativeCapabilities());
            assertEquals(10, bridge.nativeLookPackets());
            bridge.publishControl(true, 5, 1, new MouseLook.Input(true, false, 20, -15, 160));
            await(() -> { try { return Files.readString(log).contains("'total_yaw': 20.0"); } catch (Exception e) { return false; } });
            assertTrue(Files.readString(log).contains("INPUT armed=True buttons=5"));
            assertThrows(IllegalArgumentException.class, () -> bridge.publishControl(true, 16, 1));
            assertThrows(IllegalArgumentException.class, () -> bridge.publishControl(true, 32, 1));
            bridge.publishControl(true, 69, 1);
            await(() -> bridge.nativeButtons() == 69);
            bridge.publishControl(true, 5, 1);
            await(() -> bridge.nativeButtons() == 5); // Releasing sprint keeps ordinary movement.
            bridge.publishControl(true, 5, 1, null, new BlockPreview(1,2,3003,1000));
            await(() -> { try { return Files.readString(log).contains("PREVIEW enabled=True"); } catch (Exception e) { return false; } });
            await(bridge::previewRequested);
            assertEquals(12,bridge.previewSkipped());assertEquals("pase omitido",bridge.previewReason());
            assertEquals(1,bridge.previewStatus());assertEquals(99,bridge.previewFrames());assertEquals(20,bridge.previewDraws());
            var scene=new BlockScene(10,20,30,0,java.util.List.of(
                new BlockScene.Bounds(0,0,-1000,1000,1000,0),new BlockScene.Bounds(1000,0,-1000,2000,500,0)),1);
            bridge.publishSceneControl(true,5,1,null,scene);
            await(() -> bridge.previewBoxCount()==2 && bridge.previewStyle()==1);
            bridge.publishSceneControl(true,5,1,null,new BlockScene(scene.x(),scene.y(),scene.z(),scene.yaw(),scene.boxes(),0));
            await(() -> bridge.previewStyle()==0);
            bridge.publishSceneControl(true,5,1,null,new BlockScene(scene.x(),scene.y(),scene.z(),scene.yaw(),scene.boxes(),2));
            await(() -> bridge.previewStyle()==2);
            assertTrue(Files.readString(log).contains("'origin': [10.0, 20.0, 30.0, 0.0]"));
            bridge.publishControl(true,5,1);
            // The network worker remains running; absence of new client ticks must still stop input.
            await(() -> !bridge.nativeArmed());
            await(() -> !bridge.previewRequested());
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
