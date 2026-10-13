#ifndef RE4_BLOCK_SURFACE_H
#define RE4_BLOCK_SURFACE_H
#include "block_geometry.h"
#include "block_material.h"
#include <stdlib.h>
#include <string.h>
/* A clipped convex quad gains at most one vertex per five frustum planes.
 * 9 vertices -> 7 triangles. Memory and draw size remain explicitly bounded. */
#define BLOCK_SURFACE_MAX_BOXES 128u
#define BLOCK_SURFACE_MAX_FACES (6u*BLOCK_SURFACE_MAX_BOXES)
#define BLOCK_SURFACE_MAX_VERTICES (21u*BLOCK_SURFACE_MAX_FACES)
typedef struct {float v[3],uv[2];} BlockPoint;
typedef struct {float x,y,z,rhw;uint32_t color;float u,v;} BlockSurfaceVertex;
_Static_assert(sizeof(BlockSurfaceVertex)==28,"Textured surface vertex layout");
typedef struct {unsigned box,axis,side;float depth;} BlockFace;
typedef struct {
    BlockFace faces[BLOCK_SURFACE_MAX_FACES];
    BlockSurfaceVertex vertices[BLOCK_SURFACE_MAX_VERTICES];
} BlockSurfaceMesh;
static inline BlockPoint block_transform(const float *view,const float *point) {
    BlockPoint result={{0},{0}};
    for (unsigned r=0;r<3;++r) {
        result.v[r]=view[4*r+3];
        for (unsigned c=0;c<3;++c) result.v[r]+=view[4*r+c]*point[c];
    }
    return result;
}
static inline float block_plane_distance(BlockPoint point,unsigned plane,float horizontal,float vertical) {
    switch (plane) {
    case 0:return -point.v[2]-1.f;
    case 1:return point.v[0]-point.v[2]*horizontal;
    case 2:return -point.v[0]-point.v[2]*horizontal;
    case 3:return point.v[1]-point.v[2]*vertical;
    default:return -point.v[1]-point.v[2]*vertical;
    }
}
static inline unsigned block_clip_face_near(BlockPoint *points,unsigned count,float horizontal,float vertical,float near_plane) {
    BlockPoint temporary[12];
    for (unsigned plane=0;plane<5 && count;++plane) {
        unsigned output=0;BlockPoint previous=points[count-1];
        float prior=plane?block_plane_distance(previous,plane,horizontal,vertical):-previous.v[2]-near_plane;
        for (unsigned i=0;i<count;++i) {
            float current=plane?block_plane_distance(points[i],plane,horizontal,vertical):-points[i].v[2]-near_plane;
            if ((prior>=0)!=(current>=0)) {
                if (output==12) return 0;
                float ratio=prior/(prior-current);
                for (unsigned axis=0;axis<3;++axis)
                    temporary[output].v[axis]=previous.v[axis]+ratio*(points[i].v[axis]-previous.v[axis]);
                for (unsigned axis=0;axis<2;++axis) temporary[output].uv[axis]=previous.uv[axis]+ratio*(points[i].uv[axis]-previous.uv[axis]);
                output++;
            }
            if (current>=0) {if (output==12) return 0;temporary[output++]=points[i];}
            previous=points[i];prior=current;
        }
        memcpy(points,temporary,output*sizeof(*points));count=output;
    }
    return count<=9?count:0;
}
static inline unsigned block_clip_face(BlockPoint *points,unsigned count,float horizontal,float vertical){return block_clip_face_near(points,count,horizontal,vertical,1.f);}
static int block_face_far_first(const void *a,const void *b) {
    const BlockFace *left=a,*right=b;
    if (left->depth!=right->depth) return left->depth>right->depth?-1:1;
    unsigned l=left->box*6+left->axis*2+left->side,r=right->box*6+right->axis*2+right->side;
    return l<r?-1:l>r?1:0;
}
static inline int block_face_covered(const float boxes[][6],unsigned count,unsigned index,unsigned axis,unsigned side) {
    const float *box=boxes[index];float plane=box[axis+(side?3:0)];
    for (unsigned i=0;i<count;++i) {
        if (i==index || fabsf(boxes[i][axis+(side?0:3)]-plane)>.01f) continue;
        unsigned covered=1;
        for (unsigned other=0;other<3;++other) if (other!=axis
                && (boxes[i][other]>box[other]+.01f || boxes[i][other+3]<box[other+3]-.01f)) covered=0;
        if (covered) return 1;
    }
    return 0;
}
/* Build clipped XY/RHW geometry. The caller supplies validated X projection
 * and assigns native Z; legacy diagnostic drawing retains its original focal. */
static inline unsigned block_surfaces_materials_projected(const float boxes[][6],unsigned box_count,const float *view,
        float fovy,float width,float height,BlockSurfaceMesh *mesh,const BlockFaceMaterial materials[][6],float near_plane,float focal_x) {
    if (!box_count || box_count>BLOCK_SURFACE_MAX_BOXES || !block_valid_view(view,fovy,width,height)) return 0;
    for (unsigned i=0;i<box_count;++i) if (!block_valid_shape_bounds(boxes[i])) return 0;
    float eye[3]={0};
    for (unsigned axis=0;axis<3;++axis) for (unsigned row=0;row<3;++row)
        eye[axis]-=view[4*row+axis]*view[4*row+3];
    unsigned face_count=0;
    for (unsigned i=0;i<box_count;++i) for (unsigned axis=0;axis<3;++axis) for (unsigned side=0;side<2;++side) {
        const float *box=boxes[i];float plane=box[axis+(side?3:0)];
        if (side?eye[axis]<=plane:eye[axis]>=plane) continue;
        if (!materials && block_face_covered(boxes,box_count,i,axis,side)) continue;
        float center[3];for (unsigned c=0;c<3;++c) center[c]=(box[c]+box[c+3])*.5f;
        center[axis]=plane;BlockPoint projected=block_transform(view,center);
        mesh->faces[face_count++]=(BlockFace){i,axis,side,-projected.v[2]};
    }
    qsort(mesh->faces,face_count,sizeof(BlockFace),block_face_far_first);
    float vertical=tanf(fovy*0.00872664625997165f),focal=height/(2*vertical);
    if(!isfinite(focal_x) || focal_x<0 || focal_x>100000000.f)return 0;
    if(focal_x==0)focal_x=focal;
    float horizontal=width/(2*focal_x);unsigned output=0;
    static const uint32_t colors[3][2]={{0xFF858585,0xFF737373},{0xFF606060,0xFFB8B8B8},{0xFF969696,0xFF808080}};
    static const unsigned corners[4][2]={{0,0},{1,0},{1,1},{0,1}};
    for (unsigned i=0;i<face_count;++i) {
        BlockFace face=mesh->faces[i];const float *box=boxes[face.box];
        unsigned u=(face.axis+1)%3,v=(face.axis+2)%3;BlockPoint polygon[12];
        for (unsigned corner=0;corner<4;++corner) {
            float point[3];point[face.axis]=box[face.axis+(face.side?3:0)];
            point[u]=box[u+(corners[corner][0]?3:0)];point[v]=box[v+(corners[corner][1]?3:0)];
            polygon[corner]=block_transform(view,point);
            polygon[corner].uv[0]=(point[u]-box[u])/(box[u+3]-box[u]);
            polygon[corner].uv[1]=(box[v+3]-point[v])/(box[v+3]-box[v]);
            if(materials) {
                BlockFaceMaterial material=materials[face.box][face.axis*2+face.side];
                if(material.tile>=BLOCK_MAX_TILES || !isfinite(material.uv[corner*2]) || !isfinite(material.uv[corner*2+1])
                    || material.uv[corner*2]<0 || material.uv[corner*2]>1 || material.uv[corner*2+1]<0 || material.uv[corner*2+1]>1)return 0;
                polygon[corner].uv[0]=((material.tile%8)*32+8.5f+15*material.uv[corner*2])/256.f;
                polygon[corner].uv[1]=((material.tile/8)*32+8.5f+15*material.uv[corner*2+1])/256.f;
            }
        }
        unsigned n=block_clip_face_near(polygon,4,horizontal,vertical,near_plane);
        if (n<3) continue;
        BlockSurfaceVertex projected[9];
        for (unsigned corner=0;corner<n;++corner) {
            float depth=fmaxf(near_plane,-polygon[corner].v[2]);
            float x=width/2+focal_x*polygon[corner].v[0]/depth,y=height/2-focal*polygon[corner].v[1]/depth;
            if (!isfinite(x+y)) return 0;
            projected[corner]=(BlockSurfaceVertex){fminf(width,fmaxf(0,x)),fminf(height,fmaxf(0,y)),0,1.f/depth,colors[face.axis][face.side],polygon[corner].uv[0],polygon[corner].uv[1]};
        }
        for (unsigned corner=1;corner+1<n;++corner) {
            float area=(projected[corner].x-projected[0].x)*(projected[corner+1].y-projected[0].y)
                    -(projected[corner].y-projected[0].y)*(projected[corner+1].x-projected[0].x);
            if (fabsf(area)<.0001f) continue;
            if (output+3>BLOCK_SURFACE_MAX_VERTICES) return 0;
            mesh->vertices[output++]=projected[0];mesh->vertices[output++]=projected[corner];mesh->vertices[output++]=projected[corner+1];
        }
    }
    return output;
}
static inline unsigned block_surfaces_materials_near(const float boxes[][6],unsigned count,const float *view,float fovy,float width,float height,BlockSurfaceMesh *mesh,const BlockFaceMaterial materials[][6],float near_plane){return block_surfaces_materials_projected(boxes,count,view,fovy,width,height,mesh,materials,near_plane,0);}
static inline unsigned block_surfaces_materials(const float boxes[][6],unsigned count,const float *view,float fovy,float width,float height,BlockSurfaceMesh *mesh,const BlockFaceMaterial materials[][6]){return block_surfaces_materials_near(boxes,count,view,fovy,width,height,mesh,materials,1.f);}
static inline unsigned block_surfaces(const float boxes[][6],unsigned count,const float *view,float fovy,float width,float height,BlockSurfaceMesh *mesh) {
    return block_surfaces_materials(boxes,count,view,fovy,width,height,mesh,NULL);
}
#endif
