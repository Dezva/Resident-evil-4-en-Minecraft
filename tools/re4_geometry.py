"""Offline queries over validated local SAT/EAT exports. No game/IPC writes.

This is a geometry foundation, not an active Minecraft terrain importer or visual depth buffer.
Unknown space outside the exported cube must never be treated as free space.
"""
from dataclasses import dataclass
import math


def _vector(value):
    if not isinstance(value,(list,tuple)) or len(value)!=3 or any(type(v) not in (int,float) or not math.isfinite(v) or abs(v)>20_000_000 for v in value):
        raise ValueError('Invalid geometry vector')
    return tuple(float(v) for v in value)


def _sub(a,b):return tuple(a[i]-b[i] for i in range(3))
def _dot(a,b):return sum(a[i]*b[i] for i in range(3))
def _cross(a,b):return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])


@dataclass(frozen=True)
class Triangle:
    vertices: tuple
    source: str
    object: int
    kind: str
    bounds: tuple


@dataclass(frozen=True)
class Hit:
    distance: float
    point: tuple
    normal: tuple
    triangle: Triangle


def _ray_box(origin,direction,bounds,limit):
    low,high=0.,limit
    for axis in range(3):
        if abs(direction[axis])<1e-15:
            if not bounds[axis]<=origin[axis]<=bounds[axis+3]:return False
        else:
            a,b=(bounds[axis]-origin[axis])/direction[axis],(bounds[axis+3]-origin[axis])/direction[axis]
            low=max(low,min(a,b));high=min(high,max(a,b))
            if low>high:return False
    return True


def _ray_triangle(origin,direction,triangle,limit):
    a,b,c=triangle.vertices;ab,ac=_sub(b,a),_sub(c,a);p=_cross(direction,ac);det=_dot(ab,p)
    if abs(det)<=1e-12*math.sqrt(_dot(ab,ab)*_dot(ac,ac)):return None
    offset=_sub(origin,a);u=_dot(offset,p)/det
    if u<-1e-9 or u>1+1e-9:return None
    q=_cross(offset,ab);v=_dot(direction,q)/det
    if v<-1e-9 or u+v>1+1e-9:return None
    distance=_dot(ac,q)/det
    if distance<0 or distance>limit:return None
    normal=_cross(ab,ac);norm=math.sqrt(_dot(normal,normal));normal=tuple(x/norm for x in normal)
    if _dot(normal,direction)>0:normal=tuple(-x for x in normal)
    return Hit(distance,tuple(origin[i]+distance*direction[i] for i in range(3)),normal,triangle)


def _overlap(a,b):return all(a[i]<=b[i+3] and b[i]<=a[i+3] for i in range(3))


def _triangle_box(triangle,bounds):
    # Separating axes: box normals, triangle normal, and all edge/box-axis cross products.
    center=tuple((bounds[i]+bounds[i+3])/2 for i in range(3));half=tuple((bounds[i+3]-bounds[i])/2 for i in range(3))
    points=tuple(_sub(p,center) for p in triangle.vertices)
    edges=tuple(_sub(points[(i+1)%3],points[i]) for i in range(3))
    basis=((1,0,0),(0,1,0),(0,0,1))
    for axis in (*basis,_cross(edges[0],edges[1]),*(_cross(e,b) for e in edges for b in basis)):
        if _dot(axis,axis)<1e-20:continue
        projection=tuple(_dot(p,axis) for p in points);radius=sum(half[i]*abs(axis[i]) for i in range(3))
        tolerance=1e-9*math.sqrt(_dot(axis,axis))
        if min(projection)>radius+tolerance or max(projection)<-radius-tolerance:return False
    return True


class _Node:
    def __init__(self,triangles):
        self.bounds=tuple(min(t.bounds[i] for t in triangles) for i in range(3))+tuple(max(t.bounds[i+3] for t in triangles) for i in range(3))
        self.children=();self.triangles=triangles
        if len(triangles)>8:
            axis=max(range(3),key=lambda i:self.bounds[i+3]-self.bounds[i]);ordered=sorted(triangles,key=lambda t:t.bounds[axis]+t.bounds[axis+3]);mid=len(ordered)//2
            self.children=(_Node(tuple(ordered[:mid])),_Node(tuple(ordered[mid:])));self.triangles=()


class CollisionWorld:
    def __init__(self,report):
        if not isinstance(report,dict) or report.get('mode')!='READ ONLY COLLISION GEOMETRY' or report.get('complete') is not True:
            raise ValueError('A complete collision export is required')
        self.position=_vector(report.get('position'));radius=report.get('radius_native')
        if type(radius) not in (int,float) or not math.isfinite(radius) or not 1000<=radius<=60000:raise ValueError('Invalid coverage')
        self.coverage=tuple(p-radius for p in self.position)+tuple(p+radius for p in self.position)
        rows=report.get('triangles')
        if not isinstance(rows,list) or len(rows)>50000:raise ValueError('Invalid triangle count')
        triangles=[]
        for row in rows:
            if not isinstance(row,dict) or row.get('source') not in ('SAT','EAT') or row.get('kind') not in ('floor','slope','wall','other') or type(row.get('object')) is not int or not 0<=row['object']<2048:
                raise ValueError('Invalid triangle owner')
            vertices=row.get('vertices')
            if not isinstance(vertices,list) or len(vertices)!=3:raise ValueError('Invalid triangle')
            points=tuple(_vector(p) for p in vertices);normal=_cross(_sub(points[1],points[0]),_sub(points[2],points[0]))
            if _dot(normal,normal)<1e-8:raise ValueError('Degenerate triangle')
            bounds=tuple(min(p[i] for p in points) for i in range(3))+tuple(max(p[i] for p in points) for i in range(3))
            triangles.append(Triangle(points,row['source'],row['object'],row['kind'],bounds))
        self.triangles=tuple(triangles);self.root=_Node(self.triangles) if triangles else None

    def _covered(self,bounds):
        if not all(self.coverage[i]<=bounds[i] and bounds[i+3]<=self.coverage[i+3] for i in range(3)):
            raise ValueError('Unknown space outside local export; refresh geometry before using it')

    def raycast(self,origin,direction,distance,kinds=None):
        origin,direction=_vector(origin),_vector(direction);length=math.sqrt(_dot(direction,direction))
        if length<1e-12 or type(distance) not in (int,float) or not math.isfinite(distance) or not 0<distance<=60000:raise ValueError('Invalid ray')
        direction=tuple(x/length for x in direction);end=tuple(origin[i]+direction[i]*distance for i in range(3))
        self._covered(tuple(min(origin[i],end[i]) for i in range(3))+tuple(max(origin[i],end[i]) for i in range(3)))
        best=None;pending=[self.root] if self.root else []
        while pending:
            node=pending.pop();limit=best.distance if best else distance
            if not _ray_box(origin,direction,node.bounds,limit):continue
            pending.extend(node.children)
            for triangle in node.triangles:
                if kinds is not None and triangle.kind not in kinds:continue
                hit=_ray_triangle(origin,direction,triangle,limit)
                if hit is not None and (best is None or hit.distance<best.distance):best=hit;limit=hit.distance
        return best

    def intersections(self,bounds,inset=0):
        if not isinstance(bounds,(tuple,list)) or len(bounds)!=6:raise ValueError('Invalid placement bounds')
        bounds=_vector(bounds[:3])+_vector(bounds[3:])
        if type(inset) not in (int,float) or not math.isfinite(inset) or not 0<=inset<=10 or any(bounds[i+3]-bounds[i]<=2*inset for i in range(3)):raise ValueError('Invalid placement extent')
        self._covered(bounds)
        tested=tuple(bounds[i]+inset for i in range(3))+tuple(bounds[i+3]-inset for i in range(3))
        result=[];pending=[self.root] if self.root else []
        while pending:
            node=pending.pop()
            if not _overlap(node.bounds,tested):continue
            pending.extend(node.children)
            result.extend(t for t in node.triangles if _overlap(t.bounds,tested) and _triangle_box(t,tested))
        return tuple(result)
