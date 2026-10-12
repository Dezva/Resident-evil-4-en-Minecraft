package dev.dezva.re4bridge;
import com.google.gson.JsonObject;
import com.google.gson.JsonArray;
import java.util.Base64;
/** Immutable cached HUD image: legacy bar or bounded, compressed GUI/hand layer. */
public final class HudFrame {
    public static final int WIDTH=256,HEIGHT=32,BYTES=WIDTH*HEIGHT*4;
    public static final int MAX_WIDTH=1600,MAX_HEIGHT=900,MAX_COMPRESSED=196608;
    private final String image;private final int width,height;private final boolean menu;private final double[] selection;
    public HudFrame(String image){this(image,0,0,false,null);}
    private HudFrame(String image,int width,int height,boolean menu,double[] selection){
        if(image==null)throw new IllegalArgumentException("Invalid HUD pixels");
        int length=Base64.getDecoder().decode(image).length;
        if(width==0?(image.length()!=43692 || length!=BYTES):(width<32 || width>MAX_WIDTH || height<32 || height>MAX_HEIGHT || length>MAX_COMPRESSED || image.length()>262144))throw new IllegalArgumentException("Invalid HUD pixels");
        if(selection!=null){
            if(selection.length!=10)throw new IllegalArgumentException("Invalid selection");
            for(double x:selection)if(!Double.isFinite(x) || Math.abs(x)>20000000)throw new IllegalArgumentException("Invalid selection");
            if(Math.abs(selection[3])>Math.PI)throw new IllegalArgumentException("Invalid selection rotation");
            for(int i=0;i<3;i++)if(selection[7+i]-selection[4+i]<1 || selection[7+i]-selection[4+i]>7500)throw new IllegalArgumentException("Invalid selection size");
        }
        this.image=image;this.width=width;this.height=height;this.menu=menu;this.selection=selection==null?null:selection.clone();
    }
    public static HudFrame overlay(String image,int width,int height,boolean menu){return new HudFrame(image,width,height,menu,null);}
    public HudFrame selection(double[] selection){return new HudFrame(image,width,height,menu,selection);}
    public String image(){return image;}public boolean menu(){return menu;}
    public JsonObject toJson(){
        var object=new JsonObject();object.addProperty("image",image);
        if(width!=0){object.addProperty("encoding","zlib-argb");object.addProperty("width",width);object.addProperty("height",height);object.addProperty("menu",menu);}
        if(selection!=null && !menu){var values=new JsonArray();for(double x:selection)values.add(x);object.add("selection",values);}
        return object;
    }
}
