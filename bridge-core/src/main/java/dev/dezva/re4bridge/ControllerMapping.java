package dev.dezva.re4bridge;

/** One rigid transform shared by the editor's player, rays and placed blocks. */
public record ControllerMapping(Re4Sample origin,double x,double y,double z,double rotation,double scale) {
    public record Point(double x,double y,double z) {}
    public ControllerMapping {
        if(origin==null || !Double.isFinite(x+y+z+rotation+scale) || Math.abs(x)>29_000_000 || Math.abs(z)>29_000_000 || Math.abs(y)>10000 || Math.abs(rotation)>Math.PI || scale<40 || scale>2500)throw new IllegalArgumentException("Invalid controller mapping");
    }
    public static ControllerMapping anchor(Re4Sample sample,double x,double y,double z,double minecraftYaw,double scale){
        return new ControllerMapping(sample,x,y,z,Math.IEEEremainder(-sample.yawRadians()-Math.toRadians(minecraftYaw),2*Math.PI),scale);
    }
    public Point toNative(double mx,double my,double mz){
        double dx=(mx-x)*scale,dz=(mz-z)*scale,c=Math.cos(rotation),s=Math.sin(rotation);
        Point point=new Point(origin.x()+c*dx-s*dz,origin.y()+(my-y)*scale,origin.z()+s*dx+c*dz);
        if(!Double.isFinite(point.x()+point.y()+point.z()) || Math.abs(point.x())>19_990_000 || Math.abs(point.y())>19_990_000 || Math.abs(point.z())>19_990_000)throw new IllegalArgumentException("Native position outside scene bounds");
        return point;
    }
    public Point toMinecraft(Re4Sample sample){
        double dx=(sample.x()-origin.x())/scale,dz=(sample.z()-origin.z())/scale,c=Math.cos(rotation),s=Math.sin(rotation);
        Point point=new Point(x+c*dx+s*dz,y+(sample.y()-origin.y())/scale,z-s*dx+c*dz);
        if(!Double.isFinite(point.x()+point.y()+point.z()) || Math.abs(point.x())>29_000_000 || Math.abs(point.z())>29_000_000 || Math.abs(point.y())>10000)throw new IllegalArgumentException("Editor position outside world bounds");
        return point;
    }
    public ControllerMapping gridHeight(double height){
        if(!Double.isFinite(height) || Math.abs(height)>2500)throw new IllegalArgumentException("Invalid grid height");
        return new ControllerMapping(new Re4Sample(origin.x(),origin.y()+height,origin.z(),origin.yawRadians(),origin.receivedNs()),x,y,z,rotation,scale);
    }
    public double minecraftEyeHeight(double eye){
        double nativeEye=eye*scale;
        if(!Double.isFinite(nativeEye) || nativeEye<40 || nativeEye>2500)throw new IllegalArgumentException("Eye height outside native range");
        return nativeEye;
    }
    public Point eye(Re4Sample sample,double nativeEye,double gridHeight){
        if(!Double.isFinite(nativeEye+gridHeight) || Math.abs(nativeEye)>2500 || Math.abs(gridHeight)>2500)throw new IllegalArgumentException("Invalid editor eye");
        return toMinecraft(new Re4Sample(sample.x(),sample.y()+nativeEye-gridHeight,sample.z(),sample.yawRadians(),sample.receivedNs()));
    }
    /** Absolute mapped gaze survives missed first mouse samples and body/idle turns. */
    public MouseLook.Input look(double minecraftYaw,double pitch,double eye){
        return new MouseLook.Input(true,true,Math.toDegrees(Math.IEEEremainder(Math.toRadians(minecraftYaw)+rotation,2*Math.PI)),Math.max(-85,Math.min(85,pitch)),eye,true);
    }
    public float minecraftYaw(double nativeYaw){return (float)Math.toDegrees(Math.IEEEremainder(-nativeYaw-rotation,2*Math.PI));}
}
