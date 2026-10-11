package dev.dezva.re4bridge.minecraft;

import net.minecraft.client.input.Input;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

class ControllerInputTests {
    static class Player implements ControllerInput.Slot {
        Input input=new Input();
        public Input get(){return input;}
        public void set(Input value){input=value;}
    }
    @Test void locksMovementWithoutClearingSourceAndRestoresOnStopOrDisconnection(){
        var controller=new ControllerInput();var player=new Player();var keyboard=player.input;
        keyboard.pressingForward=true;keyboard.movementForward=1;keyboard.jumping=true;
        controller.apply(player,true);assertNotSame(keyboard,player.input);
        player.input.tick(false,1);assertEquals(0,player.input.movementForward);assertFalse(player.input.jumping);
        assertTrue(keyboard.pressingForward);assertEquals(1,keyboard.movementForward);
        controller.apply(player,true);controller.release();assertSame(keyboard,player.input);
        controller.apply(player,true);controller.apply(null,false);assertSame(keyboard,player.input);
    }
    @Test void playerChangeAndForeignReplacementPreserveTheRightInput(){
        var controller=new ControllerInput();var first=new Player();var second=new Player();
        var keyboard1=first.input;var keyboard2=second.input;
        controller.apply(first,true);controller.apply(second,true);assertSame(keyboard1,first.input);
        var foreign=new Input();second.input=foreign;controller.release();assertSame(foreign,second.input);
        controller.apply(second,true);controller.apply(second,false);assertSame(foreign,second.input);
        assertNotSame(keyboard2,second.input);
    }
}
