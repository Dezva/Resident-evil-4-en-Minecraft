package dev.dezva.re4bridge.minecraft;

import net.minecraft.client.MinecraftClient;
import net.minecraft.client.gui.screen.Screen;

/** Runs on the client thread, before vanilla's GUI/perspective transitions. */
public final class BridgeKeys {
    static java.util.function.Consumer<Screen> screen=next->{};
    static java.util.function.BooleanSupplier mode=()->false;
    public static void screen(Screen next){screen.accept(next);}
    public static boolean key(MinecraftClient client,long window,int key,int action){
        return window==client.getWindow().getHandle() && client.isWindowFocused()
            && consumeModeKey(mode.getAsBoolean(),client.currentScreen!=null,key,action,toggle);
    }
    static boolean consumeModeKey(boolean managed,boolean menu,int key,int action,Runnable toggle){
        if(!managed || menu || key!=org.lwjgl.glfw.GLFW.GLFW_KEY_F5)return false;
        if(action==org.lwjgl.glfw.GLFW.GLFW_PRESS)toggle.run();
        return true; // repeat/release never cycle perspective or change mode again
    }
    static Runnable toggle=()->{};
    private BridgeKeys(){}
}
