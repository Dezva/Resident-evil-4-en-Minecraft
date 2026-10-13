package dev.dezva.re4bridge;
import com.google.gson.JsonObject;
import java.io.*;
import java.net.*;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.concurrent.*;
import java.util.function.Consumer;

/** Loopback-only, authenticated, fixed-action VIEW preparation endpoint. */
public final class PrepareServer implements AutoCloseable {
    private final ServerSocket server;private final BridgeConfig config;private final Consumer<CompletableFuture<String>> prepare;
    private volatile boolean closed;private final Thread worker;
    public static int port(BridgeConfig config){return config.port()==65535?65534:config.port()+1;}
    public PrepareServer(BridgeConfig config,Consumer<CompletableFuture<String>> prepare) throws IOException {
        this.config=config;this.prepare=prepare;server=new ServerSocket();server.bind(new InetSocketAddress(InetAddress.getByName("127.0.0.1"),port(config)),1);
        worker=new Thread(this::run,"re4bridge-view-prepare");worker.setDaemon(true);worker.start();
    }
    private void run(){while(!closed){try(var peer=server.accept()){handle(peer);}catch(IOException ignored){if(closed)return;}}}
    private void handle(Socket peer) throws IOException {
        peer.setSoTimeout(1500);var future=new CompletableFuture<String>();var response=new JsonObject();response.addProperty("protocol",1);response.addProperty("ok",false);
        try{
            var input=new FilterInputStream(new BufferedInputStream(peer.getInputStream())){int count;@Override public int read() throws IOException {if(++count>4096)throw new IOException("VIEW request too large");return super.read();}};
            var request=LineProtocol.read(input);var token=request.getAsJsonPrimitive("token");
            if(!request.get("type").getAsString().equals("prepare") || !token.isString() || token.getAsString().length()!=64 || !MessageDigest.isEqual(token.getAsString().getBytes(StandardCharsets.US_ASCII),config.token().getBytes(StandardCharsets.US_ASCII)))throw new IOException("Preparacion no autorizada");
            prepare.accept(future);peer.setSoTimeout(100);long deadline=System.nanoTime()+TimeUnit.SECONDS.toNanos(12);
            while(!future.isDone()) {
                if(closed)throw new IOException("VIEW cerrado");if(System.nanoTime()>deadline)throw new TimeoutException();
                try {if(input.read()<0)throw new IOException("VIEW desconectado");throw new IOException("Unexpected preparation data");}
                catch(SocketTimeoutException waiting) { }
            }
            String message=future.get();response.addProperty("ok",true);response.addProperty("message",message);
        }catch(Exception error){future.cancel(false);String message=error instanceof ExecutionException && error.getCause()!=null?error.getCause().getMessage():error instanceof TimeoutException?"Minecraft no termino la preparacion; control detenido.":"No se pudo preparar Minecraft: "+error.getClass().getSimpleName();response.addProperty("message",message);}
        peer.getOutputStream().write(LineProtocol.encode(response));
    }
    @Override public void close(){closed=true;try{server.close();}catch(IOException ignored){}worker.interrupt();}
}
