package dev.dezva.re4bridge.minecraft;

import dev.dezva.re4bridge.BlockPreview;
import dev.dezva.re4bridge.BlockScene;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.function.Function;
import net.minecraft.block.BlockState;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Box;

/** Only explicitly placed/selected blocks; never copies the superflat terrain. */
final class PlacedBlockScene {
    static final int MAX_BLOCKS=64;
    private record Entry(BlockState state,List<Box> shapes) {}
    private final LinkedHashMap<BlockPos,Entry> blocks=new LinkedHashMap<>();
    private BlockPos anchor;
    private BlockPreview nativeAnchor;
    private String dimension;
    private double yaw,height;
    private int style;

    boolean put(BlockPlacementProbe.Placed placed,List<Box> shapes,double minecraftYaw) {
        if (shapes.isEmpty()) return false;
        boolean first=anchor==null;
        if (!first && !dimension.equals(placed.dimension())) return false;
        if (!blocks.containsKey(placed.pos()) && blocks.size()==MAX_BLOCKS) return false;
        // The reference block remains the reference even after it is removed.
        BlockPos reference=first?placed.pos():anchor;
        double size=first?placed.box().size():nativeAnchor.size();
        var candidate=new Entry(placed.state(),List.copyOf(shapes));
        try {
            convert(placed.pos(),candidate,reference,size);
            int count=shapes.size();
            for (var entry:blocks.entrySet()) if (!entry.getKey().equals(placed.pos())) count+=entry.getValue().shapes().size();
            if (count>BlockScene.MAX_BOXES || shapes.size()>16) return false;
        } catch (IllegalArgumentException error) { return false; }
        if (first) {
            anchor=placed.pos().toImmutable();nativeAnchor=placed.box();dimension=placed.dimension();
            yaw=Math.IEEEremainder(minecraftYaw,360)*Math.PI/180;
        }
        blocks.put(placed.pos().toImmutable(),candidate);return true;
    }
    // Set the grid rotation once from both players' facing, never from movement.
    void align(double re4Yaw) {
        yaw=Math.IEEEremainder(-re4Yaw-yaw,2*Math.PI);
    }
    boolean update(Function<BlockPos,BlockState> read,Function<BlockPos,List<Box>> shapes,String world) {
        return update(read,shapes,pos -> true,world);
    }
    boolean update(Function<BlockPos,BlockState> read,Function<BlockPos,List<Box>> shapes,java.util.function.Predicate<BlockPos> loaded,String world) {
        if (anchor==null) return true;
        if (!dimension.equals(world)) {clear();return true;}
        var iterator=blocks.entrySet().iterator();
        while (iterator.hasNext()) {
            var entry=iterator.next();if (!loaded.test(entry.getKey())) continue;
            var state=read.apply(entry.getKey());
            if (state.isAir() || !state.isOf(entry.getValue().state().getBlock())) iterator.remove();
            else {
                // Slabs may become double slabs and fences may gain neighbours.
                var changed=new Entry(state,List.copyOf(shapes.apply(entry.getKey())));
                try {
                    convert(entry.getKey(),changed,anchor,nativeAnchor.size());
                    entry.setValue(changed);
                } catch (IllegalArgumentException error) { iterator.remove(); }
            }
        }
        int count=0;
        for (var entry:blocks.values()) count+=entry.shapes().size();
        if (count>BlockScene.MAX_BOXES) {blocks.clear();return false;}
        return true;
    }
    private static List<BlockScene.Bounds> convert(BlockPos pos,Entry entry,BlockPos reference,double size) {
        var boxes=new ArrayList<BlockScene.Bounds>();
        if (entry.shapes().size()>16) throw new IllegalArgumentException("Complex shape");
        double x=((double)pos.getX()-reference.getX()-.5)*size;
        double y=((double)pos.getY()-reference.getY())*size;
        double z=((double)pos.getZ()-reference.getZ()-.5)*size;
        for (var box:entry.shapes()) boxes.add(new BlockScene.Bounds(x+box.minX*size,y+box.minY*size,z+box.minZ*size,
                x+box.maxX*size,y+box.maxY*size,z+box.maxZ*size));
        return boxes;
    }
    BlockScene snapshot() {
        if (anchor==null) return null;
        var boxes=new ArrayList<BlockScene.Bounds>();
        for (var entry:blocks.entrySet()) boxes.addAll(convert(entry.getKey(),entry.getValue(),anchor,nativeAnchor.size()));
        return boxes.isEmpty()?null:new BlockScene(nativeAnchor.x(),nativeAnchor.y()+height,nativeAnchor.z(),yaw,boxes,style);
    }
    void height(double value) {
        if (!Double.isFinite(value) || Math.abs(value)>2500) throw new IllegalArgumentException("Invalid height adjustment");
        height=value;
    }
    int size() {return blocks.size();}
    void style(int value) {if (value<0 || value>1) throw new IllegalArgumentException("Invalid scene style");style=value;}
    int style() {return style;}
    boolean anchored() {return anchor!=null;}
    String dimension() {return dimension;}
    void clear() {blocks.clear();anchor=null;nativeAnchor=null;dimension=null;yaw=height=0;style=0;}
}
