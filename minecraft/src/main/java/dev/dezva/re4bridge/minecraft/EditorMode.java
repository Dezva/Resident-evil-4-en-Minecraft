package dev.dezva.re4bridge.minecraft;

import net.minecraft.entity.player.PlayerEntity;

/** Temporary creative editor mode: RE4 supplies physics, Minecraft supplies block interaction. */
final class EditorMode {
    interface Slot {
        boolean flying();boolean noclip();
        void flying(boolean value);void noclip(boolean value);
    }
    private Slot owner;
    private boolean flying,noclip;
    static Slot player(PlayerEntity player){return new Slot(){
        public boolean flying(){return player.getAbilities().flying;}
        public boolean noclip(){return player.noClip;}
        public void flying(boolean value){player.getAbilities().flying=value && player.getAbilities().allowFlying;}
        public void noclip(boolean value){player.noClip=value;}
    };}
    void take(Slot slot){
        if(owner!=slot){release();owner=slot;flying=slot.flying();noclip=slot.noclip();}
        slot.flying(true);slot.noclip(true);
    }
    void release(){if(owner!=null){owner.flying(flying);owner.noclip(noclip);owner=null;}}
}
