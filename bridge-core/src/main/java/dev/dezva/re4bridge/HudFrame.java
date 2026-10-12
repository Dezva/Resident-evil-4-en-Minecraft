package dev.dezva.re4bridge;
import com.google.gson.JsonObject;
import java.util.Base64;
/** Bounded transparent hotbar, composed only from locally loaded resources. */
public record HudFrame(String image) {
    public static final int WIDTH=256,HEIGHT=32,BYTES=WIDTH*HEIGHT*4;
    public HudFrame {if(image==null || image.length()!=43692 || Base64.getDecoder().decode(image).length!=BYTES)throw new IllegalArgumentException("Invalid HUD pixels");}
    public JsonObject toJson(){var object=new JsonObject();object.addProperty("image",image);return object;}
}
