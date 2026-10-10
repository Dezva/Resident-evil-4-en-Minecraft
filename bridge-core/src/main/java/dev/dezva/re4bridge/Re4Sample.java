package dev.dezva.re4bridge;

import com.google.gson.JsonObject;
import java.io.IOException;

/** A validated sample, aged using the receiving JVM clock, never the peer's clock. */
public record Re4Sample(double x, double y, double z, double yawRadians, long receivedNs) {
    public boolean fresh(long nowNs) { return nowNs >= receivedNs && nowNs - receivedNs < 1_000_000_000L; }

    public static Re4Sample parse(JsonObject sample, long nowNs) throws IOException {
        try {
            String state = sample.get("state").getAsString();
            if (state.equals("waiting_player") || state.equals("transition") || state.equals("unavailable")) return null;
            if (!state.equals("player")) throw new IOException("Unknown RE4 state");
            double[][] vectors = new double[2][3];
            String[] keys = {"position_raw", "angles_raw"};
            for (int i = 0; i < 2; i++) {
                var array = sample.getAsJsonArray(keys[i]);
                if (array.size() != 3) throw new IOException("Invalid RE4 vector");
                for (int j = 0; j < 3; j++) {
                    var value = array.get(j);
                    if (!value.isJsonPrimitive() || !value.getAsJsonPrimitive().isNumber()) throw new IOException("Non numeric RE4 vector");
                    vectors[i][j] = value.getAsDouble();
                    if (!Double.isFinite(vectors[i][j]) || Math.abs(vectors[i][j]) > 100_000_000) throw new IOException("RE4 vector out of bounds");
                }
            }
            return new Re4Sample(vectors[0][0], vectors[0][1], vectors[0][2], vectors[1][1], nowNs);
        } catch (RuntimeException e) { throw new IOException("Invalid RE4 sample"); }
    }
}
