package dev.dezva.re4bridge;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
class ControllerMappingTests {
    @Test void rigidMappingRoundTripsAtEveryOrientationAndDoesNotReanchorWhenLeonMoves(){
        for(double ny:new double[]{0,.7,-2.9})for(double my:new double[]{0,90,-123}){
            var origin=new Re4Sample(-103000,-400,1700,ny,100);
            var mapping=ControllerMapping.anchor(origin,12.2,64,-7.6,my,1000);
            var target=mapping.toNative(18.2,65.5,-12.6);
            var reverse=mapping.toMinecraft(new Re4Sample(target.x(),target.y(),target.z(),2,200));
            assertEquals(18.2,reverse.x(),1e-8);assertEquals(65.5,reverse.y(),1e-8);assertEquals(-12.6,reverse.z(),1e-8);
            var forward=mapping.toNative(12.2-Math.sin(Math.toRadians(my)),64,-7.6+Math.cos(Math.toRadians(my)));
            assertEquals(Math.sin(ny)*1000,forward.x()-origin.x(),1e-7);assertEquals(Math.cos(ny)*1000,forward.z()-origin.z(),1e-7);
            assertEquals(my,mapping.minecraftYaw(ny),1e-4);
            assertEquals(target,mapping.toNative(18.2,65.5,-12.6)); // the block never follows the player
        }
    }
    @Test void cameraEyeAndGridHeightShareTheSameTransform(){
        var sample=new Re4Sample(-103000,-400,1700,.7,0);var mapping=ControllerMapping.anchor(sample,12,64,3,20,1000);
        var eye=mapping.eye(sample,1620,250);var nativeEye=mapping.toNative(eye.x(),eye.y(),eye.z());
        assertEquals(sample.y()+1620,nativeEye.y()+250,1e-8);
        assertEquals(sample.x(),nativeEye.x(),1e-8);assertEquals(sample.z(),nativeEye.z(),1e-8);
    }
    @Test void absoluteGazeMatchesMappedRaysAfterArbitraryMouseTurnsAndRearming(){
        for(double ny:new double[]{0,.7,-2.9})for(double my:new double[]{0,90,-123}){
            var sample=new Re4Sample(-103000,-400,1700,ny,0);var mapping=ControllerMapping.anchor(sample,12,80,3,my,1000);
            for(double turn:new double[]{0,179,360,-720,953}){
                var gaze=mapping.look(my+turn,35,1620);assertTrue(gaze.absoluteYaw());
                double nativeYaw=-Math.toRadians(gaze.totalYaw());
                double yaw=Math.toRadians(my+turn),pitch=Math.toRadians(35);
                var origin=mapping.toNative(12,81.62,3);
                var target=mapping.toNative(12-Math.sin(yaw)*Math.cos(pitch),81.62-Math.sin(pitch),3+Math.cos(yaw)*Math.cos(pitch));
                assertEquals(Math.sin(nativeYaw)*Math.cos(pitch)*1000,target.x()-origin.x(),1e-7);
                assertEquals(Math.cos(nativeYaw)*Math.cos(pitch)*1000,target.z()-origin.z(),1e-7);
                assertEquals(-Math.sin(pitch)*1000,target.y()-origin.y(),1e-7);
            }
        }
    }
    @Test void poseHeightAndGridShiftKeepFeetAndEyesConsistent(){
        var origin=new Re4Sample(-103000,-400,1700,.7,0);
        for(double scale:new double[]{500,1000})for(double eye:new double[]{1.62,1.27})for(double shift:new double[]{0,250,-150}){
            var base=ControllerMapping.anchor(origin,12,80,3,20,scale);var grid=base.gridHeight(shift);
            double nativeEye=base.minecraftEyeHeight(eye);
            var head=base.eye(origin,nativeEye,shift);var direct=grid.eye(origin,nativeEye,0);
            assertEquals(head.x(),direct.x(),1e-9);assertEquals(head.y(),direct.y(),1e-9);assertEquals(head.z(),direct.z(),1e-9);
            var feet=grid.toNative(head.x(),head.y()-eye,head.z());
            assertEquals(origin.y(),feet.y(),1e-8);
            var nativeHead=grid.toNative(head.x(),head.y(),head.z());assertEquals(origin.y()+eye*scale,nativeHead.y(),1e-8);
            var point=base.toNative(13,82,4);var raised=grid.toNative(13,82,4);assertEquals(shift,raised.y()-point.y(),1e-8);
        }
    }
    @Test void invalidTransformsAndExtremeMappedPositionsAreRejected(){
        var sample=new Re4Sample(0,0,0,0,0);
        assertThrows(IllegalArgumentException.class,()->ControllerMapping.anchor(sample,0,0,0,0,0));
        var mapping=ControllerMapping.anchor(sample,0,64,0,0,1000);
        assertThrows(IllegalArgumentException.class,()->mapping.toNative(Double.NaN,64,0));
        assertThrows(IllegalArgumentException.class,()->mapping.toMinecraft(new Re4Sample(0,20_000_000,0,0,0)));
    }
}
