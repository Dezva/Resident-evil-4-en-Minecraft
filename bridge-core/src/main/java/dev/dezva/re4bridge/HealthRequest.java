package dev.dezva.re4bridge;
import com.google.gson.JsonObject;
public record HealthRequest(int generation,int mode,float ratio,long damageAck,long healingAck,long createdNs) {
    public HealthRequest {if(generation<=0 || mode<1 || mode>2 || !Float.isFinite(ratio) || ratio<0 || ratio>1 || damageAck<0 || damageAck>0xffffffffL || healingAck<0 || healingAck>0xffffffffL)throw new IllegalArgumentException("Invalid health request");}
    public JsonObject toJson(){var j=new JsonObject();j.addProperty("mode",mode);j.addProperty("ratio",ratio);j.addProperty("damage_ack",damageAck);j.addProperty("healing_ack",healingAck);return j;}
}
