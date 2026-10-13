package dev.dezva.re4bridge.minecraft;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
class EditorModeTests {
    @Test void survivalFlightIsBorrowedWithoutChangingGamemodeAndNewModesKeepTheirDefaults(){
        var player=new Player();var lease=new EditorMode();lease.take(player);
        assertEquals(0,player.mode);assertTrue(player.allowFlying);assertTrue(player.flying);
        lease.release();assertFalse(player.allowFlying);assertFalse(player.flying);assertFalse(player.noclip);
        lease.take(player);player.mode=1;player.allowFlying=true;player.flying=false;player.noclip=false;
        lease.take(player);lease.release();assertTrue(player.allowFlying);assertFalse(player.flying);assertFalse(player.noclip);
    }
    static class Player implements EditorMode.Slot {
        boolean flying,noclip,allowFlying;int mode;
        public boolean allowFlying(){return allowFlying;}public void allowFlying(boolean value){allowFlying=value;}public int mode(){return mode;}
        public boolean flying(){return flying;}public boolean noclip(){return noclip;}
        public void flying(boolean value){flying=value;}public void noclip(boolean value){noclip=value;}
    }
    @Test void repeatedFollowAndStopRestoreEveryOriginalModeCombination(){
        for(boolean flight:new boolean[]{false,true})for(boolean ghost:new boolean[]{false,true}){
            var player=new Player();player.flying=flight;player.noclip=ghost;var mode=new EditorMode();
            for(int i=0;i<100;i++)mode.take(player);
            assertTrue(player.flying);assertTrue(player.noclip);assertTrue(player.allowFlying);
            mode.release();mode.release();assertEquals(flight,player.flying);assertEquals(ghost,player.noclip);assertFalse(player.allowFlying);
        }
    }
    @Test void switchingPlayersRestoresOldModeBeforeTakingTheNewOne(){
        var first=new Player();var second=new Player();second.flying=true;var mode=new EditorMode();
        mode.take(first);mode.take(second);assertFalse(first.flying);assertFalse(first.noclip);
        mode.release();assertTrue(second.flying);assertFalse(second.noclip);
    }
}
