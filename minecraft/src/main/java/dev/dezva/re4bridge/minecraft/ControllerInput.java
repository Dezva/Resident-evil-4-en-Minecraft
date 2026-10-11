package dev.dezva.re4bridge.minecraft;

import net.minecraft.client.input.Input;
import net.minecraft.client.network.ClientPlayerEntity;

/** Borrow Input only while controlling RE4; raw keybindings/mouse/item use stay available. */
final class ControllerInput {
    interface Slot {Input get();void set(Input input);}
    private ClientPlayerEntity boundPlayer;
    private Slot boundSlot,owner;
    private Input original;
    private final Input idle=new Input(){
        @Override public void tick(boolean slowDown,float factor){
            movementSideways=movementForward=0;
            pressingForward=pressingBack=pressingLeft=pressingRight=jumping=sneaking=false;
        }
    };
    void update(ClientPlayerEntity current,boolean locked) {
        if(boundPlayer!=current){
            release();boundPlayer=current;
            boundSlot=current==null?null:new Slot(){
                public Input get(){return current.input;}
                public void set(Input input){current.input=input;}
            };
        }
        apply(boundSlot,locked);
    }
    void apply(Slot current,boolean locked){
        if(owner!=current || !locked)release();
        if(!locked || current==null)return;
        if(current.get()!=idle){original=current.get();owner=current;current.set(idle);}
    }
    void release(){
        // Never overwrite a replacement installed by another component.
        if(owner!=null && owner.get()==idle)owner.set(original);
        owner=null;original=null;
    }
}
