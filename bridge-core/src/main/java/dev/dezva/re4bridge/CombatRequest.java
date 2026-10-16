package dev.dezva.re4bridge;
import com.google.gson.*;

/** One bounded swing or one tick of an owned arrow. 50 native HP per damage point. */
public record CombatRequest(int generation,int id,int slot,long guid,int damage,double scale,double reach,TerrainSnapshot.Point start,TerrainSnapshot.Point end,long createdNs,int kind,int projectile) {
 public CombatRequest(int generation,int id,int slot,long guid,int damage,double scale,double reach,TerrainSnapshot.Point start,TerrainSnapshot.Point end,long createdNs){this(generation,id,slot,guid,damage,scale,reach,start,end,createdNs,0,0);}
 public CombatRequest {
  if(generation<1 || id<1 || slot<0 || slot>4095 || guid<0 || guid>0xffffffffL || damage<1 || damage>50000 || !Double.isFinite(scale+reach) || scale<40 || scale>2500 || reach<=0 || reach>6 || start==null || end==null)throw new IllegalArgumentException("Invalid melee request");
  for(var p:new TerrainSnapshot.Point[]{start,end})for(int i=0;i<3;i++)if(!Double.isFinite(p.component(i)) || Math.abs(p.component(i))>20000000)throw new IllegalArgumentException("Invalid melee ray");
  double distance=end.subtract(start).dot(end.subtract(start));if(distance<.000001 || distance>Math.pow(scale*reach+1,2))throw new IllegalArgumentException("Invalid melee reach");
  if(kind<0 || kind>1 || projectile<0 || (kind==0 && projectile!=0) || (kind==1 && projectile==0))throw new IllegalArgumentException("Invalid projectile identity");
 }
 public JsonObject toJson(){var j=new JsonObject();j.addProperty("id",id);j.addProperty("slot",slot);j.addProperty("guid",guid);j.addProperty("damage",damage);j.addProperty("scale",scale);j.addProperty("reach",reach);j.addProperty("kind",kind);j.addProperty("projectile",projectile);for(var key:new String[]{"start","end"}){var a=new JsonArray();var p=key.equals("start")?start:end;for(int i=0;i<3;i++)a.add(p.component(i));j.add(key,a);}return j;}
 public static double damage(double attribute,double cooldown){if(!Double.isFinite(attribute+cooldown) || attribute<0 || attribute>1000 || cooldown<0 || cooldown>1)throw new IllegalArgumentException("Invalid melee damage");return attribute*(.2+.8*cooldown*cooldown);}
}
