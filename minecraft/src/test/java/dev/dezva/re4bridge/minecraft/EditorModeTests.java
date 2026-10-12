package dev.dezva.re4bridge.minecraft;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
class EditorModeTests {
    static class Player implements EditorMode.Slot {
        boolean flying,noclip;
        public boolean flying(){return flying;}public boolean noclip(){return noclip;}
        public void flying(boolean value){flying=value;}public void noclip(boolean value){noclip=value;}
    }
    @Test void repeatedFollowAndStopRestoreEveryOriginalModeCombination(){
        for(boolean flight:new boolean[]{false,true})for(boolean ghost:new boolean[]{false,true}){
            var player=new Player();player.flying=flight;player.noclip=ghost;var mode=new EditorMode();
            for(int i=0;i<100;i++)mode.take(player);
            assertTrue(player.flying);assertTrue(player.noclip);
            mode.release();mode.release();assertEquals(flight,player.flying);assertEquals(ghost,player.noclip);
        }
    }
    @Test void switchingPlayersRestoresOldModeBeforeTakingTheNewOne(){
        var first=new Player();var second=new Player();second.flying=true;var mode=new EditorMode();
        mode.take(first);mode.take(second);assertFalse(first.flying);assertFalse(first.noclip);
        mode.release();assertTrue(second.flying);assertFalse(second.noclip);
    }
}
