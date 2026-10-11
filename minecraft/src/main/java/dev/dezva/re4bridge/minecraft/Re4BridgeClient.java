package dev.dezva.re4bridge.minecraft;

import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import dev.dezva.re4bridge.BridgeConfig;
import dev.dezva.re4bridge.BridgeConnection;
import dev.dezva.re4bridge.Re4Calibration;
import com.mojang.brigadier.arguments.DoubleArgumentType;
import com.mojang.brigadier.arguments.IntegerArgumentType;
import java.io.IOException;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicBoolean;
import net.fabricmc.api.ClientModInitializer;
import net.fabricmc.fabric.api.client.command.v2.ClientCommandRegistrationCallback;
import net.fabricmc.fabric.api.client.event.lifecycle.v1.ClientTickEvents;
import net.fabricmc.loader.api.FabricLoader;
import dev.dezva.re4bridge.ControlArming;
import dev.dezva.re4bridge.MouseLook;
import net.minecraft.client.gui.screen.ChatScreen;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.option.Perspective;
import net.minecraft.server.MinecraftServer;
import net.minecraft.server.network.ServerPlayerEntity;
import net.minecraft.server.world.ServerWorld;
import net.minecraft.block.ShapeContext;
import net.minecraft.text.Text;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Box;
import net.minecraft.util.math.Vec3d;
import static net.fabricmc.fabric.api.client.command.v2.ClientCommandManager.literal;
import static net.fabricmc.fabric.api.client.command.v2.ClientCommandManager.argument;

/** Diagnostic slice: no injection, no world editing, no native damage calls. */
public final class Re4BridgeClient implements ClientModInitializer {
    private BridgeConfig config;
    private volatile BridgeConnection bridge;
    private Perspective previousPerspective;
    private int ticks;
    private final long clockOrigin = System.nanoTime();
    private final AtomicBoolean sampling = new AtomicBoolean();
    private volatile Re4Calibration calibration;
    private volatile String calibrationDimension;
    private Re4Calibration.Target lastTarget;
    private volatile boolean following;
    private double unitsPerBlock = 100;
    private int ySign = -1;
    private int turnSign = 1;
    private final ControlArming controlArming = new ControlArming();
    private int controlHudTicks;
    private final MouseLook mouseLook = new MouseLook();
    private boolean mouseEnabled;
    private boolean firstPersonEnabled;
    private double eyeHeight = 1600;

    @Override public void onInitializeClient() {
        try {
            config = BridgeConfig.loadOrCreate(FabricLoader.getInstance().getConfigDir().resolve("re4bridge.json"));
        } catch (IOException e) {
            // Keep Minecraft playable when configuration is broken; report through /re4bridge.
            config = null;
        }
        ClientCommandRegistrationCallback.EVENT.register((dispatcher, access) -> dispatcher.register(
            literal("re4bridge")
                .then(literal("connect").executes(ctx -> connect(ctx.getSource().getClient(), false)))
                .then(literal("re4").executes(ctx -> connect(ctx.getSource().getClient(), true)))
                .then(literal("controller").executes(ctx -> connectController(ctx.getSource().getClient())))
                .then(literal("mouse").executes(ctx -> {
                    if (bridge == null || !bridge.controllerMode() || (bridge.nativeCapabilities() & 1) == 0) {
                        say(ctx.getSource().getClient(), "Raton no disponible: conecta el host y DLL 0.4.0 con controller."); return 0;
                    }
                    stopDriving(); mouseEnabled = !mouseEnabled;
                    if (!mouseEnabled) firstPersonEnabled = false;
                    say(ctx.getSource().getClient(), "Raton: " + mouseEnabled + ". Usa arm para probar."); return 1;
                }))
                .then(literal("firstperson").executes(ctx -> {
                    if (bridge == null || !bridge.controllerMode() || (bridge.nativeCapabilities() & 2) == 0) {
                        say(ctx.getSource().getClient(), "Camara no disponible: la DLL no valido la firma de camara."); return 0;
                    }
                    stopDriving(); firstPersonEnabled = !firstPersonEnabled;
                    if (firstPersonEnabled) mouseEnabled = true;
                    say(ctx.getSource().getClient(), "Primera persona experimental: " + firstPersonEnabled + ". Usa arm."); return 1;
                }))
                .then(literal("eyeheight").then(argument("height", DoubleArgumentType.doubleArg(-2500, 2500)).executes(ctx -> {
                    double height = DoubleArgumentType.getDouble(ctx, "height");
                    if (Math.abs(height) < 40) { say(ctx.getSource().getClient(), "Altura absoluta entre 40 y 2500 unidades RE4."); return 0; }
                    stopDriving(); eyeHeight = height;
                    say(ctx.getSource().getClient(), "Altura de camara: " + height + ". Usa arm."); return 1;
                })))
                .then(literal("arm").executes(ctx -> {
                    var current = bridge;
                    var sample = current == null ? null : current.re4Sample();
                    if (current == null || !current.controllerMode() || !current.status().connected()
                            || sample == null || !sample.fresh(System.nanoTime()) || current.nativeTicks() == 0) {
                        say(ctx.getSource().getClient(), "Conecta /re4bridge controller con RE4 cargado y el modulo nativo activo.");
                        return 0;
                    }
                    following = false;
                    controlArming.request(System.nanoTime());
                    mouseLook.reset();
                    current.publishControl(false, 0, controlArming.generation());
                    say(ctx.getSource().getClient(), "Cierra el chat para activar control. Mira el indicador sobre la barra de items. F8 detiene.");
                    return 1;
                }))
                .then(literal("stop").executes(ctx -> {
                    stopDriving();
                    say(ctx.getSource().getClient(), "Control RE4 desarmado.");
                    return 1;
                }))
                .then(literal("calibrate").executes(ctx -> calibrate(ctx.getSource().getClient())))
                .then(literal("scale").then(argument("units", DoubleArgumentType.doubleArg(1, 10000)).executes(ctx -> {
                    unitsPerBlock = DoubleArgumentType.getDouble(ctx, "units");
                    return calibrate(ctx.getSource().getClient());
                })))
                .then(literal("ysign").then(argument("sign", IntegerArgumentType.integer(-1, 1)).executes(ctx -> {
                    int sign = IntegerArgumentType.getInteger(ctx, "sign");
                    if (sign == 0) return 0;
                    ySign = sign;
                    return calibrate(ctx.getSource().getClient());
                })))
                .then(literal("turnsign").then(argument("sign", IntegerArgumentType.integer(-1, 1)).executes(ctx -> {
                    int sign = IntegerArgumentType.getInteger(ctx, "sign");
                    if (sign == 0) return 0;
                    turnSign = sign;
                    return calibrate(ctx.getSource().getClient());
                })))
                .then(literal("follow").executes(ctx -> {
                    stopDriving();
                    MinecraftClient client = ctx.getSource().getClient();
                    if (following) { following = false; say(client, "Seguimiento detenido."); return 1; }
                    if (client.player == null || !client.player.isSpectator() || calibration == null
                            || bridge == null || !bridge.readerMode() || !bridge.status().connected()) {
                        say(client, "Conecta con /re4bridge re4, calibra y usa modo espectador antes de /re4bridge follow.");
                        return 0;
                    }
                    following = true;
                    var p = client.player.getPos();
                    lastTarget = new Re4Calibration.Target(p.x, p.y, p.z, client.player.getYaw());
                    say(client, "Seguimiento RE4 -> Minecraft activo. /re4bridge follow lo detiene.");
                    return 1;
                }))
                .then(literal("disconnect").executes(ctx -> {
                    disconnect(ctx.getSource().getClient());
                    ctx.getSource().sendFeedback(Text.literal("RE4 Bridge: desconectado."));
                    return 1;
                }))
                .then(literal("status").executes(ctx -> {
                    String state = bridge == null ? "disconnected" : bridge.status().message()
                            + "; mensajes confirmados: " + bridge.status().acknowledgedSequence();
                    ctx.getSource().sendFeedback(Text.literal("RE4 Bridge: " + state));
                    if (bridge != null && bridge.controllerMode()) ctx.getSource().sendFeedback(Text.literal(
                        "Control: solicitado=" + controlArming.active() + "; causa=" + controlArming.reason() + "; nativo=" + bridge.nativeArmed() + "; parada="
                        + bridge.nativeStop() + "; ticks=" + bridge.nativeTicks()));
                    if (bridge != null && bridge.readerMode()) {
                        var sample = bridge.re4Sample();
                        ctx.getSource().sendFeedback(Text.literal(sample != null && sample.fresh(System.nanoTime())
                            ? String.format(java.util.Locale.ROOT, "RE4 raw: %.2f %.2f %.2f; angulo: %.3f; escala: %.1f; follow: %s",
                                sample.x(), sample.y(), sample.z(), sample.yawRadians(), unitsPerBlock, following)
                            : "RE4: sin personaje o muestra reciente; seguimiento suspendido."));
                    }
                    return 1;
                }))));
        ClientTickEvents.END_CLIENT_TICK.register(this::tick);
    }

    private int connect(MinecraftClient client, boolean readerMode) {
        return connect(client, readerMode ? BridgeConnection.Mode.READER : BridgeConnection.Mode.SIMULATOR);
    }

    private int connect(MinecraftClient client, BridgeConnection.Mode mode) {
        if (config == null) {
            say(client, "No se pudo leer config/re4bridge.json. Revisa su formato y reinicia Minecraft.");
            return 0;
        }
        if (client.player == null || client.getServer() == null || !client.isInSingleplayer()) {
            say(client, "El prototipo solo funciona en un mundo local de un jugador.");
            return 0;
        }
        disconnect(client);
        previousPerspective = client.options.getPerspective();
        client.options.setPerspective(Perspective.FIRST_PERSON);
        bridge = new BridgeConnection(config, mode);
        bridge.start();
        say(client, mode != BridgeConnection.Mode.SIMULATOR ? "Conectando a RE4. Usa /re4bridge status."
            : "Conectando al simulador local. RE4 todavía no está conectado. Usa /re4bridge status.");
        return 1;
    }

    private void disconnect(MinecraftClient client) {
        stopDriving();
        mouseEnabled = false; firstPersonEnabled = false;
        if (bridge != null) bridge.close();
        bridge = null;
        following = false;
        calibration = null;
        calibrationDimension = null;
        if (previousPerspective != null) client.options.setPerspective(previousPerspective);
        previousPerspective = null;
    }

    private int connectController(MinecraftClient client) {
        int result = connect(client, BridgeConnection.Mode.CONTROL);
        if (result == 0) return 0;
        say(client, "Conectando a controles RE4. Estado inicial desarmado; luego /re4bridge arm.");
        return 1;
    }

    private void stopDriving() {
        controlArming.stop("desarmado");
        mouseLook.reset();
        if (bridge != null && bridge.controllerMode()) bridge.publishControl(false, 0, controlArming.generation());
    }

    private int calibrate(MinecraftClient client) {
        stopDriving();
        following = false;
        calibration = null;
        var current = bridge;
        var sample = current == null ? null : current.re4Sample();
        if (client.player == null || client.world == null || current == null || !current.status().connected()
                || sample == null || !sample.fresh(System.nanoTime())) {
            say(client, "Se necesita una muestra reciente de RE4 y un mundo local abierto para calibrar.");
            return 0;
        }
        var pos = client.player.getPos();
        calibration = new Re4Calibration(sample, pos.x, pos.y, pos.z, client.player.getYaw(), unitsPerBlock, ySign, turnSign);
        calibrationDimension = client.world.getRegistryKey().getValue().toString();
        say(client, "Referencia fijada aqui; " + unitsPerBlock + " unidades RE4/bloque. Seguimiento detenido; usa /re4bridge follow.");
        return 1;
    }

    private void tick(MinecraftClient client) {
        BridgeConnection current = bridge;
        if (current == null) return;
        MinecraftServer server = client.getServer();
        if (client.player == null || server == null || !client.isInSingleplayer()) {
            disconnect(client);
            return;
        }
        if (!current.status().connected()) {
            stopDriving();
            following = false;
            calibration = null;
            if (!current.status().message().equals("connecting") && previousPerspective != null) {
                client.options.setPerspective(previousPerspective);
                previousPerspective = null;
            }
            return;
        }
        if (current.controllerMode()) {
            var sample = current.re4Sample();
            long now = System.nanoTime();
            var screen = client.currentScreen == null ? ControlArming.Screen.NONE
                : client.currentScreen instanceof ChatScreen ? ControlArming.Screen.CHAT : ControlArming.Screen.OTHER;
            controlArming.tick(now, client.isPaused(), client.isWindowFocused(), screen,
                sample != null && sample.fresh(now), current.nativeArmed(), current.nativeStop());
            int buttons = 0;
            if (controlArming.active()) {
                if (client.options.forwardKey.isPressed()) buttons |= 1;
                if (client.options.backKey.isPressed()) buttons |= 2;
                if (client.options.rightKey.isPressed()) buttons |= 4;
                if (client.options.leftKey.isPressed()) buttons |= 8;
                if (buttons != 0 && client.options.sprintKey.isPressed()) buttons |= 64;
            }
            MouseLook.Input look = null;
            if (controlArming.active() && mouseEnabled) {
                try { look = mouseLook.sample(client.player.getYaw(), client.player.getPitch(), firstPersonEnabled, eyeHeight); }
                catch (IllegalArgumentException error) { controlArming.stop("orientacion invalida"); buttons = 0; mouseLook.reset(); }
            } else mouseLook.reset();
            current.publishControl(controlArming.active(), buttons, controlArming.generation(), look);
            if (++controlHudTicks % 5 == 0) client.player.sendMessage(Text.literal(
                "RE4: " + controlArming.reason() + " | arm=" + (current.nativeArmed() ? "si" : "no")
                + " | teclas=" + buttons + "/" + current.nativeButtons()
                + " | raton=" + (mouseEnabled ? Long.toString(current.nativeLookPackets()) : "off")
                + " | FP=" + (current.cameraActive() ? "si" : "no")
                + (firstPersonEnabled ? " | altura=" + (int)eyeHeight : "")
                + (current.nativeArmed() ? "" : " | " + current.nativeReason())), true);
        }
        if (client.isPaused()) return;
        if (current.readerMode()) {
            if (!client.world.getRegistryKey().getValue().toString().equals(calibrationDimension)) {
                following = false;
                calibration = null;
            }
            if (following && !client.player.isSpectator()) {
                following = false;
                say(client, "Seguimiento detenido: requiere modo espectador.");
            }
            var sample = current.re4Sample();
            if (following && calibration != null && sample != null && sample.fresh(System.nanoTime())
                    && ++ticks % 2 == 0 && sampling.compareAndSet(false, true)) {
                Re4Calibration mapping = calibration;
                UUID playerId = client.player.getUuid();
                try {
                    var target = mapping.map(sample);
                    if (lastTarget != null && (Math.abs(target.x() - lastTarget.x()) > 64
                            || Math.abs(target.y() - lastTarget.y()) > 64 || Math.abs(target.z() - lastTarget.z()) > 64)) {
                        throw new IllegalArgumentException("Scene jump");
                    }
                    lastTarget = target;
                    server.execute(() -> {
                        try {
                            var player = server.getPlayerManager().getPlayer(playerId);
                            if (player != null && player.isSpectator() && bridge == current && following
                                    && calibration == mapping && current.status().connected()
                                    && sample.fresh(System.nanoTime()) && current.re4Sample() != null
                                    && player.getServerWorld().getRegistryKey().getValue().toString().equals(calibrationDimension)) {
                                player.teleport(player.getServerWorld(), target.x(), target.y(), target.z(), target.yaw(), 0);
                                player.setVelocity(Vec3d.ZERO);
                            }
                        } finally { sampling.set(false); }
                    });
                } catch (IllegalArgumentException e) {
                    sampling.set(false);
                    following = false;
                    say(client, "Seguimiento detenido: salto de escena o posicion fuera de rango. Usa /re4bridge calibrate.");
                }
            }
            return;
        }
        JsonObject pose = new JsonObject();
        pose.addProperty("type", "pose");
        pose.addProperty("sample_ns", System.nanoTime() - clockOrigin);
        pose.addProperty("dimension", client.world.getRegistryKey().getValue().toString());
        pose.add("feet", vector(client.player.getPos()));
        pose.add("eye", vector(client.player.getEyePos()));
        pose.addProperty("yaw_deg", client.player.getYaw());
        pose.addProperty("pitch_deg", client.player.getPitch());
        current.publishPose(pose);
        if (++ticks % 5 != 0 || !sampling.compareAndSet(false, true)) return;
        UUID playerId = client.player.getUuid();
        // Collision queries belong to the integrated server thread, never the network worker.
        server.execute(() -> {
            try {
                ServerPlayerEntity player = server.getPlayerManager().getPlayer(playerId);
                if (player != null && bridge == current && current.status().connected()) {
                    current.publishCollision(sample(player));
                }
            } finally {
                sampling.set(false);
            }
        });
    }

    private JsonObject sample(ServerPlayerEntity player) {
        ServerWorld world = player.getServerWorld();
        BlockPos center = player.getBlockPos();
        JsonArray blocks = new JsonArray();
        boolean complete = true;
        for (BlockPos pos : BlockPos.iterate(center.add(-4, -2, -4), center.add(4, 2, 4))) {
            if (!world.isChunkLoaded(pos)) {
                complete = false;
                continue;
            }
            var state = world.getBlockState(pos);
            if (state.isAir()) continue;
            JsonObject block = new JsonObject();
            block.add("pos", vector(new Vec3d(pos.getX(), pos.getY(), pos.getZ())));
            block.addProperty("state", state.toString());
            var shapes = state.getCollisionShape(world, pos, ShapeContext.of(player)).getBoundingBoxes();
            boolean shapeComplete = shapes.size() <= 256;
            block.addProperty("collision_complete", shapeComplete);
            complete &= shapeComplete;
            JsonArray boxes = new JsonArray();
            if (shapeComplete) for (Box box : shapes) {
                JsonArray bounds = new JsonArray();
                for (double value : new double[]{box.minX, box.minY, box.minZ, box.maxX, box.maxY, box.maxZ}) {
                    bounds.add(value);
                }
                boxes.add(bounds);
            }
            block.add("boxes", boxes);
            blocks.add(block);
        }
        JsonObject frame = new JsonObject();
        frame.addProperty("type", "collision_snapshot");
        frame.addProperty("sample_ns", System.nanoTime() - clockOrigin);
        frame.addProperty("dimension", world.getRegistryKey().getValue().toString());
        frame.add("center", vector(new Vec3d(center.getX(), center.getY(), center.getZ())));
        frame.addProperty("complete", complete);
        frame.add("blocks", blocks);
        return frame;
    }

    private static JsonArray vector(Vec3d pos) {
        JsonArray result = new JsonArray();
        result.add(pos.x); result.add(pos.y); result.add(pos.z);
        return result;
    }

    private static void say(MinecraftClient client, String text) {
        if (client.player != null) client.player.sendMessage(Text.literal("RE4 Bridge: " + text), false);
    }
}
