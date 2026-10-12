package dev.dezva.re4bridge.minecraft;

import java.util.HashSet;
import java.util.Set;
import net.minecraft.block.BlockState;
import net.minecraft.block.Blocks;
import net.minecraft.util.math.BlockPos;

/** Bounded temporary support in the creative editor. Never replaces existing terrain/builds. */
final class EditorFloor {
    static final int RADIUS=5;
    interface World {
        BlockState read(BlockPos pos);
        boolean loaded(BlockPos pos);
        boolean write(BlockPos pos,BlockState state);
        int bottom();int top();
    }
    private World world;
    private final Set<BlockPos> owned=new HashSet<>();
    private volatile Set<BlockPos> visible=Set.of();
    private final Set<BlockPos> claimed=java.util.concurrent.ConcurrentHashMap.newKeySet();
    void claim(BlockPos pos){if(visible.contains(pos))claimed.add(pos.toImmutable());}
    boolean contains(BlockPos pos){return visible.contains(pos) && !claimed.contains(pos);}
    static double anchorHeight(double current,int surface,int top){
        double height=Math.max(current,surface+16.0);
        if(!Double.isFinite(height) || height>top-4)throw new IllegalArgumentException("No hay espacio libre sobre el terreno para el editor");
        return height;
    }
    void update(World next,BlockPos feet){
        if(world!=next){clear();world=next;}
        if(feet.getY()-1<world.bottom() || feet.getY()+2>=world.top()){clear();return;}
        var wanted=new HashSet<BlockPos>();
        for(int x=-RADIUS;x<=RADIUS;x++)for(int z=-RADIUS;z<=RADIUS;z++)wanted.add(feet.add(x,-1,z).toImmutable());
        owned.removeAll(claimed);claimed.retainAll(wanted);
        var iterator=owned.iterator();
        while(iterator.hasNext()){
            var pos=iterator.next();
            if(!wanted.contains(pos)){
                if(world.read(pos).equals(Blocks.GLASS.getDefaultState()))world.write(pos,Blocks.AIR.getDefaultState());
                iterator.remove();
            }else if(world.loaded(pos) && !world.read(pos).equals(Blocks.GLASS.getDefaultState()))iterator.remove();
        }
        for(var pos:wanted)if(!claimed.contains(pos) && !owned.contains(pos) && world.loaded(pos) && world.read(pos).isAir()
            && world.write(pos,Blocks.GLASS.getDefaultState()))owned.add(pos);
        visible=Set.copyOf(owned);
    }
    void clear(){
        owned.removeAll(claimed);
        if(world!=null)for(var pos:owned)if(world.read(pos).equals(Blocks.GLASS.getDefaultState()))world.write(pos,Blocks.AIR.getDefaultState());
        owned.clear();visible=Set.of();claimed.clear();world=null;
    }
}
