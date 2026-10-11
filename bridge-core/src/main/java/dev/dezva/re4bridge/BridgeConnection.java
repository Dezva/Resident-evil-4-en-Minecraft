package dev.dezva.re4bridge;

import com.google.gson.JsonObject;
import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.time.Duration;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicReference;

/** One background worker; latest-value slots bound memory when the peer is slow. */
public final class BridgeConnection implements AutoCloseable {
    public record Status(boolean connected, String message, long acknowledgedSequence) {}
    private final BridgeConfig config;
    public enum Mode { SIMULATOR, READER, CONTROL }
    private final Mode mode;
    private record ControlFrame(JsonObject frame, long createdNs) {}
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
    public boolean cameraActive() { return cameraActive; }
    public long nativeLookPackets() { return nativeLookPackets; }
    public void publishControl(boolean armed, int buttons, int generation) {
        publishControl(armed, buttons, generation, null);
    }
    public void publishControl(boolean armed, int buttons, int generation, MouseLook.Input look) {
        if (buttons < 0 || (buttons & ~79) != 0 || generation < 0) throw new IllegalArgumentException("Invalid control");
        JsonObject frame = new JsonObject();
        frame.addProperty("type", "re4_control"); frame.addProperty("armed", armed);
        frame.addProperty("buttons", buttons); frame.addProperty("arm_id", generation);
        if (look != null && armed) {
            JsonObject data = new JsonObject();
            data.addProperty("enabled", look.enabled()); data.addProperty("first_person", look.firstPerson());
            data.addProperty("total_yaw", look.totalYaw()); data.addProperty("pitch", look.pitch());
            data.addProperty("eye_height", look.eyeHeight()); frame.add("look", data);
        }
        control.set(new ControlFrame(frame, System.nanoTime()));
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
            JsonObject hello = new JsonObject();
            hello.addProperty("protocol", LineProtocol.VERSION);
            hello.addProperty("type", "hello");
            hello.addProperty("session", session);
            hello.addProperty("token", config.token());
            peer.getOutputStream().write(LineProtocol.encode(hello));
            JsonObject welcome = LineProtocol.read(peer.getInputStream());
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
                    if (sequence > 0 && System.nanoTime() - lastSend < (controllerMode() ? 50_000_000L : 100_000_000L)) {
                        Thread.sleep(Duration.ofMillis(10));
                        continue;
                    }
                    frame = new JsonObject();
                    frame.addProperty("type", "re4_poll");
                    if (controllerMode()) {
                        var latest = control.get();
                        if (latest != null && System.nanoTime() - latest.createdNs() < 200_000_000L) frame = latest.frame().deepCopy();
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
                JsonObject ack = LineProtocol.read(peer.getInputStream());
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
                        if (state.has("capabilities")) nativeCapabilities = boundedInteger(state, "capabilities", 7);
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
            socket = null;
            if (status.connected()) status = new Status(false, "disconnected", status.acknowledgedSequence());
        }
    }

    private static String string(JsonObject frame, String key) {
        return frame.has(key) && frame.get(key).isJsonPrimitive() ? frame.get(key).getAsString() : "";
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
