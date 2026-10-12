package dev.dezva.re4bridge;

/** Client-thread arming: wait for command chat to close before requesting native input. */
public final class ControlArming {
    public enum Screen { NONE, CHAT, INVENTORY, OTHER }
    private boolean pending;
    private boolean active;
    private int generation;
    private long requestedNs;
    private long activatedNs;
    private String reason = "desarmado";

    public void request(long now) {
        pending = true;
        active = false;
        ++generation;
        requestedNs = now;
        reason = "esperando cerrar el chat";
    }
    public void stop(String cause) { pending = false; active = false; reason = cause; }
    public boolean active() { return active; }
    public boolean pending() { return pending; }
    public int generation() { return generation; }
    public String reason() { return reason; }

    public void tick(long now, boolean paused, boolean focused, Screen screen,
            boolean freshSample, boolean nativeArmed, int nativeStop) {
        if (!pending && !active) return;
        if (!focused) { stop("Minecraft perdio el foco"); return; }
        if (paused) { stop("Minecraft pausado"); return; }
        if (!freshSample) { stop("sin muestra reciente de RE4"); return; }
        if (pending) {
            if (now - requestedNs >= 2_000_000_000L) { stop("chat no cerrado; repite arm"); return; }
            if (screen == Screen.CHAT) return;
            if (screen != Screen.NONE) { stop("menu abierto"); return; }
            pending = false;
            active = true;
            activatedNs = now;
            reason = "solicitando control nativo";
            return;
        }
        if (screen != Screen.NONE && screen != Screen.INVENTORY) { stop("chat o menu abierto"); return; }
        // Start the acknowledgement deadline when input is actually allowed, not inside chat.
        if (now - activatedNs >= 1_000_000_000L && (!nativeArmed || nativeStop != 0)) {
            stop(nativeStop != 0 ? "parada nativa " + nativeStop : "host rechazo el armado; repite arm");
            return;
        }
        reason = screen == Screen.INVENTORY ? "inventario; movimiento suspendido" : nativeArmed ? "activo" : "solicitando control nativo";
    }
}
