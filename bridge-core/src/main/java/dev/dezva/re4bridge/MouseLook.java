package dev.dezva.re4bridge;

/** Cumulative relative yaw: network coalescing cannot discard intermediate turns. */
public final class MouseLook {
    public record Input(boolean enabled, boolean firstPerson, double totalYaw, double pitch, double eyeHeight, boolean absoluteYaw) {
        public Input(boolean enabled,boolean firstPerson,double totalYaw,double pitch,double eyeHeight){this(enabled,firstPerson,totalYaw,pitch,eyeHeight,false);}
        public Input {
            if (absoluteYaw && (!firstPerson || !enabled || Math.abs(totalYaw)>180) || firstPerson && !enabled || !Double.isFinite(totalYaw) || Math.abs(totalYaw) > 10_000_000
                    || !Double.isFinite(pitch) || Math.abs(pitch) > 90
                    || !Double.isFinite(eyeHeight) || Math.abs(eyeHeight) < 40 || Math.abs(eyeHeight) > 2500)
                throw new IllegalArgumentException("Invalid mouse look");
        }
    }
    private boolean anchored;
    private double previousYaw;
    private double totalYaw;
    public void reset() { anchored = false; totalYaw = 0; }
    public Input sample(double yaw, double pitch, boolean firstPerson, double eyeHeight) {
        if (!Double.isFinite(yaw) || !Double.isFinite(pitch)) throw new IllegalArgumentException("Invalid angles");
        if (anchored) {
            double delta = ((yaw - previousYaw + 180) % 360 + 360) % 360 - 180;
            totalYaw += delta;
        }
        previousYaw = yaw;
        anchored = true;
        return new Input(true, firstPerson, totalYaw, Math.max(-90, Math.min(90, pitch)), eyeHeight);
    }
}
