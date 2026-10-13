package dev.dezva.re4bridge.minecraft;

import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import dev.dezva.re4bridge.BridgeConfig;
import dev.dezva.re4bridge.BridgeConnection;
import dev.dezva.re4bridge.Re4Calibration;
import dev.dezva.re4bridge.BlockPreview;
import dev.dezva.re4bridge.ControllerMapping;
import net.minecraft.util.hit.BlockHitResult;
import net.minecraft.util.hit.HitResult;
import com.mojang.brigadier.arguments.DoubleArgumentType;
import com.mojang.brigadier.arguments.IntegerArgumentType;
import java.io.IOException;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicBoolean;
import net.fabricmc.api.ClientModInitializer;
import net.fabricmc.fabric.api.client.command.v2.ClientCommandRegistrationCallback;
import net.fabricmc.fabric.api.client.event.lifecycle.v1.ClientTickEvents;
import net.fabricmc.fabric.api.event.player.UseBlockCallback;
import net.fabricmc.fabric.api.event.player.AttackBlockCallback;
import net.minecraft.item.BlockItem;
import net.minecraft.item.ItemPlacementContext;
import net.minecraft.block.BlockState;
import net.minecraft.util.ActionResult;
import net.minecraft.util.Hand;
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

/** Local editor bridge; temporary support is restricted to empty creative-world cells. */
public final class Re4BridgeClient implements ClientModInitializer {
    private BridgeConfig config;
    private net.minecraft.client.option.KeyBinding stopKey;
    private dev.dezva.re4bridge.PrepareServer prepareServer;
    private java.util.concurrent.CompletableFuture<String> prepareFuture;private long prepareStarted;private boolean prepareArming;
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
    private final ControllerInput controllerInput=new ControllerInput();
    private boolean minecraftWalking=true;
    private boolean synchronizedMovement=true;
    private boolean editorSupport;
    private boolean mapEditing;
    private boolean hudEnabled=true;
    private boolean blockDepth=true;
    private final MinecraftHud minecraftHud=new MinecraftHud();
    private ControllerMapping controllerMapping;
    private ControllerMapping.Point lastEditorPoint;
    private String controllerDimension;
    private ControllerFollower controllerFollower=new ControllerFollower();
    private final MouseLook mouseLook = new MouseLook();
    private boolean mouseEnabled;
    private boolean firstPersonEnabled;
    private double eyeHeight = 1620;
    private BlockPreview blockPreview;
    private BlockPos previewBlock;
    private String previewDimension;
    private BlockState previewState;
    private boolean blocksEnabled;
    private double blockSize = BlockPreview.DEFAULT_SIZE;
    private final BlockPlacementTracker placementTracker = new BlockPlacementTracker();
    private final BlockMaterials blockMaterials=new BlockMaterials();
    private final PlacedBlockScene placedScene = new PlacedBlockScene();
    private long placementEvents, placementTicks, placementsConfirmed;

    @Override public void onInitializeClient() {
        stopKey=net.fabricmc.fabric.api.client.keybinding.v1.KeyBindingHelper.registerKeyBinding(new net.minecraft.client.option.KeyBinding("key.re4bridge.stop",org.lwjgl.glfw.GLFW.GLFW_KEY_F8,"key.categories.misc"));
        try {
            config = BridgeConfig.loadOrCreate(FabricLoader.getInstance().getConfigDir().resolve("re4bridge.json"));
        } catch (IOException e) {
            // Keep Minecraft playable when configuration is broken; report through /re4bridge.
            config = null;
        }
        AttackBlockCallback.EVENT.register((player,world,hand,pos,direction)->{
            if(world.isClient && editorSupport)controllerFollower.claim(pos);
            return ActionResult.PASS;
        });
        ClientCommandRegistrationCallback.EVENT.register((dispatcher, access) -> dispatcher.register(
            literal("re4bridge")
                .then(literal("connect").executes(ctx -> connect(ctx.getSource().getClient(), false)))
                .then(literal("re4").executes(ctx -> connect(ctx.getSource().getClient(), true)))
                .then(literal("controller").executes(ctx -> connectController(ctx.getSource().getClient())))
                .then(literal("hud")
                    .then(literal("on").executes(ctx -> hud(ctx.getSource().getClient(),true)))
                    .then(literal("off").executes(ctx -> hud(ctx.getSource().getClient(),false))))
                .then(literal("map")
                    .then(literal("on").executes(ctx -> editMap(ctx.getSource().getClient(),true)))
                    .then(literal("off").executes(ctx -> editMap(ctx.getSource().getClient(),false))))
                .then(literal("sync")
                    .then(literal("on").executes(ctx -> synchronization(ctx.getSource().getClient(),true)))
                    .then(literal("off").executes(ctx -> synchronization(ctx.getSource().getClient(),false))))
                .then(literal("walking")
                    .then(literal("on").executes(ctx -> walking(ctx.getSource().getClient(),true)))
                    .then(literal("off").executes(ctx -> walking(ctx.getSource().getClient(),false))))
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
                .then(literal("arm").executes(ctx -> arm(ctx.getSource().getClient())))
                .then(literal("stop").executes(ctx -> {
                    stopDriving();
                    say(ctx.getSource().getClient(), "Control RE4 desarmado.");
                    return 1;
                }))
                .then(literal("calibrate").executes(ctx -> calibrate(ctx.getSource().getClient())))
                .then(literal("blocks")
                    .then(literal("depth")
                        .then(literal("on").executes(ctx -> {blockDepth=true;say(ctx.getSource().getClient(),"Profundidad RE4 activada; proyeccion y buffer deben validarse.");return 1;}))
                        .then(literal("off").executes(ctx -> {blockDepth=false;say(ctx.getSource().getClient(),"Profundidad desactivada: modo de diagnostico superpuesto.");return 1;})))
                    .then(literal("collisions")
                        .then(literal("on").executes(ctx -> blockCollisions(ctx.getSource().getClient(),true)))
                        .then(literal("off").executes(ctx -> blockCollisions(ctx.getSource().getClient(),false))))
                    .then(literal("style")
                        .then(literal("minecraft").executes(ctx -> blockStyle(ctx.getSource().getClient(),3)))
                        .then(literal("stone").executes(ctx -> blockStyle(ctx.getSource().getClient(),2)))
                        .then(literal("solid").executes(ctx -> blockStyle(ctx.getSource().getClient(),1)))
                        .then(literal("wire").executes(ctx -> blockStyle(ctx.getSource().getClient(),0))))
                    .then(literal("height").then(argument("offset",DoubleArgumentType.doubleArg(-2500,2500)).executes(ctx -> {
                        placedScene.height(DoubleArgumentType.getDouble(ctx,"offset"));
                        say(ctx.getSource().getClient(),"Altura de la cuadricula ajustada. Usa arm para verla.");return 1;
                    })))
                    .then(literal("on").executes(ctx -> enableBlocks(ctx.getSource().getClient(),BlockPreview.DEFAULT_SIZE))
                        .then(argument("size",DoubleArgumentType.doubleArg(40,2500)).executes(ctx ->
                            enableBlocks(ctx.getSource().getClient(),DoubleArgumentType.getDouble(ctx,"size")))))
                    .then(literal("off").executes(ctx -> {
                        blocksEnabled=false;clearBlock();
                        say(ctx.getSource().getClient(),"Construccion visual desactivada.");return 1;
                    })))
                .then(literal("blocktest").executes(ctx -> blockTest(ctx.getSource().getClient(),BlockPreview.DEFAULT_SIZE))
                    .then(literal("off").executes(ctx -> {
                        clearBlock();
                        say(ctx.getSource().getClient(),"Bloque visual retirado.");return 1;
                    }))
                    .then(argument("size",DoubleArgumentType.doubleArg(40,2500))
                        .executes(ctx -> blockTest(ctx.getSource().getClient(),DoubleArgumentType.getDouble(ctx,"size")))))
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
                        if (bridge.controllerMode()) ctx.getSource().sendFeedback(Text.literal(
                            "Bloque visual: estado="+bridge.previewStatus()+"; frames="+bridge.previewFrames()+"; dibujos="+bridge.previewDraws()
                            +"; fallos="+bridge.previewFailures()+"; HRESULT=0x"+Long.toHexString(bridge.previewHresult())
                            +"; solicitado al host="+bridge.previewRequested()+"; pases omitidos="+bridge.previewSkipped()
                            +"; motivo="+bridge.previewReason()+". Construccion="+blocksEnabled));
                        if (bridge.controllerMode()) ctx.getSource().sendFeedback(Text.literal(
                            "Deteccion: eventos="+placementEvents+"; sondeos="+placementTicks+"; confirmados="+placementsConfirmed
                            +"; pendientes="+placementTracker.size()+"; seleccionado="+(blockPreview!=null || placedScene.size()>0)+"; bloques="+placedScene.size()+"; cajas enviadas="+bridge.previewBoxCount()+"; modo="+blockStyleName(bridge.previewStyle())+"; colision nativa="+bridge.collisionActive()+"; contactos="+bridge.collisionContacts()+"; correcciones="+bridge.collisionCorrections()+"; tamano="+(int)blockSize+"; HUD="+(hudEnabled && bridge.hudAvailable())+"; Ultimos ID HUD="+bridge.nativeHudHidden()+"; HUD RE4="+((bridge.nativeCapabilities()&16)!=0?"ocultacion disponible":"firma no disponible")+"; referencia="+(controllerMapping!=null)+"; mapa auxiliar="+mapEditing+"; sync="+synchronizedMovement+"; movimiento independiente="+minecraftWalking+"; caras sin textura="+blockMaterials.missing()+"; error material="+blockMaterials.lastError()));
                        if(bridge.controllerMode()) {
                            ctx.getSource().sendFeedback(Text.literal(minecraftHud.status()));
                            ctx.getSource().sendFeedback(Text.literal("Mapa RE4: "+bridge.terrainState()+"; profundidad: "+bridge.depthReason()+"; dibujos="+bridge.depthDraws()+"; dibujos sin profundidad="+bridge.depthFallbackDraws()));
                        }
                        var sample = bridge.re4Sample();
                        ctx.getSource().sendFeedback(Text.literal(sample != null && sample.fresh(System.nanoTime())
                            ? String.format(java.util.Locale.ROOT, "RE4 raw: %.2f %.2f %.2f; angulo: %.3f; escala: %.1f; follow: %s",
                                sample.x(), sample.y(), sample.z(), sample.yawRadians(), unitsPerBlock, following)
                            : "RE4: sin personaje o muestra reciente; seguimiento suspendido."));
                    }
                    return 1;
                }))));
        if(config!=null)try{prepareServer=new dev.dezva.re4bridge.PrepareServer(config,future -> MinecraftClient.getInstance().execute(() -> prepare(MinecraftClient.getInstance(),future)));}catch(IOException error){System.err.println("RE4 Bridge: endpoint VIEW no disponible: "+error.getClass().getSimpleName());}
        net.fabricmc.fabric.api.client.event.lifecycle.v1.ClientLifecycleEvents.CLIENT_STOPPING.register(client -> {if(prepareServer!=null)prepareServer.close();});
        OverlayCapture.renderer=ticks -> minecraftHud.render(MinecraftClient.getInstance(),ticks);
        net.fabricmc.fabric.api.client.event.lifecycle.v1.ClientLifecycleEvents.CLIENT_STOPPING.register(client -> minecraftHud.close());
        ClientTickEvents.START_CLIENT_TICK.register(this::beforeTick);
        ClientTickEvents.END_CLIENT_TICK.register(this::tick);
        UseBlockCallback.EVENT.register((player,world,hand,hit) -> {
            MinecraftClient client=MinecraftClient.getInstance();
            if (world.isClient && player==client.player) capturePlacement(client,hand,hit,true);
            return ActionResult.PASS;
        });
    }

    private int hud(MinecraftClient client,boolean enabled){
        hudEnabled=enabled;say(client,"Interfaz de Minecraft: "+(enabled?"activada":"desactivada")+". La vida y municion de RE4 se ocultan automaticamente con arm y primera persona; F8 vuelve a RE4 normal.");return 1;
    }
    private int editMap(MinecraftClient client,boolean enabled){
        stopDriving();mapEditing=enabled;
        say(client,enabled?"Mapa auxiliar: camina y coloca cristal/suelo en Minecraft; estas nuevas colocaciones no se envian a RE4. map off termina.":"Mapa auxiliar terminado. Usa arm; se conserva la referencia y tu construccion.");return 1;
    }
    private ControllerMapping.Point editorPoint(MinecraftClient client,dev.dezva.re4bridge.Re4Sample sample){
        var point=firstPersonEnabled?controllerMapping.eye(sample,eyeHeight,placedScene.height()):controllerMapping.toMinecraft(sample);
        double height=firstPersonEnabled?client.player.getEyeHeight(client.player.getPose()):placedScene.height()/controllerMapping.scale();
        return new ControllerMapping.Point(point.x(),point.y()-height,point.z());
    }
    private int synchronization(MinecraftClient client,boolean enabled){
        if(enabled && controllerMapping==null && placedScene.anchored()){say(client,"Usa blocks on para iniciar una cuadricula sincronizada nueva.");return 0;}
        stopDriving();synchronizedMovement=enabled;
        say(client,enabled?"Steve sigue a Leon con referencia fija. Requiere creativo; usa arm.":"Seguimiento desactivado; las posiciones pueden separarse. Usa arm.");return 1;
    }
    private int walking(MinecraftClient client,boolean enabled){
        stopDriving();minecraftWalking=enabled;
        say(client,enabled?"Movimiento independiente habilitado cuando sync esta off. Usa arm.":"Movimiento independiente desactivado cuando sync esta off. Usa arm.");return 1;
    }

    private void beforeTick(MinecraftClient client) {
        while(stopKey.wasPressed()) {
            stopDriving();
            if(prepareFuture!=null){prepareFuture.completeExceptionally(new IllegalStateException("Preparacion cancelada con F8."));prepareFuture=null;}
        }
        preparing(client);
        var current=bridge;
        var sample=current==null?null:current.re4Sample();
        boolean lock=(synchronizedMovement || !minecraftWalking) && (controlArming.active() || controlArming.pending())
            && current!=null && current.controllerMode() && current.status().connected()
            && sample!=null && sample.fresh(System.nanoTime()) && client.isInSingleplayer()
            && (!synchronizedMovement || client.player!=null && client.player.isCreative())
            && (client.currentScreen==null || controlArming.active() && firstPersonEnabled && hudEnabled && current.overlayAvailable() && MinecraftHud.inventory(client)) && !client.isPaused() && client.isWindowFocused();
        controllerInput.update(client.player,lock);
        if(lock && synchronizedMovement && controllerMapping!=null && client.player!=null && client.player.isCreative()){
            try {
                var point=editorPoint(client,sample);
                if(!client.world.getRegistryKey().getValue().toString().equals(controllerDimension) || lastEditorPoint!=null && (Math.abs(point.x()-lastEditorPoint.x())>8 || Math.abs(point.y()-lastEditorPoint.y())>8 || Math.abs(point.z()-lastEditorPoint.z())>8))throw new IllegalArgumentException("Salto de zona");
                controllerFollower.follow(client,point,current,editorSupport);if(!mouseEnabled)client.player.setYaw(controllerMapping.minecraftYaw(sample.yawRadians()));lastEditorPoint=point;
            }catch(IllegalArgumentException error){stopDriving();say(client,"Sincronizacion detenida: salto de zona. Usa blocks on para fijar otra referencia.");}
        }else controllerFollower.release();
        // START runs before vanilla processes item use. Observe before prediction,
        // even on the tick that closes arm's chat; publishing remains arm-gated.
        if (client.options.useKey.isPressed() && client.crosshairTarget instanceof BlockHitResult hit
                && hit.getType()==HitResult.Type.BLOCK) {
            capturePlacement(client,Hand.MAIN_HAND,hit,false);
            capturePlacement(client,Hand.OFF_HAND,hit,false);
        }
    }
    private int arm(MinecraftClient client){
        if(mapEditing){say(client,"Primero map off: el mapa auxiliar no se transfiere a RE4.");return 0;}
        var current = bridge;
        var sample = current == null ? null : current.re4Sample();
        if (current == null || !current.controllerMode() || !current.status().connected()
                || sample == null || !sample.fresh(System.nanoTime()) || current.nativeTicks() == 0) {
            say(client, "Conecta /re4bridge controller con RE4 cargado y el modulo nativo activo.");
            return 0;
        }
        if(synchronizedMovement && !client.player.isCreative()){say(client,"Sincronizacion de construccion requiere creativo. Usa gamemode creative o sync off.");return 0;}
        if(synchronizedMovement){
            if(controllerMapping==null){
                var pos=client.player.getPos();
                double baseY=pos.y;
                if(blocksEnabled){
                    try{baseY=EditorFloor.anchorHeight(pos.y,client.world.getTopY(net.minecraft.world.Heightmap.Type.MOTION_BLOCKING,client.player.getBlockX(),client.player.getBlockZ()),client.world.getTopY());}
                    catch(IllegalArgumentException error){say(client,error.getMessage());return 0;}
                }
                controllerMapping=ControllerMapping.anchor(sample,pos.x,baseY,pos.z,client.player.getYaw(),blockSize);
                editorSupport=blocksEnabled;
                if(editorSupport)say(client,"Suelo temporal de cristal en Minecraft; no se envia a RE4 ni mueve construcciones.");
                controllerDimension=client.world.getRegistryKey().getValue().toString();
            }
            client.player.setYaw(controllerMapping.minecraftYaw(sample.yawRadians()));
            lastEditorPoint=editorPoint(client,sample);
        }
        following = false;
        controlArming.request(System.nanoTime());
        mouseLook.reset();
        current.publishControl(false, 0, controlArming.generation());
        say(client, "Cierra el chat para activar control. Steve sigue a Leon (sync on). F8 devuelve el movimiento normal de Minecraft.");
        return 1;
        }
    private void prepare(MinecraftClient client,java.util.concurrent.CompletableFuture<String> future){
        if(future.isCancelled())return;
        if(prepareFuture!=null){future.completeExceptionally(new IllegalStateException("Preparacion ya en curso."));return;}
        if(client.player==null || client.world==null || !client.isInSingleplayer() || !client.player.isCreative() || client.currentScreen!=null || !client.isWindowFocused()){
            future.completeExceptionally(new IllegalStateException("Abre un mundo local en creativo, cierra chat/inventario y pulsa Preparar Minecraft."));return;
        }
        if(bridge==null || !bridge.controllerMode() || !bridge.status().connected())connect(client,BridgeConnection.Mode.CONTROL);else stopDriving();prepareFuture=future;prepareStarted=System.nanoTime();prepareArming=false;
    }
    private void preparing(MinecraftClient client){
        var future=prepareFuture;if(future==null)return;
        if(future.isCancelled() || System.nanoTime()-prepareStarted>10_000_000_000L || client.player==null || client.world==null || !client.isWindowFocused() || client.currentScreen!=null){
            stopDriving();future.completeExceptionally(new IllegalStateException("Preparacion detenida: revisa partida RE4, host y foco de Minecraft."));prepareFuture=null;return;
        }
        var current=bridge;if(current==null)return;
        if(!current.status().connected()){if(current.status().message().startsWith("connection failed")){future.completeExceptionally(new IllegalStateException("Abre START-RE4-CONTROLS y una partida de RE4."));prepareFuture=null;}return;}
        if(!prepareArming){
            if(current.re4Sample()==null || !current.re4Sample().fresh(System.nanoTime()) || current.nativeTicks()==0)return;
            if((current.nativeCapabilities()&3)!=3 || !current.overlayAvailable() || !current.depthAvailable() || !current.sceneAvailable() || !current.materialsAvailable() || !current.collisionAvailable()){future.completeExceptionally(new IllegalStateException("Instala el paquete nuevo completo; primera persona/interfaz/profundidad no disponibles."));prepareFuture=null;return;}
            synchronizedMovement=true;mouseEnabled=true;firstPersonEnabled=true;hudEnabled=true;mapEditing=false;blockDepth=true;
            if(!blocksEnabled)enableBlocks(client,BlockPreview.DEFAULT_SIZE);
            placedScene.style(3);placedScene.collisions(true);
            if(arm(client)==0){future.completeExceptionally(new IllegalStateException("No se pudo armar el editor."));prepareFuture=null;return;}prepareArming=true;
        }else if(controlArming.active() && current.nativeArmed() && current.cameraActive()){
            future.complete("Minecraft preparado. Pulsa F9 para jugar; F8 vuelve a RE4 normal.");prepareFuture=null;
        }
    }

    private void capturePlacement(MinecraftClient client,Hand hand,BlockHitResult hit,boolean event) {
        var current=bridge;
        var sample=current==null?null:current.re4Sample();
        if (mapEditing || !blocksEnabled || client.world==null || client.player==null || client.player.isSpectator()
                || !client.isInSingleplayer() || client.currentScreen!=null || client.isPaused() || !client.isWindowFocused()
                || current==null || !current.controllerMode() || !current.status().connected()
                || sample==null || !sample.fresh(System.nanoTime())
                || !(client.player.getStackInHand(hand).getItem() instanceof BlockItem item)) return;
        try {
            var box=BlockPreview.ahead(sample,blockSize);
            var dimension=client.world.getRegistryKey().getValue().toString();
            var context=new ItemPlacementContext(client.player,hand,client.player.getStackInHand(hand),hit);
            // Include direct replacement and adjacent placement; confirmation still
            // requires a real change to the expected material, never just a click.
            for (var pos : new BlockPos[]{context.getBlockPos(),hit.getBlockPos(),hit.getBlockPos().offset(hit.getSide())}) {
                var placedBox=box;
                if(controllerMapping!=null){var center=controllerMapping.toNative(pos.getX()+.5,pos.getY(),pos.getZ()+.5);placedBox=new BlockPreview(center.x(),center.y(),center.z(),blockSize);}
                placementTracker.attempt(pos,client.world.getBlockState(pos),item.getBlock(),placedBox,dimension);
            }
            if (event) placementEvents++; else placementTicks++;
        } catch (IllegalArgumentException error) { /* Invalid RE4 coordinates create no selection. */ }
    }

    private java.util.List<Box> sceneShapes(MinecraftClient client,BlockPos pos) {
        var state=client.world.getBlockState(pos);
        return state.getCollisionShape(client.world,pos,ShapeContext.absent()).getBoundingBoxes();
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
        bridge.overlaySource(minecraftHud::latest);
        bridge.start();
        say(client, mode != BridgeConnection.Mode.SIMULATOR ? "Conectando a RE4. Usa /re4bridge status."
            : "Conectando al simulador local. RE4 todavía no está conectado. Usa /re4bridge status.");
        return 1;
    }

    private void disconnect(MinecraftClient client) {
        stopDriving();TerrainInteraction.clear();controllerFollower=new ControllerFollower();
        mouseEnabled = false; firstPersonEnabled = false;
        blocksEnabled=false;clearBlock();
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
        controllerInput.release();controllerFollower.release();
        controlArming.stop("desarmado");
        placementTracker.clear();
        mouseLook.reset();
        if (bridge != null && bridge.controllerMode()) bridge.publishControl(false, 0, controlArming.generation());
    }

    private void clearBlock() {
        controllerMapping=null;controllerDimension=null;lastEditorPoint=null;editorSupport=false;
        blockPreview=null;previewBlock=null;previewDimension=null;previewState=null;placementTracker.clear();placedScene.clear();blockMaterials.clear();
    }

    private int enableBlocks(MinecraftClient client,double size) {
        if (bridge==null || !bridge.controllerMode() || !bridge.status().connected() || !bridge.sceneAvailable()) {
            say(client,"Bloques multiples requieren host y DLL 0.4.21. Conecta controller despues de instalarlos.");return 0;
        }
        stopDriving();clearBlock();blocksEnabled=true;blockSize=size;
        placementEvents=placementTicks=placementsConfirmed=0;
        placedScene.style(bridge.materialsAvailable()?3:bridge.textureAvailable()?2:bridge.surfaceAvailable()?1:0);
        say(client,"Cuadricula visual activa, tamano="+(int)size+". Usa arm y coloca bloques: la primera colocacion fija la referencia. Maximo 64 bloques. Modo="+blockStyleName(placedScene.style())+". F8 detiene.");
        return 1;
    }

    private static String blockStyleName(int style) {return style==3?"Minecraft":style==2?"piedra":style==1?"caras":"aristas";}

    private int blockCollisions(MinecraftClient client,boolean enabled) {
        if(enabled && (bridge==null || !bridge.collisionAvailable())){say(client,"Colision requiere host y DLL 0.4.24.");return 0;}
        stopDriving();placedScene.collisions(enabled);say(client,"Paredes para Leon: "+(enabled?"activadas":"desactivadas")+". Usa arm. Saltos/apoyo encima y enemigos pendientes.");return 1;
    }
    private int blockStyle(MinecraftClient client,int style) {
        if(style==3 && (bridge==null || !bridge.materialsAvailable())){say(client,"Texturas Minecraft requieren host y DLL 0.4.24.");return 0;}

        if (style==2 && (bridge==null || !bridge.textureAvailable())) {say(client,"Textura requiere host y DLL 0.4.23 conectados.");return 0;}
        if (style==1 && (bridge==null || !bridge.surfaceAvailable())) {
            say(client,"Caras rellenas requieren host y DLL 0.4.22 conectados.");return 0;
        }
        placedScene.style(style);
        say(client,"Modo visual: "+blockStyleName(style)+". Usa arm para verlo.");return 1;
    }

    private int blockTest(MinecraftClient client,double size) {
        var current=bridge;
        var sample=current==null?null:current.re4Sample();
        if (client.world==null || current==null || !current.controllerMode()
                || !current.status().connected() || !current.previewAvailable() || sample==null || !sample.fresh(System.nanoTime())
                || !(client.crosshairTarget instanceof BlockHitResult hit)
                || hit.getType()!=HitResult.Type.BLOCK || client.world.getBlockState(hit.getBlockPos()).isAir()) {
            say(client,"Conecta controller y mira un bloque colocado en Minecraft antes de blocktest.");return 0;
        }
        blocksEnabled=false;placedScene.clear();
        try {blockPreview=BlockPreview.ahead(sample,size);}
        catch (IllegalArgumentException error) {say(client,"Posicion RE4 fuera del rango de prueba.");return 0;}
        previewBlock=hit.getBlockPos().toImmutable();
        previewDimension=client.world.getRegistryKey().getValue().toString();
        previewState=client.world.getBlockState(previewBlock);
        placementTracker.clear();
        say(client,"Cubo de lineas fijado delante de Leon, tamano="+(int)size+". Usa arm; romper este bloque lo retira. F8 detiene.");
        return 1;
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
        if (client.player == null || client.world == null || server == null || !client.isInSingleplayer()) {
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
                : client.currentScreen instanceof ChatScreen ? ControlArming.Screen.CHAT : firstPersonEnabled && hudEnabled && current.overlayAvailable() && MinecraftHud.inventory(client) ? ControlArming.Screen.INVENTORY : ControlArming.Screen.OTHER;
            controlArming.tick(now, client.isPaused(), client.isWindowFocused(), screen,
                sample != null && sample.fresh(now), current.nativeArmed(), current.nativeStop());
            if(controlArming.active() && synchronizedMovement && !client.player.isCreative()){stopDriving();say(client,"Sincronizacion detenida al salir de creativo.");}
            if(!controlArming.active()){controllerInput.release();controllerFollower.release();}
            int buttons = 0;
            if (controlArming.active() && screen==ControlArming.Screen.NONE) {
                if (client.options.forwardKey.isPressed()) buttons |= 1;
                if (client.options.backKey.isPressed()) buttons |= 2;
                if (client.options.rightKey.isPressed()) buttons |= 4;
                if (client.options.leftKey.isPressed()) buttons |= 8;
                if (buttons != 0 && client.options.sprintKey.isPressed()) buttons |= 64;
            }
            if(controlArming.active() && synchronizedMovement && controllerMapping!=null){
                try{controllerFollower.follow(client,editorPoint(client,sample),current,editorSupport);}catch(IllegalArgumentException error){stopDriving();buttons=0;}
            }
            MouseLook.Input look = null;
            if (controlArming.active() && mouseEnabled) {
                try { look = firstPersonEnabled && synchronizedMovement && controllerMapping!=null && (current.nativeCapabilities()&8)!=0
                    ? controllerMapping.look(client.player.getYaw(),client.player.getPitch(),eyeHeight)
                    : mouseLook.sample(client.player.getYaw(), client.player.getPitch(), firstPersonEnabled, eyeHeight); }
                catch (IllegalArgumentException error) { controlArming.stop("orientacion invalida"); buttons = 0; mouseLook.reset(); }
            } else mouseLook.reset();
            String dimension=client.world.getRegistryKey().getValue().toString();
            if (!placedScene.update(client.world::getBlockState,pos -> sceneShapes(client,pos),client.world::isChunkLoaded,dimension))
                say(client,"Cuadricula retirada: demasiadas formas. Usa blocks on para empezar otra mas pequena.");
            if (sample==null || !sample.fresh(now)) placementTracker.clear();
            for (var placed:placementTracker.pollAll(client.world::getBlockState,dimension)) {
                if(controllerFollower.helper(placed.pos()))continue;
                boolean first=!placedScene.anchored();
                if (placedScene.put(placed,sceneShapes(client,placed.pos()),client.player.getYaw())) {
                    if(first){if(controllerMapping!=null)placedScene.rotation(controllerMapping.rotation());else placedScene.align(sample.yawRadians());}
                    placementsConfirmed++;
                    say(client,"Bloque colocado detectado. Cuadricula: "+placedScene.size()+" bloques.");
                } else say(client,"Bloque no agregado: limite de bloques/formas o forma fuera del rango permitido.");
            }
            if (blockPreview!=null && (previewBlock==null
                    || !client.world.getRegistryKey().getValue().toString().equals(previewDimension)
                    || !client.world.getBlockState(previewBlock).equals(previewState))) {
                clearBlock();
            }
            if(placedScene.style()==3)blockMaterials.resources(client);
            var scene=placedScene.snapshot((pos,state) -> blockMaterials.faces(client,pos,state),blockMaterials);
            dev.dezva.re4bridge.HudFrame hud=null;
            if(controlArming.active() && firstPersonEnabled && hudEnabled && current.hudAvailable()){
                try{hud=minecraftHud.snapshot(client,current.overlayWidth(),current.overlayHeight());
                    if(hud!=null && controllerMapping!=null && !MinecraftHud.inventory(client) && client.crosshairTarget instanceof BlockHitResult hit && hit.getType()==HitResult.Type.BLOCK){
                        var shape=client.world.getBlockState(hit.getBlockPos()).getOutlineShape(client.world,hit.getBlockPos(),ShapeContext.of(client.player));
                        if(shape.isEmpty() && TerrainInteraction.surfaceTarget(hit))shape=net.minecraft.util.shape.VoxelShapes.fullCube();
                        if(!shape.isEmpty()){
                            var p=hit.getBlockPos();var bounds=shape.getBoundingBox();var origin=controllerMapping.toNative(p.getX(),p.getY(),p.getZ());double scale=controllerMapping.scale();
                            hud=hud.selection(new double[]{origin.x(),origin.y(),origin.z(),controllerMapping.rotation(),bounds.minX*scale,controllerFollower.helper(p)?bounds.maxY*scale-1:bounds.minY*scale,bounds.minZ*scale,bounds.maxX*scale,bounds.maxY*scale,bounds.maxZ*scale});
                        }
                    }}catch(IllegalArgumentException | IllegalStateException error){hudEnabled=false;stopDriving();buttons=0;say(client,"Interfaz detenida: "+error.getMessage()+". F8 devuelve la ventana de RE4.");}
            }
            boolean terrain=controlArming.active() && firstPersonEnabled && synchronizedMovement && controllerMapping!=null;
            current.terrainRequested(terrain);
            current.depthRequested(controlArming.active() && blockDepth);
            if(terrain)TerrainInteraction.bind(current,controllerMapping,controlArming.generation(),client.player.getUuid(),dimension,placedScene.positions());else TerrainInteraction.clear();
            if (scene!=null || hud!=null) current.publishSceneControl(controlArming.active(),buttons,controlArming.generation(),look,scene,hud);
            else current.publishControl(controlArming.active(), buttons, controlArming.generation(), look, blockPreview);
            if (++controlHudTicks % 5 == 0) client.player.sendMessage(Text.literal(
                "RE4: " + controlArming.reason() + " | arm=" + (current.nativeArmed() ? "si" : "no")
                + " | teclas=" + buttons + "/" + current.nativeButtons()
                + " | raton=" + (mouseEnabled ? Long.toString(current.nativeLookPackets()) : "off")
                + " | FP=" + (current.cameraActive() ? "si" : "no")
                + (firstPersonEnabled ? " | altura=" + (int)eyeHeight : "")
                + (blocksEnabled || blockPreview!=null ? " | bloques="+placedScene.size()+" | cajas="+current.previewBoxCount()+" | modo="+blockStyleName(placedScene.style())+" | col="+(current.collisionActive()?"si":"no")+" | choques="+current.collisionContacts()+" | sin tex="+blockMaterials.missing()
                    +" | enviado="+(current.previewRequested() ? "si" : "no")+" | dibujos="+current.previewDraws() : "")
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
