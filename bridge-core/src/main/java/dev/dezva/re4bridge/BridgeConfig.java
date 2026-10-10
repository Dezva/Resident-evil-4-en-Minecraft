package dev.dezva.re4bridge;

import com.google.gson.Gson;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardOpenOption;
import java.security.SecureRandom;
import java.util.HexFormat;

/** Local authentication is created once and must never be logged or bundled. */
public record BridgeConfig(int protocol, int port, String token) {
    public static BridgeConfig loadOrCreate(Path file) throws IOException {
        Gson gson = new Gson();
        if (!Files.exists(file)) {
            byte[] secret = new byte[32];
            new SecureRandom().nextBytes(secret);
            BridgeConfig config = new BridgeConfig(1, 28741, HexFormat.of().formatHex(secret));
            Files.createDirectories(file.toAbsolutePath().getParent());
            Files.writeString(file, gson.toJson(config) + "\n", StandardOpenOption.CREATE_NEW);
            return config;
        }
        try {
            if (Files.size(file) > 4096) throw new IOException("Bridge config is too large");
            BridgeConfig config = gson.fromJson(Files.readString(file), BridgeConfig.class);
            if (config == null || config.protocol != 1 || config.port < 1024 || config.port > 65535
                    || config.token == null || !config.token.matches("[0-9a-f]{64}")) {
                throw new IOException("Invalid bridge config; expected protocol 1, a local port and a 64-character token");
            }
            return config;
        } catch (RuntimeException e) {
            // Do not include parsed values or parser error text: it may contain the secret.
            throw new IOException("Cannot parse bridge config");
        }
    }
}
