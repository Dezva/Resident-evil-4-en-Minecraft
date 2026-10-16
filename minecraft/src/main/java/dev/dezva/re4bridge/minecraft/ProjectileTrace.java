package dev.dezva.re4bridge.minecraft;

/** Bounded lifecycle evidence in the normal Minecraft log; never one entry per frame. */
public final class ProjectileTrace {
 private static int generation,messages;
 public static synchronized void event(int session,String stage,int entity,String detail){
  if(session<=0)return;
  if(generation!=session){generation=session;messages=0;}
  if(messages>=128)return;
  messages++;org.slf4j.LoggerFactory.getLogger("RE4Bridge").info("Arrow: generation={} stage={} entity={} {}",session,stage,entity,detail);
 }
 private ProjectileTrace(){}
}
