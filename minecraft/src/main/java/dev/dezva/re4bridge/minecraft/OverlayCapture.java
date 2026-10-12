package dev.dezva.re4bridge.minecraft;
/** The vanilla blur targets Minecraft's main framebuffer, never our transparent layer. */
public final class OverlayCapture {
    static java.util.function.Consumer<net.minecraft.client.render.RenderTickCounter> renderer=ticks->{};
    public static void render(net.minecraft.client.render.RenderTickCounter ticks){renderer.accept(ticks);}
private OverlayCapture(){}public static boolean active(){return MinecraftHud.capturing;}public static boolean target(Object framebuffer){return framebuffer==MinecraftHud.activeTarget;}}
