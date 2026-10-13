package dev.dezva.re4bridge.minecraft;

import net.minecraft.entity.player.PlayerEntity;

/** Temporary creative editor mode: RE4 supplies physics, Minecraft supplies block interaction. */
final class EditorMode {
    interface Slot {
        boolean flying();boolean noclip();
        boolean allowFlying();void allowFlying(boolean value);
        default int mode(){return 0;}
        void flying(boolean value);void noclip(boolean value);
    }
    private Slot owner;
    private boolean flying,noclip,allowFlying;private int mode;
    static Slot player(PlayerEntity player){return new Slot(){
        public boolean flying(){return player.getAbilities().flying;}
        public boolean noclip(){return player.noClip;}
        public boolean allowFlying(){return player.getAbilities().allowFlying;}
        public void allowFlying(boolean value){player.getAbilities().allowFlying=value;}
        public int mode(){return player.isSpectator()?3:player.isCreative()?1:player.getAbilities().allowModifyWorld?0:2;}
        public void flying(boolean value){player.getAbilities().flying=value && player.getAbilities().allowFlying;}
        public void noclip(boolean value){player.noClip=value;}
    };}
    void take(Slot slot){
        if(owner!=slot || slot.mode()!=mode){release();owner=slot;flying=slot.flying();noclip=slot.noclip();allowFlying=slot.allowFlying();mode=slot.mode();}
        slot.allowFlying(true);
        slot.flying(true);slot.noclip(true);
    }
    void release(){if(owner!=null){if(owner.mode()==mode){owner.allowFlying(allowFlying);owner.flying(flying);owner.noclip(noclip);}owner=null;}}
}
