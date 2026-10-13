package dev.dezva.re4bridge;

import com.google.gson.JsonObject;
import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.time.Duration;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.Supplier;

/** One background worker; latest-value slots bound memory when the peer is slow. */
public final class BridgeConnection implements AutoCloseable {
    public record Status(boolean connected, String message, long acknowledgedSequence) {}
    private final BridgeConfig config;
    public enum Mode { SIMULATOR, READER, CONTROL }
    private final Mode mode;
    private record ControlFrame(JsonObject frame, HudFrame hud, long createdNs) {}
    private volatile Supplier<HudFrame> overlaySource;
    private volatile boolean terrainRequested;private volatile TerrainSnapshot terrain;private volatile String terrainState="sin host de mapa";
    private volatile boolean depthRequested,depthAvailable;private volatile String depthReason="sin canal de profundidad";private volatile long depthDraws,depthFallbackDraws;
    private volatile boolean menuRequested;
    private volatile HealthRequest healthRequest;
    private volatile HealthState healthState;
    public void healthRequest(HealthRequest request){healthRequest=request;}
    public HealthState healthState(){return healthState;}
    public void menuRequested(boolean value){menuRequested=value;}
    /** Called before vanilla opens a GUI and clears keys/pauses its server. */
    public void suspendForMenu(){
        menuRequested=true;
        control.updateAndGet(old->{if(old==null)return null;var frame=old.frame().deepCopy();frame.addProperty("buttons",0);frame.addProperty("menu",true);return new ControlFrame(frame,old.hud(),System.nanoTime());});
    }
    public void depthRequested(boolean value){depthRequested=value;}public boolean depthAvailable(){return depthAvailable;}public String depthReason(){return depthReason;}public long depthDraws(){return depthDraws;}public long depthFallbackDraws(){return depthFallbackDraws;}
    public void terrainRequested(boolean requested){terrainRequested=requested;}
    public TerrainSnapshot terrain(){return terrain;}public String terrainState(){return terrainState;}
    /** Source reads only an immutable encoder result, never Minecraft or OpenGL state. */
    public void overlaySource(Supplier<HudFrame> source) { overlaySource=source; }
    private final AtomicReference<ControlFrame> control = new AtomicReference<>();
    private volatile boolean nativeArmed;
    private volatile int nativeStop;
    private volatile long nativeTicks;
    private volatile int nativeButtons;
    private volatile String nativeReason = "sin detalle";
    private volatile int nativeCapabilities;
    private volatile boolean cameraActive;
    private volatile long nativeLookPackets;
    private volatile Re4Sample re4Sample;
    private final AtomicReference<JsonObject> pose = new AtomicReference<>();
    private final AtomicReference<JsonObject> collision = new AtomicReference<>();
    private volatile Status status = new Status(false, "disconnected", 0);
    private volatile boolean running;
    private volatile Socket socket;
    private Thread worker;

    public BridgeConnection(BridgeConfig config) { this(config, Mode.SIMULATOR); }
    public BridgeConnection(BridgeConfig config, boolean readerMode) { this(config, readerMode ? Mode.READER : Mode.SIMULATOR); }
    public BridgeConnection(BridgeConfig config, Mode mode) { this.config = config; this.mode = mode; }
    public boolean readerMode() { return mode != Mode.SIMULATOR; }
    public boolean controllerMode() { return mode == Mode.CONTROL; }
    public boolean nativeArmed() { return nativeArmed; }
    public int nativeStop() { return nativeStop; }
    public long nativeTicks() { return nativeTicks; }
    public int nativeButtons() { return nativeButtons; }
    public String nativeReason() { return nativeReason; }
    public int nativeCapabilities() { return nativeCapabilities; }
    private volatile int previewStatus = -1;
    private volatile boolean previewAvailable;
    private volatile boolean sceneAvailable;
    private volatile boolean surfaceAvailable;
    private volatile boolean textureAvailable;
    private volatile boolean materialsAvailable,collisionAvailable,hudAvailable;
    public boolean hudAvailable(){return hudAvailable;}
    private volatile boolean overlayAvailable;private volatile int overlayWidth,overlayHeight,nativeHudHidden;
    public boolean overlayAvailable(){return overlayAvailable;}
    public int overlayWidth(){return overlayWidth;}public int overlayHeight(){return overlayHeight;}public int nativeHudHidden(){return nativeHudHidden;}
    public boolean materialsAvailable(){return materialsAvailable;}
    public boolean collisionAvailable(){return collisionAvailable;}
    public boolean textureAvailable() {return textureAvailable;}
    private volatile long collisionContacts,collisionCorrections;
    private volatile boolean collisionActive;
    public long collisionContacts(){return collisionContacts;}
    public long collisionCorrections(){return collisionCorrections;}
    public boolean collisionActive(){return collisionActive;}
    private volatile int previewStyle;
    public boolean surfaceAvailable() {return surfaceAvailable;}
    public int previewStyle() {return previewStyle;}
    private volatile int previewBoxCount;
    public boolean sceneAvailable() { return sceneAvailable; }
    public int previewBoxCount() { return previewBoxCount; }
    private volatile boolean previewRequested;
    private volatile long previewSkipped;
    private volatile String previewReason = "sin estado";
    public boolean previewRequested() { return previewRequested; }
    public long previewSkipped() { return previewSkipped; }
    public String previewReason() { return previewReason; }
    public boolean previewAvailable() { return previewAvailable; }
    private volatile long previewFrames, previewDraws;
    private volatile long previewFailures,previewHresult;
    public int previewStatus() { return previewStatus; }
    public long previewFrames() { return previewFrames; }
    public long previewDraws() { return previewDraws; }
    public long previewFailures() { return previewFailures; }
    public long previewHresult() { return previewHresult; }
    public boolean cameraActive() { return cameraActive; }
    public long nativeLookPackets() { return nativeLookPackets; }
    public void publishControl(boolean armed, int buttons, int generation) {
        publishControl(armed, buttons, generation, null);
    }
    public void publishControl(boolean armed, int buttons, int generation, MouseLook.Input look) {
        publishControl(armed,buttons,generation,look,null);
    }
    public void publishControl(boolean armed, int buttons, int generation, MouseLook.Input look, BlockPreview preview) {
        publishVisualControl(armed,buttons,generation,look,preview==null?null:preview.toJson());
    }
    // Client-thread owned, immutable scene identity; the worker only reads this JSON tree.
    private BlockScene serializedScene;
    private JsonObject serializedSceneJson;
    public void publishSceneControl(boolean armed,int buttons,int generation,MouseLook.Input look,BlockScene scene) {publishSceneControl(armed,buttons,generation,look,scene,null);}
    public void publishSceneControl(boolean armed,int buttons,int generation,MouseLook.Input look,BlockScene scene,HudFrame hud) {
        if(scene!=serializedScene){serializedScene=scene;serializedSceneJson=scene==null?null:scene.toJson();}
        publishVisualControl(armed,buttons,generation,look,serializedSceneJson,hud);
    }
    private void publishVisualControl(boolean armed,int buttons,int generation,MouseLook.Input look,JsonObject preview){publishVisualControl(armed,buttons,generation,look,preview,null);}
    private void publishVisualControl(boolean armed,int buttons,int generation,MouseLook.Input look,JsonObject preview,HudFrame hud) {
        if (buttons < 0 || (buttons & ~79) != 0 || generation < 0) throw new IllegalArgumentException("Invalid control");
        JsonObject frame = new JsonObject();
        frame.addProperty("type", "re4_control"); frame.addProperty("armed", armed);
        frame.addProperty("buttons", buttons); frame.addProperty("arm_id", generation);
        frame.addProperty("menu",armed && menuRequested);
        if (armed && preview != null) frame.add("preview",preview);
        if (look != null && armed) {
            JsonObject data = new JsonObject();
            data.addProperty("enabled", look.enabled()); data.addProperty("first_person", look.firstPerson());
            data.addProperty("total_yaw", look.totalYaw()); data.addProperty("pitch", look.pitch());
            data.addProperty("absolute_yaw",look.absoluteYaw()); data.addProperty("eye_height", look.eyeHeight()); frame.add("look", data);
        }
        control.set(new ControlFrame(frame,armed && look!=null && look.firstPerson()?hud:null, System.nanoTime()));
    }
    public Re4Sample re4Sample() { return re4Sample; }
    public Status status() { return status; }
    public void publishPose(JsonObject frame) { pose.set(frame.deepCopy()); }
    public void publishCollision(JsonObject frame) { collision.set(frame.deepCopy()); }

    public synchronized void start() {
        if (worker != null) throw new IllegalStateException("Use a new connection for each session");
        running = true;
        status = new Status(false, "connecting", 0);
        worker = new Thread(this::run, "re4bridge-local");
        worker.setDaemon(true);
        worker.start();
    }

    private void run() {
        String session = UUID.randomUUID().toString();
        try (Socket peer = new Socket()) {
            socket = peer;
            if (!running) return;
            peer.connect(new InetSocketAddress("127.0.0.1", config.port()), 1500);
            peer.setSoTimeout(2500);
            peer.setTcpNoDelay(true);
            var input = new java.io.BufferedInputStream(peer.getInputStream(),16384);
            JsonObject hello = new JsonObject();
            hello.addProperty("protocol", LineProtocol.VERSION);
            hello.addProperty("type", "hello");
            hello.addProperty("session", session);
            hello.addProperty("token", config.token());
            peer.getOutputStream().write(LineProtocol.encode(hello));
            JsonObject welcome = LineProtocol.read(input);
            if (!"welcome".equals(string(welcome, "type")) || !session.equals(string(welcome, "session"))
                    || !(mode == Mode.CONTROL ? "re4_control" : readerMode() ? "re4_reader" : "simulator").equals(string(welcome, "mode"))) {
                throw new IOException("Unexpected peer mode");
            }
            String modeMessage = mode == Mode.CONTROL ? "RE4 controller" : readerMode() ? "RE4 reader (read only)" : "simulator (RE4 is not connected)";
            status = new Status(true, modeMessage, 0);
            long sequence = 0;
            long lastSend = System.nanoTime();
            boolean preferCollision = true;
            while (running) {
                JsonObject frame = null;
                if (readerMode()) {
                    if (sequence > 0 && System.nanoTime() - lastSend < (controllerMode() ? 16_666_667L : 100_000_000L)) {
                        Thread.sleep(Duration.ofMillis(1));
                        continue;
                    }
                    frame = new JsonObject();
                    frame.addProperty("type", "re4_poll");
                    if (controllerMode()) {
                        var latest = control.get();
                        // GUI initialization can stall a render tick. Only its zero-input
                        // snapshot receives this bounded transition grace; movement never does.
                        if (latest != null && System.nanoTime() - latest.createdNs() < (latest.frame().has("menu") && latest.frame().get("menu").getAsBoolean() && latest.frame().get("buttons").getAsInt()==0?1_000_000_000L:200_000_000L)) {
                            frame = latest.frame().deepCopy();
                            var health=healthRequest;
                            if(health!=null && frame.get("armed").getAsBoolean() && health.generation()==frame.get("arm_id").getAsInt()
                                && (System.nanoTime()-health.createdNs()<500_000_000L || frame.get("menu").getAsBoolean()))frame.add("health",health.toJson());
                            var hud=latest.hud();var source=overlaySource;
                            if(hud!=null && source!=null){
                                var visual=source.get();
                                hud=visual!=null && visual.menu()==hud.menu()?hud.visual(visual):null;
                            }
                            if(hud!=null)frame.add("hud",hud.toJson());
                            frame.addProperty("terrain",terrainRequested && frame.get("armed").getAsBoolean());frame.addProperty("terrain_revision",terrain==null?0:terrain.revision());
                            frame.addProperty("depth",depthRequested && frame.get("armed").getAsBoolean());
                        }
                        else {
                            frame.addProperty("type", "re4_control"); frame.addProperty("armed", false);
                            frame.addProperty("buttons", 0);
                            frame.addProperty("arm_id", latest == null ? 0 : latest.frame().get("arm_id").getAsInt());
                        }
                    }
                } else frame = preferCollision ? collision.getAndSet(null) : pose.getAndSet(null);
                if (frame == null) frame = preferCollision ? pose.getAndSet(null) : collision.getAndSet(null);
                preferCollision = !preferCollision;
                if (frame == null && System.nanoTime() - lastSend >= 1_000_000_000L) {
                    frame = new JsonObject();
                    frame.addProperty("type", "heartbeat");
                }
                if (frame == null) {
                    Thread.sleep(Duration.ofMillis(10));
                    continue;
                }
                frame.addProperty("protocol", LineProtocol.VERSION);
                frame.addProperty("session", session);
                frame.addProperty("seq", ++sequence);
                peer.getOutputStream().write(LineProtocol.encode(frame));
                lastSend = System.nanoTime();
                JsonObject ack = LineProtocol.read(input);
                if (!"ack".equals(string(ack, "type")) || !session.equals(string(ack, "session"))
                        || !Long.toString(sequence).equals(string(ack, "seq"))) {
                    throw new IOException("Invalid acknowledgement");
                }
                if (readerMode()) {
                    if (!ack.has("re4_sample") || !ack.get("re4_sample").isJsonObject()) throw new IOException("Missing RE4 sample");
                    re4Sample = Re4Sample.parse(ack.getAsJsonObject("re4_sample"), System.nanoTime());
                    if (controllerMode()) {
                        JsonObject state = ack.getAsJsonObject("control_state");
                        if (state == null || !state.getAsJsonPrimitive("armed").isBoolean()
                                || !state.getAsJsonPrimitive("stop").isNumber() || !state.getAsJsonPrimitive("hook_ticks").isNumber())
                            throw new IOException("Invalid control acknowledgement");
                        nativeArmed = state.get("armed").getAsBoolean();
                        if(state.has("health_state"))healthState=HealthState.parse(state.getAsJsonObject("health_state"));
                        if(state.has("terrain"))terrain=TerrainSnapshot.parse(state.getAsJsonObject("terrain"),System.nanoTime());
                        if(state.has("terrain_state")){var value=state.getAsJsonPrimitive("terrain_state");if(!value.isString() || value.getAsString().length()>160)throw new IOException("Invalid terrain state");terrainState=value.getAsString();}
                        nativeStop = state.get("stop").getAsInt();
                        nativeTicks = state.get("hook_ticks").getAsLong();
                        if (nativeStop < 0 || nativeStop > 2 || nativeTicks < 0 || nativeTicks > 0xFFFFFFFFL)
                            throw new IOException("Invalid native status");
                        if (!state.get("stop").getAsString().equals(Integer.toString(nativeStop))
                                || !state.get("hook_ticks").getAsString().equals(Long.toString(nativeTicks)))
                            throw new IOException("Non integral native status");
                        if (state.has("native_buttons")) {
                            var value = state.getAsJsonPrimitive("native_buttons");
                            nativeButtons = value.getAsInt();
                            if (!value.isNumber() || nativeButtons < 0 || (nativeButtons & ~79) != 0
                                    || !value.getAsString().equals(Integer.toString(nativeButtons)))
                                throw new IOException("Invalid native button status");
                        }
                        if (state.has("reason")) {
                            var value = state.getAsJsonPrimitive("reason");
                            if (!value.isString() || value.getAsString().length() > 160)
                                throw new IOException("Invalid native reason");
                            nativeReason = value.getAsString();
                        }
                        if (state.has("capabilities")) nativeCapabilities = boundedInteger(state, "capabilities", 31);
                        if (state.has("preview_state")) {
                            var preview=state.getAsJsonObject("preview_state");
                            previewStatus=boundedInteger(preview,"status",4);
                            if(preview.has("depth_available")){var available=preview.getAsJsonPrimitive("depth_available");if(!available.isBoolean())throw new IOException("Invalid depth state");depthAvailable=available.getAsBoolean();}
                            if(preview.has("depth_reason")){var value=preview.getAsJsonPrimitive("depth_reason");if(!value.isString() || value.getAsString().length()>160)throw new IOException("Invalid depth reason");depthReason=value.getAsString();}
                            if(preview.has("depth_draws"))depthDraws=unsignedCounter(preview,"depth_draws");
                            if(preview.has("depth_fallback_draws"))depthFallbackDraws=unsignedCounter(preview,"depth_fallback_draws");
                            previewFrames=unsignedCounter(preview,"frames");
                            previewDraws=unsignedCounter(preview,"draws");
                            previewFailures=unsignedCounter(preview,"failures");
                            previewHresult=unsignedCounter(preview,"last_hresult");
                            if (preview.has("requested")) {
                                var value=preview.getAsJsonPrimitive("requested");
                                if (!value.isBoolean()) throw new IOException("Invalid preview request status");
                                previewRequested=value.getAsBoolean();
                            }
                            if (preview.has("skipped_passes")) previewSkipped=unsignedCounter(preview,"skipped_passes");
                            if (preview.has("box_count")) previewBoxCount=boundedInteger(preview,"box_count",BlockScene.MAX_BOXES);
                            if(preview.has("collision_state")) {
                                var c=preview.getAsJsonObject("collision_state");collisionContacts=unsignedCounter(c,"contacts");collisionCorrections=unsignedCounter(c,"corrections");
                                if(!c.getAsJsonPrimitive("active").isBoolean())throw new IOException("Invalid collision state");collisionActive=c.get("active").getAsBoolean();boundedInteger(c,"reason",5);
                            }
                            if (preview.has("style")) previewStyle=boundedInteger(preview,"style",3);
                            if (preview.has("reason")) {
                                var value=preview.getAsJsonPrimitive("reason");
                                if (!value.isString() || value.getAsString().length()>160)
                                    throw new IOException("Invalid preview reason");
                                previewReason=value.getAsString();
                            }
                        }
                        if (state.has("preview_available")) {
                            var available=state.getAsJsonPrimitive("preview_available");
                            if (!available.isBoolean()) throw new IOException("Invalid preview capability");
                            previewAvailable=available.getAsBoolean();
                        }
                        if (state.has("scene_available")) {
                            var available=state.getAsJsonPrimitive("scene_available");
                            if (!available.isBoolean()) throw new IOException("Invalid scene capability");
                            sceneAvailable=available.getAsBoolean();
                        }
                        if(state.has("overlay_available")){var v=state.getAsJsonPrimitive("overlay_available");if(!v.isBoolean())throw new IOException("Invalid overlay capability");overlayAvailable=v.getAsBoolean();}
                        if(state.has("overlay_viewport_width"))overlayWidth=boundedInteger(state,"overlay_viewport_width",16384);
                        if(state.has("overlay_viewport_height"))overlayHeight=boundedInteger(state,"overlay_viewport_height",16384);
                        if(state.has("native_hud_hidden"))nativeHudHidden=boundedInteger(state,"native_hud_hidden",256);
                        if(state.has("hud_available")){var v=state.getAsJsonPrimitive("hud_available");if(!v.isBoolean())throw new IOException("Invalid HUD capability");hudAvailable=v.getAsBoolean();}
                        if(state.has("materials_available")){var v=state.getAsJsonPrimitive("materials_available");if(!v.isBoolean())throw new IOException("Invalid material capability");materialsAvailable=v.getAsBoolean();}
                        if(state.has("collision_available")){var v=state.getAsJsonPrimitive("collision_available");if(!v.isBoolean())throw new IOException("Invalid collision capability");collisionAvailable=v.getAsBoolean();}
                        if (state.has("texture_available")) {
                            var available=state.getAsJsonPrimitive("texture_available");
                            if (!available.isBoolean()) throw new IOException("Invalid texture capability");
                            textureAvailable=available.getAsBoolean();
                        }
                        if (state.has("surface_available")) {
                            var available=state.getAsJsonPrimitive("surface_available");
                            if (!available.isBoolean()) throw new IOException("Invalid surface capability");
                            surfaceAvailable=available.getAsBoolean();
                        }
                        if (state.has("camera_active")) {
                            if (!state.getAsJsonPrimitive("camera_active").isBoolean()) throw new IOException("Invalid camera state");
                            cameraActive = state.get("camera_active").getAsBoolean();
                        }
                        if (state.has("look_packets")) {
                            var value = state.getAsJsonPrimitive("look_packets");
                            nativeLookPackets = value.getAsLong();
                            if (!value.isNumber() || nativeLookPackets < 0 || nativeLookPackets > 0xFFFFFFFFL
                                    || !value.getAsString().equals(Long.toString(nativeLookPackets)))
                                throw new IOException("Invalid look counter");
                        }
                    }
                }
                status = new Status(true, modeMessage, sequence);
            }
        } catch (IOException | RuntimeException e) {
            if (running) status = new Status(false, "connection failed; check host, config and protocol", status.acknowledgedSequence());
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        } finally {
            running = false;
            re4Sample = null;
            nativeArmed = false;
            previewRequested = false;
            socket = null;
            if (status.connected()) status = new Status(false, "disconnected", status.acknowledgedSequence());
        }
    }

    private static String string(JsonObject frame, String key) {
        return frame.has(key) && frame.get(key).isJsonPrimitive() ? frame.get(key).getAsString() : "";
    }
    private static long unsignedCounter(JsonObject frame,String key) throws IOException {
        var value=frame.getAsJsonPrimitive(key);long result=value.getAsLong();
        if (!value.isNumber() || result<0 || result>0xFFFFFFFFL || !value.getAsString().equals(Long.toString(result)))
            throw new IOException("Invalid "+key);
        return result;
    }
    private static int boundedInteger(JsonObject frame, String key, int max) throws IOException {
        var value = frame.getAsJsonPrimitive(key);
        int result = value.getAsInt();
        if (!value.isNumber() || result < 0 || result > max || !value.getAsString().equals(Integer.toString(result)))
            throw new IOException("Invalid " + key);
        return result;
    }

    @Override public void close() {
        running = false;
        previewRequested = false;
        Socket peer = socket;
        if (peer != null) {
            try { peer.close(); } catch (IOException ignored) { }
        }
        if (worker != null) worker.interrupt();
        status = new Status(false, "disconnected", status.acknowledgedSequence());
        pose.set(null);
        collision.set(null);
        re4Sample = null;
        nativeArmed = false;
        control.set(null);
    }
}
