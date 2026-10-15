package dev.dezva.re4bridge.minecraft;

import net.minecraft.client.MinecraftClient;
import net.minecraft.client.gui.screen.Screen;

/** Runs on the client thread, before vanilla's GUI/perspective transitions. */
public final class BridgeKeys {
    static java.util.function.Consumer<Screen> screen=next->{};
    static java.util.function.Consumer<Boolean> focus=focused->{};
    static java.util.function.BooleanSupplier mode=()->false;
    static java.util.function.BooleanSupplier scripted=()->false;
    static Runnable interact=()->{},skip=()->{};
    static java.util.function.BooleanSupplier dialog=()->false;
    static java.util.function.IntConsumer navigate=kind->{};
    public static void screen(Screen next){screen.accept(next);}
    public static void focus(boolean focused){focus.accept(focused);}
    public static boolean key(MinecraftClient client,long window,int key,int action){
        if(window!=client.getWindow().getHandle() || !client.isWindowFocused() || !mode.getAsBoolean() || client.currentScreen!=null)return false;
        if(consumeDialogKey(dialog.getAsBoolean(),key,action,navigate))return true;
        if(consumeActionKey(key,action,interact,skip))return true;
        return consumeModeKey(true,false,key,action,scripted.getAsBoolean()?()->{}:toggle);
    }
    public static void input(MinecraftClient client){
        if(!mode.getAsBoolean() || !scripted.getAsBoolean())return;
        client.options.attackKey.setPressed(false);client.options.useKey.setPressed(false);
        while(client.options.attackKey.wasPressed()){}
        while(client.options.useKey.wasPressed()){}
    }
    static int dialogKind(int key){
        return switch(key){case 265->2;case 264->3;case 263->4;case 262->5;case 257,335,82->6;case 259->7;default->0;};
    }
    private static long repeatedAt;private static int repeatedKey;
    static boolean consumeDialogKey(boolean active,int key,int action,java.util.function.IntConsumer send){
        int kind=dialogKind(key);if(!active || kind==0)return false;
        long now=System.nanoTime();
        if(action==org.lwjgl.glfw.GLFW.GLFW_PRESS || action==org.lwjgl.glfw.GLFW.GLFW_REPEAT && kind<6 && (repeatedKey!=key || now-repeatedAt>=150_000_000L)){
            repeatedAt=now;repeatedKey=key;send.accept(kind);
        }
        return true;
    }
    static boolean consumeActionKey(int key,int action,Runnable interact,Runnable skip){
        if(key!=org.lwjgl.glfw.GLFW.GLFW_KEY_R && key!=org.lwjgl.glfw.GLFW.GLFW_KEY_F6)return false;
        if(action==org.lwjgl.glfw.GLFW.GLFW_PRESS)(key==org.lwjgl.glfw.GLFW.GLFW_KEY_R?interact:skip).run();
        return true;
    }
    static boolean consumeModeKey(boolean managed,boolean menu,int key,int action,Runnable toggle){
        if(!managed || menu || key!=org.lwjgl.glfw.GLFW.GLFW_KEY_F5)return false;
        if(action==org.lwjgl.glfw.GLFW.GLFW_PRESS)toggle.run();
        return true; // repeat/release never cycle perspective or change mode again
    }
    static Runnable toggle=()->{};
    private BridgeKeys(){}
}
