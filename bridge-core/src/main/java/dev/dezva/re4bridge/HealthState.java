package dev.dezva.re4bridge;

import com.google.gson.JsonObject;
import java.io.IOException;

/** Native cumulative normalized HP deltas, not a transient hit notification. */
public record HealthState(boolean available,boolean active,boolean fresh,int reason,int generation,long player,int hp,int maximum,long damage,long healing,boolean dead,int mode) {
    public static final long UNIT=1_000_000L;
    public static HealthState parse(JsonObject j) throws IOException {
        try{
            boolean available=bool(j,"available");
            if(!available)return new HealthState(false,false,false,1,0,0,0,0,0,0,false,0);
            if(!j.has("generation"))return new HealthState(true,false,false,2,0,0,0,0,0,0,false,0);
            int hp=(int)number(j,"hp",32767),max=(int)number(j,"maximum",32767);
            if(hp>max)throw new IllegalArgumentException();
            return new HealthState(true,bool(j,"active"),bool(j,"fresh"),(int)number(j,"reason",5),(int)number(j,"generation",Integer.MAX_VALUE),number(j,"player",0xffffffffL),hp,max,number(j,"damage",0xffffffffL),number(j,"healing",0xffffffffL),bool(j,"dead"),(int)number(j,"mode",2));
        }catch(RuntimeException e){throw new IOException("Invalid native health state",e);}
    }
    private static boolean bool(JsonObject j,String key){var p=j.getAsJsonPrimitive(key);if(!p.isBoolean())throw new IllegalArgumentException();return p.getAsBoolean();}
    private static long number(JsonObject j,String key,long max){var p=j.getAsJsonPrimitive(key);long n=p.getAsLong();if(!p.isNumber() || n<0 || n>max || !p.getAsString().equals(Long.toString(n)))throw new IllegalArgumentException();return n;}
    public static long delta(long current,long acknowledged){long result=(current-acknowledged)&0xffffffffL;if(result>100*UNIT)throw new IllegalArgumentException("Health counter discontinuity");return result;}
}
