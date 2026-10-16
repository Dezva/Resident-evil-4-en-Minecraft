package dev.dezva.re4bridge.minecraft;

/** Implemented on vanilla projectiles by the client/integrated-server mixin. */
public interface BridgeArrow {
 boolean re4bridge$managed();
 boolean re4bridge$markRendered();
 /** Called once by the spawn check, before any bridge pause can cancel a tick. */
 static int launchGeneration(int age,int generation,boolean owned){return age>=0 && age<=1 && generation>0 && owned?generation:0;}
}
