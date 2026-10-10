package dev.dezva.re4bridge;

/** Relative mapping around a user-selected anchor; scale/elevation/turn sign are explicit. */
public record Re4Calibration(Re4Sample origin, double minecraftX, double minecraftY, double minecraftZ,
                             float minecraftYaw, double unitsPerBlock, int ySign, int turnSign) {
    public record Target(double x, double y, double z, float yaw) {}
    public Re4Calibration {
        if (origin == null || !Double.isFinite(unitsPerBlock) || unitsPerBlock < 1 || unitsPerBlock > 10000
                || Math.abs(ySign) != 1 || Math.abs(turnSign) != 1) throw new IllegalArgumentException("Invalid calibration");
    }
    public Target map(Re4Sample sample) {
        double x = minecraftX + (sample.x() - origin.x()) / unitsPerBlock;
        double y = minecraftY + ySign * (sample.y() - origin.y()) / unitsPerBlock;
        double z = minecraftZ + (sample.z() - origin.z()) / unitsPerBlock;
        double degrees = minecraftYaw + turnSign * Math.toDegrees(sample.yawRadians() - origin.yawRadians());
        float yaw = (float) (((degrees + 180) % 360 + 360) % 360 - 180);
        if (!Double.isFinite(x + y + z) || Math.abs(x) > 29_000_000 || Math.abs(z) > 29_000_000 || Math.abs(y) > 10000)
            throw new IllegalArgumentException("Mapped position out of bounds");
        return new Target(x, y, z, yaw);
    }
}
