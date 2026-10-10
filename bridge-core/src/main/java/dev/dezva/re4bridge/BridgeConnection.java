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
    private final boolean readerMode;
    private volatile Re4Sample re4Sample;
    private final AtomicReference<JsonObject> pose = new AtomicReference<>();
    private final AtomicReference<JsonObject> collision = new AtomicReference<>();
    private volatile Status status = new Status(false, "disconnected", 0);
    private volatile boolean running;
    private volatile Socket socket;
    private Thread worker;

    public BridgeConnection(BridgeConfig config) { this(config, false); }
    public BridgeConnection(BridgeConfig config, boolean readerMode) { this.config = config; this.readerMode = readerMode; }
    public boolean readerMode() { return readerMode; }
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
                    || !(readerMode ? "re4_reader" : "simulator").equals(string(welcome, "mode"))) {
                throw new IOException("Unexpected peer mode");
            }
            String modeMessage = readerMode ? "RE4 reader (read only)" : "simulator (RE4 is not connected)";
            status = new Status(true, modeMessage, 0);
            long sequence = 0;
            long lastSend = System.nanoTime();
            boolean preferCollision = true;
            while (running) {
                JsonObject frame = null;
                if (readerMode) {
                    if (sequence > 0 && System.nanoTime() - lastSend < 100_000_000L) {
                        Thread.sleep(Duration.ofMillis(10));
                        continue;
                    }
                    frame = new JsonObject();
                    frame.addProperty("type", "re4_poll");
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
                if (readerMode) {
                    if (!ack.has("re4_sample") || !ack.get("re4_sample").isJsonObject()) throw new IOException("Missing RE4 sample");
                    re4Sample = Re4Sample.parse(ack.getAsJsonObject("re4_sample"), System.nanoTime());
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
            socket = null;
            if (status.connected()) status = new Status(false, "disconnected", status.acknowledgedSequence());
        }
    }

    private static String string(JsonObject frame, String key) {
        return frame.has(key) && frame.get(key).isJsonPrimitive() ? frame.get(key).getAsString() : "";
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
    }
}
