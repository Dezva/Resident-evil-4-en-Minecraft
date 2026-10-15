package dev.dezva.re4bridge;
import com.google.gson.JsonObject;
import java.io.IOException;
public record RuntimeState(boolean available,boolean fresh,int scene,long flags,boolean enemiesAvailable,boolean actionAvailable,long contacts,long corrections,long actions,long receivedNs,int enemyReason,int scanned,int eligible,long skips,boolean dialogActive,long dialogActions,int actionAcknowledgement,int actionGeneration) {
    public boolean dialog(long now){return available && fresh && dialogActive && now>=receivedNs && now-receivedNs<500_000_000L;}
    public boolean scripted(long now){return available && fresh && scene==1 && now>=receivedNs && now-receivedNs<500_000_000L;}
    public static RuntimeState parse(JsonObject j,long now) throws IOException {
        try{
            if(!bool(j,"available") || !bool(j,"fresh"))return new RuntimeState(j.get("available").getAsBoolean(),false,2,0,false,false,0,0,0,now,1,0,0,0,false,0,0,0);
            return new RuntimeState(true,true,(int)number(j,"scene",2),number(j,"flags",0xffffffffL),bool(j,"enemies_available"),bool(j,"action_available"),number(j,"enemy_contacts",0xffffffffL),number(j,"enemy_corrections",0xffffffffL),number(j,"actions",0xffffffffL),now,(int)optional(j,"enemy_reason",6),(int)optional(j,"enemy_scanned",4096),(int)optional(j,"enemy_eligible",4096),optional(j,"skips",0xffffffffL),j.has("dialog_active") && bool(j,"dialog_active"),optional(j,"dialog_actions",0xffffffffL),(int)optional(j,"action_ack",2147483647),(int)optional(j,"action_generation",2147483647));
        }catch(RuntimeException e){throw new IOException("Invalid native runtime state",e);}
    }
    private static long optional(JsonObject j,String key,long max){return j.has(key)?number(j,key,max):0;}
    private static boolean bool(JsonObject j,String key){var p=j.getAsJsonPrimitive(key);if(!p.isBoolean())throw new IllegalArgumentException();return p.getAsBoolean();}
    private static long number(JsonObject j,String key,long max){var p=j.getAsJsonPrimitive(key);long value=p.getAsLong();if(!p.isNumber() || value<0 || value>max || !p.getAsString().equals(Long.toString(value)))throw new IllegalArgumentException();return value;}
}
