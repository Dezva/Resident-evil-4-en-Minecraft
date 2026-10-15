package dev.dezva.re4bridge;
import com.google.gson.*;
import java.io.IOException;
import java.util.*;

/** Native actor identities/feet, never Minecraft entities or caller-provided pointers. */
public record CombatState(boolean available,boolean active,boolean fresh,int generation,int acknowledgement,int reason,int hpBefore,int hpAfter,long hits,long kills,long receivedNs,List<Enemy> enemies,boolean reactionSupported,TerrainSnapshot.Point impact,int targetKind) {
 public record Enemy(int slot,long guid,int kind,int hp,int maximum,TerrainSnapshot.Point position,double radius,double height){}
 public record Hit(Enemy enemy,double fraction,TerrainSnapshot.Point point){}
 public boolean fresh(long now){return available && active && fresh && now>=receivedNs && now-receivedNs<200_000_000L;}
 public Hit raycast(TerrainSnapshot.Point start,TerrainSnapshot.Point end){Hit best=null;for(var e:enemies){double t=intersection(start,end,new double[]{e.position.x()-e.radius,e.position.y(),e.position.z()-e.radius,e.position.x()+e.radius,e.position.y()+e.height,e.position.z()+e.radius});if(Double.isFinite(t) && (best==null || t<best.fraction))best=new Hit(e,t,start.add(end.subtract(start).times(t)));}return best;}
 public static double intersection(TerrainSnapshot.Point start,TerrainSnapshot.Point end,double[] box){double low=0,high=1;for(int i=0;i<3;i++){double delta=end.component(i)-start.component(i),a=start.component(i);if(Math.abs(delta)<1e-9){if(a<box[i] || a>box[i+3])return Double.NaN;}else{double t=(box[i]-a)/delta,u=(box[i+3]-a)/delta;if(t>u){double tmp=t;t=u;u=tmp;}low=Math.max(low,t);high=Math.min(high,u);if(low>high)return Double.NaN;}}return low;}
 public static CombatState parse(JsonObject j,long now) throws IOException {
  try{
   boolean available=bool(j,"available");if(!available)return new CombatState(false,false,false,0,0,1,0,0,0,0,now,List.of(),false,null,0);
   boolean active=bool(j,"active"),fresh=bool(j,"fresh");int generation=(int)number(j,"generation",Integer.MAX_VALUE),ack=(int)number(j,"ack",Integer.MAX_VALUE),reason=(int)number(j,"reason",9),before=(int)number(j,"hp_before",32767),after=(int)number(j,"hp_after",32767);
   if(after>before)throw new IllegalArgumentException();var rows=j.getAsJsonArray("enemies");if(rows.size()>128 || !active && !rows.isEmpty())throw new IllegalArgumentException();var result=new ArrayList<Enemy>();var slots=new HashSet<Integer>();
   for(var row:rows){var e=row.getAsJsonObject();int slot=(int)number(e,"slot",4095),hp=(int)number(e,"hp",32767),max=(int)number(e,"maximum",32767);if(hp<1 || max<hp || !slots.add(slot))throw new IllegalArgumentException();var p=e.getAsJsonArray("position");if(p.size()!=3)throw new IllegalArgumentException();double radius=real(e.get("radius")),height=real(e.get("height"));if(radius<100 || radius>1000 || height<300 || height>3000)throw new IllegalArgumentException();result.add(new Enemy(slot,number(e,"guid",0xffffffffL),(int)number(e,"kind",255),hp,max,new TerrainSnapshot.Point(real(p.get(0)),real(p.get(1)),real(p.get(2))),radius,height));}
   boolean supported=j.has("engine_supported") && bool(j,"engine_supported");TerrainSnapshot.Point impact=null;int targetKind=0;
   if(j.has("hit_position")){var p=j.getAsJsonArray("hit_position");if(p.size()!=3)throw new IllegalArgumentException();impact=new TerrainSnapshot.Point(real(p.get(0)),real(p.get(1)),real(p.get(2)));targetKind=(int)number(j,"target_kind",255);}
   return new CombatState(true,active,fresh,generation,ack,reason,before,after,number(j,"hits",0xffffffffL),number(j,"kills",0xffffffffL),now,List.copyOf(result),supported,impact,targetKind);
  }catch(RuntimeException e){throw new IOException("Invalid native combat state",e);}
 }
 private static boolean bool(JsonObject j,String name){var p=j.getAsJsonPrimitive(name);if(!p.isBoolean())throw new IllegalArgumentException();return p.getAsBoolean();}
 private static long number(JsonObject j,String name,long max){var p=j.getAsJsonPrimitive(name);long n=p.getAsLong();if(!p.isNumber() || n<0 || n>max || !p.getAsString().equals(Long.toString(n)))throw new IllegalArgumentException();return n;}
 private static double real(JsonElement e){if(!e.isJsonPrimitive() || !e.getAsJsonPrimitive().isNumber())throw new IllegalArgumentException();double n=e.getAsDouble();if(!Double.isFinite(n) || Math.abs(n)>20000000)throw new IllegalArgumentException();return n;}
}
