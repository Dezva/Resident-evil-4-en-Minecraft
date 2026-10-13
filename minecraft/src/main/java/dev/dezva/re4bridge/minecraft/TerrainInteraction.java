package dev.dezva.re4bridge.minecraft;
import dev.dezva.re4bridge.*;
import java.util.*;
import net.minecraft.client.MinecraftClient;
import net.minecraft.entity.projectile.PersistentProjectileEntity;
import net.minecraft.item.ItemPlacementContext;
import net.minecraft.block.BlockState;
import net.minecraft.util.hit.*;
import net.minecraft.util.math.*;
import net.minecraft.world.World;

/** Native surfaces for the one local bridge player. Normal Minecraft is untouched. */
public final class TerrainInteraction {
    private record Context(BridgeConnection bridge,ControllerMapping mapping,TerrainSnapshot mesh,int generation,UUID player,String dimension,Set<BlockPos> placed,double eye,Vec3d direction,long tick){}
    private static volatile Context context;
    private static BlockPos cachedCell,cachedCandidate;private static Direction cachedSide;private static TerrainSnapshot cachedMesh;
    public static void bind(BridgeConnection bridge,ControllerMapping mapping,int generation,UUID player,String dimension,Set<BlockPos> placed,double eye,Vec3d direction){context=mapping==null?null:new Context(bridge,mapping,bridge.terrain(),generation,player,dimension,placed,eye,direction,System.nanoTime());}
    public static void clear(){context=null;cachedCell=cachedCandidate=null;cachedMesh=null;}
    private static boolean active(Context c){return c!=null && System.nanoTime()-c.tick<200_000_000L && c.bridge.status().connected() && c.bridge.nativeArmed() && c.bridge.nativeStop()==0;}
    private static boolean ready(Context c){return active(c) && c.mesh!=null && c.mesh.generation()==c.generation && c.mesh.fresh(System.nanoTime());}
    private static TerrainSnapshot.Point point(Vec3d p){return new TerrainSnapshot.Point(p.x,p.y,p.z);}
    private static Vec3d vector(TerrainSnapshot.Point p){return new Vec3d(p.x(),p.y(),p.z());}
    private static TerrainSnapshot.Hit ray(Context c,Vec3d a,Vec3d b){return c.mesh.raycast(TerrainSnapshot.nativePoint(c.mapping,point(a)),TerrainSnapshot.nativePoint(c.mapping,point(b)));}
    private static Direction normal(Context c,TerrainSnapshot.Point n){double co=Math.cos(c.mapping.rotation()),si=Math.sin(c.mapping.rotation());return Direction.getFacing((float)(co*n.x()+si*n.z()),(float)n.y(),(float)(-si*n.x()+co*n.z()));}
    private static boolean covered(Context c,Vec3d p){return ready(c) && c.mesh.covered(TerrainSnapshot.nativePoint(c.mapping,point(p)));}
    private static boolean box(Context c,Box box){return !c.mesh.intersects(c.mapping,placementBounds(box),.0005);}
    // Two centimeters of support contact, not one whole voxel of vertical displacement.
    static double[] placementBounds(Box box){return new double[]{box.minX,box.minY+Math.min(.02,(box.maxY-box.minY)*.2),box.minZ,box.maxX,box.maxY,box.maxZ};}
    static BlockHitResult nearestPlaced(Vec3d start,Vec3d end,Set<BlockPos> positions,java.util.function.Function<BlockPos,net.minecraft.util.shape.VoxelShape> shapes){
        BlockHitResult best=null;double distance=Double.POSITIVE_INFINITY;
        for(var pos:positions){var hit=shapes.apply(pos).raycast(start,end,pos);if(hit!=null && start.squaredDistanceTo(hit.getPos())<distance){best=hit;distance=start.squaredDistanceTo(hit.getPos());}}
        return best;
    }
    private static BlockHitResult placed(Context c,World world,Vec3d start,Vec3d end,boolean outline){
        return nearestPlaced(start,end,c.placed,pos->{
            if(!world.isChunkLoaded(pos))return net.minecraft.util.shape.VoxelShapes.empty();
            var state=world.getBlockState(pos);return outline?state.getOutlineShape(world,pos):state.getCollisionShape(world,pos);
        });
    }
    public static void crosshair(MinecraftClient client,float delta){
        Context c=context;if(!active(c) || client.player==null || client.world==null || !client.player.getUuid().equals(c.player))return;
        var sample=c.bridge.re4Sample();if(sample==null || !sample.fresh(System.nanoTime()))return;
        var head=c.mapping.eye(sample,c.eye,0);
        Vec3d start=new Vec3d(head.x(),head.y(),head.z()),end=start.add(c.direction.multiply(client.player.getBlockInteractionRange()));
        client.targetedEntity=null;
        if(!ready(c) || !covered(c,start) || !covered(c,end)){client.crosshairTarget=BlockHitResult.createMissed(end,Direction.UP,BlockPos.ofFloored(end));return;}
        BlockHitResult best=placed(c,client.world,start,end,true);double distance=best==null?Double.POSITIVE_INFINITY:start.squaredDistanceTo(best.getPos());
        var nativeHit=ray(c,start,end);
        if(nativeHit!=null){Vec3d impact=vector(TerrainSnapshot.grid(c.mapping,nativeHit.point()));
            if(start.squaredDistanceTo(impact)<distance){
                Direction side=normal(c,nativeHit.normal());BlockPos cell=BlockPos.ofFloored(impact.add(side.getOffsetX()*.0001,side.getOffsetY()*.0001,side.getOffsetZ()*.0001));
                if(!cell.equals(cachedCell) || side!=cachedSide || c.mesh!=cachedMesh){
                    cachedCell=cell;cachedSide=side;cachedMesh=c.mesh;cachedCandidate=null;
                    for(int step=0;step<2;step++){var candidate=cell.offset(side,step);try{if(client.world.getBlockState(candidate).isReplaceable() && box(c,new Box(candidate))){cachedCandidate=candidate;break;}}catch(IllegalArgumentException unknown){break;}}
                }
                best=cachedCandidate!=null?candidateHit(impact,side,cachedCandidate):BlockHitResult.createMissed(impact,side,cell);
                if(best.getType()==HitResult.Type.BLOCK && (start.squaredDistanceTo(best.getPos())>client.player.getBlockInteractionRange()*client.player.getBlockInteractionRange() || !client.world.getBlockState(best.getBlockPos()).isReplaceable()))best=BlockHitResult.createMissed(impact,side,cell);
            }
        }
        client.crosshairTarget=best==null?BlockHitResult.createMissed(end,Direction.UP,BlockPos.ofFloored(end)):best;
    }
    /** Native hit stays in the geometry query; the vanilla packet must name a
     * point inside the selected Minecraft cell, including after slope adjustment. */
    static BlockHitResult candidateHit(Vec3d impact,Direction side,BlockPos candidate){
        if(!Double.isFinite(impact.x+impact.y+impact.z))throw new IllegalArgumentException("Invalid native hit");
        var point=new Vec3d(MathHelper.clamp(impact.x,candidate.getX(),candidate.getX()+1.),MathHelper.clamp(impact.y,candidate.getY(),candidate.getY()+1.),MathHelper.clamp(impact.z,candidate.getZ(),candidate.getZ()+1.));
        return new BlockHitResult(point,side,candidate,false);
    }
    public static boolean surfaceTarget(BlockHitResult hit){return active(context) && cachedCandidate!=null && cachedCandidate.equals(hit.getBlockPos());}
    public static boolean placement(ItemPlacementContext placement,BlockState state){
        Context c=context;if(!active(c) || placement.getPlayer()==null || !placement.getPlayer().getUuid().equals(c.player) || !placement.getWorld().getRegistryKey().getValue().toString().equals(c.dimension))return true;
        if(!ready(c))return false;
        var pos=placement.getBlockPos();var shape=state.getOutlineShape(placement.getWorld(),pos);
        if(shape.isEmpty())shape=state.getCollisionShape(placement.getWorld(),pos);
        try{for(var bounds:shape.getBoundingBoxes())if(!box(c,bounds.offset(pos)))return false;return true;}catch(IllegalArgumentException unknown){return false;}
    }
    private static Context arrow(PersistentProjectileEntity arrow){Context c=context;return active(c) && arrow.getOwner()!=null && arrow.getOwner().getUuid().equals(c.player) && arrow.getWorld().getRegistryKey().getValue().toString().equals(c.dimension)?c:null;}
    public static boolean managedArrow(PersistentProjectileEntity arrow){return arrow(arrow)!=null;}
    public static boolean freeze(PersistentProjectileEntity arrow){Context c=arrow(arrow);return c!=null && (!ready(c) || !covered(c,arrow.getPos()) || !covered(c,arrow.getPos().add(arrow.getVelocity())));}
    public static BlockHitResult projectile(PersistentProjectileEntity arrow,World world,Vec3d start,Vec3d end,BlockHitResult original){
        Context c=arrow(arrow);if(c==null || !ready(c) || !covered(c,start) || !covered(c,end))return original;
        BlockHitResult best=placed(c,world,start,end,false);double distance=best==null?Double.POSITIVE_INFINITY:start.squaredDistanceTo(best.getPos());
        var hit=ray(c,start,end);if(hit!=null){var impact=vector(TerrainSnapshot.grid(c.mapping,hit.point()));if(start.squaredDistanceTo(impact)<distance)best=new BlockHitResult(impact,normal(c,hit.normal()),BlockPos.ofFloored(impact),false);}
        return best==null?BlockHitResult.createMissed(end,Direction.UP,BlockPos.ofFloored(end)):best;
    }
}
