package dev.dezva.re4bridge;

import com.google.gson.Gson;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.stream.JsonReader;
import com.google.gson.stream.JsonToken;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.StringReader;
import java.nio.ByteBuffer;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.CodingErrorAction;
import java.nio.charset.StandardCharsets;

/** UTF-8 JSON lines with a strict byte limit, even when the sender omits the newline. */
public final class LineProtocol {
    public static final int VERSION = 1;
    public static final int MAX_FRAME_BYTES = 524288;
    private static final Gson JSON = new Gson();
    private LineProtocol() {}

    public static JsonObject read(InputStream input) throws IOException {
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        int b;
        while ((b = input.read()) != -1 && b != '\n') {
            if (bytes.size() >= MAX_FRAME_BYTES) throw new IOException("Frame exceeds byte limit");
            bytes.write(b);
        }
        if (b == -1) throw new IOException("Peer closed or sent an incomplete frame");
        try {
            String json = StandardCharsets.UTF_8.newDecoder()
                    .onMalformedInput(CodingErrorAction.REPORT)
                    .onUnmappableCharacter(CodingErrorAction.REPORT)
                    .decode(ByteBuffer.wrap(bytes.toByteArray())).toString();
            JsonReader reader = new JsonReader(new StringReader(json));
            reader.setLenient(false);
            // Calling the adapter directly preserves strictness on Gson 2.10.1.
            // JsonParser.parseString temporarily enables lenient parsing.
            JsonObject frame = JSON.getAdapter(JsonElement.class).read(reader).getAsJsonObject();
            if (reader.peek() != JsonToken.END_DOCUMENT) throw new IOException("Trailing JSON data");
            if (!frame.has("protocol") || !frame.get("protocol").isJsonPrimitive()
                    || !frame.getAsJsonPrimitive("protocol").isNumber()
                    || !frame.get("protocol").getAsString().equals("1")) {
                throw new IOException("Incompatible protocol");
            }
            return frame;
        } catch (RuntimeException | CharacterCodingException e) {
            throw new IOException("Invalid JSON frame");
        }
    }

    public static byte[] encode(JsonObject frame) throws IOException {
        byte[] bytes = (frame.toString() + "\n").getBytes(StandardCharsets.UTF_8);
        if (bytes.length - 1 > MAX_FRAME_BYTES) throw new IOException("Frame exceeds byte limit");
        return bytes;
    }
}
