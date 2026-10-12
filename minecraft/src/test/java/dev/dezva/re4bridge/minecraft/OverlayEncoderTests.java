package dev.dezva.re4bridge.minecraft;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
class OverlayEncoderTests {
    @Test void encoderPublishesNewestMenuFrameAndStopsWithoutReusingClosedWork() throws Exception {
        var encoder=new OverlayEncoder();
        try{
            for(int i=0;i<40;i++)encoder.submit(new byte[32*32*4],32,32,false);
            encoder.submit(new byte[64*32*4],64,32,true);
            long end=System.nanoTime()+2_000_000_000L;
            while((encoder.latest()==null || !encoder.latest().menu()) && System.nanoTime()<end)Thread.sleep(5);
            assertNotNull(encoder.latest());assertTrue(encoder.latest().menu());assertEquals(64,encoder.latest().toJson().get("width").getAsInt());assertNull(encoder.failure());
        }finally{encoder.close();}
        encoder.submit(new byte[32*32*4],32,32,false);assertNull(encoder.latest());
    }
}
