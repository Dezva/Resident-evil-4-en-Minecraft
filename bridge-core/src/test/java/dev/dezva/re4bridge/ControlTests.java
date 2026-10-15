package dev.dezva.re4bridge;

import com.google.gson.Gson;
import java.net.ServerSocket;
import java.nio.file.Files;
import java.nio.file.Path;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;
import static org.junit.jupiter.api.Assertions.*;

class ControlTests {
    @Test void viewStopCodeRemainsConnectedAndDisarmedInsteadOfFailingProtocol() throws Exception {
        int port;try(var socket=new ServerSocket(0)){port=socket.getLocalPort();}
        var config=new BridgeConfig(1,port,"a".repeat(64));var file=temp.resolve("stop-config.json");var log=temp.resolve("stop-peer.log");
        Files.writeString(file,new Gson().toJson(config));
        var peer=new ProcessBuilder(System.getenv().getOrDefault("RE4BRIDGE_PYTHON","python3"),Path.of("../tools/fixtures/control_peer.py").toAbsolutePath().toString(),"--config",file.toString(),"--native-stop","8")
            .redirectErrorStream(true).redirectOutput(log.toFile()).start();
        try(var bridge=new BridgeConnection(config,BridgeConnection.Mode.CONTROL)){
            await(()->{try{return Files.readString(log).contains("READY");}catch(Exception e){return false;}});
            bridge.start();await(()->bridge.nativeStop()==8);
            assertTrue(bridge.status().connected());assertFalse(bridge.nativeArmed());assertEquals(0,bridge.nativeButtons());
            long seq=bridge.status().acknowledgedSequence();await(()->bridge.status().acknowledgedSequence()>seq);
            assertTrue(bridge.status().connected());assertEquals(8,bridge.nativeStop());
        }finally{peer.destroyForcibly();}
    }
    @Test void movingToEscImmediatelyZeroesInputWhileKeepingGenerationThroughGuiInitialization() throws Exception {
        int port;try(var socket=new ServerSocket(0)){port=socket.getLocalPort();}
        var config=new BridgeConfig(1,port,"a".repeat(64));var file=temp.resolve("config.json");var log=temp.resolve("pause-peer.log");Files.writeString(file,new Gson().toJson(config));
        var peer=new ProcessBuilder(System.getenv().getOrDefault("RE4BRIDGE_PYTHON","python3"),Path.of("../tools/fixtures/control_peer.py").toAbsolutePath().toString(),"--config",file.toString()).redirectErrorStream(true).redirectOutput(log.toFile()).start();
        try(var bridge=new BridgeConnection(config,BridgeConnection.Mode.CONTROL)){
            await(()->{try{return Files.readString(log).contains("READY");}catch(Exception e){return false;}});bridge.start();await(()->bridge.nativeTicks()==123);
            bridge.publishControl(true,65,7,new MouseLook.Input(true,true,0,0,1620));await(()->bridge.nativeButtons()==65);
            bridge.vertical(7,1,420,1000,-80,false);await(()->{try{return Files.readString(log).contains("VERTICAL generation=7 token=1 lift=420.0 feet_y=-80.0 native_ground=False");}catch(Exception e){return false;}});
            bridge.publishControl(true,65,7,new MouseLook.Input(true,true,0,0,1620));
            bridge.vertical(7,2,0,800,315.845642,true);await(()->{try{return Files.readString(log).contains("VERTICAL generation=7 token=2 lift=0.0 feet_y=315.845642 native_ground=True");}catch(Exception e){return false;}});
            bridge.publishControl(true,65,7,new MouseLook.Input(true,true,0,0,1620));
            bridge.combatRequested(true,1000);bridge.combatRequest(new CombatRequest(7,1,0,10,50,1000,3,new TerrainSnapshot.Point(0,1000,0),new TerrainSnapshot.Point(3000,1000,0),System.nanoTime()));
            await(()->bridge.combatState()!=null && bridge.combatState().acknowledgement()==1);
            assertEquals(1,bridge.combatState().hits());assertTrue(Files.readString(log).contains("MELEE generation=7 id=1 damage=50 slot=0"));bridge.combatRequest(null);
            bridge.publishControl(true,65,7,new MouseLook.Input(true,true,0,0,1620));
            bridge.requestAction(7);await(()->bridge.runtimeState()!=null && bridge.runtimeState().actions()==1);
            assertTrue(Files.readString(log).contains("ACTION generation=7 id=1 kind=0"));
            bridge.publishControl(true,65,7,new MouseLook.Input(true,true,0,0,1620));
            bridge.requestSkip(7);await(()->bridge.runtimeState().skips()==1);assertTrue(Files.readString(log).contains("ACTION generation=7 id=2 kind=1"));
            bridge.suspendForMenu();await(()->bridge.nativeArmed() && bridge.nativeButtons()==0);
            long seq=bridge.status().acknowledgedSequence();Thread.sleep(350);assertTrue(bridge.nativeArmed());assertEquals(0,bridge.nativeButtons());assertTrue(bridge.status().acknowledgedSequence()>seq);
            bridge.healthRequest(new HealthRequest(7,1,.8f,200000,0,System.nanoTime()));
            await(()->{try{return Files.readString(log).contains("damage_ack=200000 generation=7");}catch(Exception e){return false;}});
            await(()->bridge.healthState()!=null && bridge.healthState().generation()==7);assertEquals(200000,bridge.healthState().damage());
            bridge.menuRequested(false);bridge.publishControl(true,1,7,new MouseLook.Input(true,true,0,0,1620));await(()->bridge.nativeButtons()==1);assertTrue(bridge.nativeArmed());
            assertEquals(1,bridge.runtimeState().actions()); // no replay of the action cancelled on menu entry
            bridge.requestAction(8);bridge.publishControl(true,1,7,new MouseLook.Input(true,true,0,0,1620));
            long acknowledged=bridge.status().acknowledgedSequence();await(()->bridge.status().acknowledgedSequence()>acknowledged);
            assertFalse(Files.readString(log).contains("ACTION generation=8")); // action from another generation cannot cross the wire
            // Stalled input is zeroed at200ms; a full second still ends the native session.
            await(()->!bridge.nativeArmed());assertTrue(bridge.status().connected());
        }finally{peer.destroyForcibly();}
    }
    @Test void shortTickStallStopsInputAt200msAndLongStallStillRequiresANewArm() throws Exception {
        int port;try(var socket=new ServerSocket(0)){port=socket.getLocalPort();}
        var config=new BridgeConfig(1,port,"a".repeat(64));var file=temp.resolve("stall-config.json");var log=temp.resolve("stall-peer.log");
        Files.writeString(file,new Gson().toJson(config));
        var peer=new ProcessBuilder(System.getenv().getOrDefault("RE4BRIDGE_PYTHON","python3"),Path.of("../tools/fixtures/control_peer.py").toAbsolutePath().toString(),"--config",file.toString())
            .redirectErrorStream(true).redirectOutput(log.toFile()).start();
        try(var bridge=new BridgeConnection(config,BridgeConnection.Mode.CONTROL)){
            await(()->{try{return Files.readString(log).contains("READY");}catch(Exception e){return false;}});bridge.start();await(()->bridge.nativeTicks()==123);
            var look=new MouseLook.Input(true,true,0,0,1620);
            var scene=new BlockScene(10,20,30,0,java.util.List.of(new BlockScene.Bounds(0,0,0,1000,1000,1000)),0);
            bridge.clientState("activo",true,false,"NONE");bridge.publishSceneControl(true,1,7,look,scene);await(()->bridge.nativeArmed() && bridge.nativeButtons()==1);
            await(()->bridge.nativeButtons()==0);assertTrue(bridge.nativeArmed());assertTrue(bridge.previewRequested());
            assertTrue(Files.readString(log).contains("sin tick de Minecraft; entrada suspendida"));
            assertTrue(Files.readString(log).contains("FROZEN generation=7 fp=True"));
            bridge.publishSceneControl(true,1,7,look,scene);await(()->bridge.nativeButtons()==1);assertTrue(bridge.nativeArmed());
            await(()->!bridge.nativeArmed());assertEquals(0,bridge.nativeButtons());
            long sequence=bridge.status().acknowledgedSequence();bridge.publishSceneControl(true,1,7,look,scene);await(()->bridge.status().acknowledgedSequence()>sequence);assertFalse(bridge.nativeArmed());
            bridge.publishSceneControl(true,1,8,look,scene);await(bridge::nativeArmed);
        }finally{peer.destroyForcibly();}
    }
    @Test void focusSuspensionKeepsSceneAndGenerationWithZeroButtonsAndReportsItsReason() throws Exception {
        int port;try(var socket=new ServerSocket(0)){port=socket.getLocalPort();}
        var config=new BridgeConfig(1,port,"a".repeat(64));var file=temp.resolve("focus-config.json");var log=temp.resolve("focus-peer.log");
        Files.writeString(file,new Gson().toJson(config));
        var peer=new ProcessBuilder(System.getenv().getOrDefault("RE4BRIDGE_PYTHON","python3"),Path.of("../tools/fixtures/control_peer.py").toAbsolutePath().toString(),"--config",file.toString())
            .redirectErrorStream(true).redirectOutput(log.toFile()).start();
        try(var bridge=new BridgeConnection(config,BridgeConnection.Mode.CONTROL)){
            await(()->{try{return Files.readString(log).contains("READY");}catch(Exception e){return false;}});bridge.start();await(()->bridge.nativeTicks()==123);
            var look=new MouseLook.Input(true,true,0,0,1620);
            var scene=new BlockScene(10,20,30,0,java.util.List.of(new BlockScene.Bounds(0,0,0,1000,1000,1000)),0);
            bridge.publishSceneControl(true,1,7,look,scene);await(()->bridge.nativeArmed() && bridge.nativeButtons()==1 && bridge.previewRequested());
            bridge.clientState("foco transitorio; entrada suspendida",false,false,"NONE");bridge.suspendForMenu();bridge.clearCombat();
            await(()->bridge.nativeArmed() && bridge.nativeButtons()==0);Thread.sleep(75);assertTrue(bridge.previewRequested());
            assertTrue(Files.readString(log).contains("foco transitorio; entrada suspendida"));assertTrue(Files.readString(log).contains("'focused': False"));
            bridge.menuRequested(false);bridge.clientState("activo",true,false,"NONE");bridge.publishSceneControl(true,1,7,look,scene);
            await(()->bridge.nativeButtons()==1);assertTrue(bridge.nativeArmed());
            assertThrows(IllegalArgumentException.class,()->bridge.clientState("bad\nline",true,false,"NONE"));
        }finally{peer.destroyForcibly();}
    }
    @Test void pickupDialogTransmitsArrowThenEnterAndClearsLaterInputWhenItCloses() throws Exception {
        int port;try(var socket=new ServerSocket(0)){port=socket.getLocalPort();}
        var config=new BridgeConfig(1,port,"a".repeat(64));var file=temp.resolve("dialog-config.json");var log=temp.resolve("dialog-peer.log");
        Files.writeString(file,new Gson().toJson(config));
        var peer=new ProcessBuilder(System.getenv().getOrDefault("RE4BRIDGE_PYTHON","python3"),Path.of("../tools/fixtures/control_peer.py").toAbsolutePath().toString(),"--config",file.toString(),"--dialog")
            .redirectErrorStream(true).redirectOutput(log.toFile()).start();
        try(var bridge=new BridgeConnection(config,BridgeConnection.Mode.CONTROL)){
            await(()->{try{return Files.readString(log).contains("READY");}catch(Exception e){return false;}});bridge.start();await(()->bridge.nativeTicks()==123);
            var look=new MouseLook.Input(true,true,0,0,1620);bridge.publishControl(true,0,7,look);await(bridge::nativeArmed);
            assertFalse(bridge.nativeDialog());bridge.requestAction(7);
            await(bridge::nativeDialog);bridge.requestDialog(7,5);bridge.requestDialog(7,6);bridge.requestDialog(7,4);
            long deadline=System.nanoTime()+2_000_000_000L;
            while(System.nanoTime()<deadline && bridge.runtimeState().dialogActions()<2){bridge.publishControl(true,0,7,look);Thread.sleep(10);}
            assertEquals(2,bridge.runtimeState().dialogActions());await(()->!bridge.nativeDialog());
            for(int i=0;i<25;i++){bridge.publishControl(true,0,7,look);Thread.sleep(10);}
            var text=Files.readString(log);int arrow=text.indexOf("id=2 kind=5"),enter=text.indexOf("id=3 kind=6");
            assertTrue(arrow>=0 && enter>arrow,text);assertFalse(text.contains("id=4 kind=4"));assertEquals(2,bridge.runtimeState().dialogActions());
        }finally{peer.destroyForcibly();}
    }
    @TempDir Path temp;
    private static HudFrame overlay(int width,boolean menu) throws Exception {
        var out=new java.io.ByteArrayOutputStream();
        try(var zip=new java.util.zip.DeflaterOutputStream(out)){zip.write(new byte[width*32*4]);}
        return HudFrame.overlay(java.util.Base64.getEncoder().encodeToString(out.toByteArray()),width,32,menu);
    }
    @Test void imageWorkerCanRefreshBetweenTicksButCannotKeepArmedAfterClientTicksStop() throws Exception {
        int port;try(var socket=new ServerSocket(0)){port=socket.getLocalPort();}
        var config=new BridgeConfig(1,port,"a".repeat(64));var file=temp.resolve("config.json");var log=temp.resolve("overlay-peer.log");
        Files.writeString(file,new Gson().toJson(config));
        var peer=new ProcessBuilder(System.getenv().getOrDefault("RE4BRIDGE_PYTHON","python3"),Path.of("../tools/fixtures/control_peer.py").toAbsolutePath().toString(),"--config",file.toString(),"--overlay")
            .redirectErrorStream(true).redirectOutput(log.toFile()).start();
        try(var bridge=new BridgeConnection(config,BridgeConnection.Mode.CONTROL)){
            await(()->{try{return Files.readString(log).contains("READY");}catch(Exception e){return false;}});
            var image=new java.util.concurrent.atomic.AtomicReference<>(overlay(32,false));bridge.overlaySource(image::get);
            bridge.start();await(()->bridge.nativeTicks()==123);
            var look=new MouseLook.Input(true,true,0,0,1620);
            bridge.publishSceneControl(true,1,1,look,null,image.get());
            await(bridge::nativeArmed);
            bridge.refreshLook(1,new MouseLook.Input(true,true,99,90,1620,true));
            await(()->{try{return Files.readString(log).contains("'total_yaw': 99.0");}catch(Exception e){return false;}});
            image.set(overlay(64,false)); // No new control tick here.
            await(()->{try{return Files.readString(log).contains("OVERLAY width=64 menu=False");}catch(Exception e){return false;}});
            image.set(overlay(96,true)); // A menu image cannot be mixed with gameplay buttons.
            Thread.sleep(25);assertFalse(Files.readString(log).contains("OVERLAY width=96 menu=True"));
            bridge.publishSceneControl(true,0,1,look,null,image.get());
            await(()->{try{return Files.readString(log).contains("OVERLAY width=96 menu=True");}catch(Exception e){return false;}});
            image.set(overlay(128,true));
            for(int i=0;i<60;i++){bridge.refreshLook(1,look);Thread.sleep(5);}
            await(()->!bridge.nativeArmed());assertTrue(bridge.status().connected());
        }finally{peer.destroyForcibly();}
    }
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
            await(bridge::materialsAvailable);await(bridge::collisionAvailable);await(bridge::hudAvailable);
            assertFalse(bridge.nativeArmed());
            bridge.publishControl(false,0,0,null,new BlockPreview(1,2,3003,1000));
            await(() -> bridge.status().acknowledgedSequence()>=2);
            assertFalse(bridge.previewRequested());
            assertFalse(Files.readString(log).contains("PREVIEW enabled=True"));
            bridge.publishControl(true, 5, 1);
            await(bridge::nativeArmed);
            await(() -> bridge.nativeButtons() == 5);
            assertEquals("activo", bridge.nativeReason());
            assertEquals(31, bridge.nativeCapabilities());
            assertEquals(10, bridge.nativeLookPackets());
            bridge.publishControl(true, 5, 1, new MouseLook.Input(true, false, 20, -15, 160));
            await(() -> { try { return Files.readString(log).contains("'total_yaw': 20.0"); } catch (Exception e) { return false; } });
            assertTrue(Files.readString(log).contains("INPUT armed=True buttons=5"));
            assertThrows(IllegalArgumentException.class, () -> bridge.publishControl(true, 16, 1));
            assertThrows(IllegalArgumentException.class, () -> bridge.publishControl(true, 32, 1));
            bridge.publishControl(true,5,1,new MouseLook.Input(true,true,-135,25,1620,true));
            await(() -> {try{return Files.readString(log).contains("'absolute_yaw': True");}catch(Exception e){return false;}});
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
            var tile=java.util.Base64.getEncoder().encodeToString(new byte[1024]);var face=new BlockScene.Face(0,java.util.List.of(0.,0.,1.,0.,1.,1.,0.,1.));
            var material=new BlockScene(10,20,30,0,java.util.List.of(scene.boxes().getFirst()),3,true,java.util.List.of(tile),java.util.List.of(java.util.Collections.nCopies(6,face)));
            bridge.publishSceneControl(true,5,1,null,material);await(() -> bridge.previewStyle()==3);assertEquals(5,bridge.collisionContacts());assertEquals(4,bridge.collisionCorrections());

            assertTrue(Files.readString(log).contains("'origin': [10.0, 20.0, 30.0, 0.0]"));
            var hud=new HudFrame(java.util.Base64.getEncoder().encodeToString(new byte[HudFrame.BYTES]));
            bridge.publishSceneControl(true,5,1,new MouseLook.Input(true,true,0,0,1600),material,hud);
            await(() -> {try{return Files.readString(log).contains("HUD enabled=True generation=1 pixels=43692");}catch(Exception e){return false;}});
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
