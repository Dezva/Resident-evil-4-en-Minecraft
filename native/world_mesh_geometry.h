#ifndef RE4_WORLD_MESH_GEOMETRY_H
#define RE4_WORLD_MESH_GEOMETRY_H
#include "block_surface.h"
#include "block_depth.h"
#define WORLD_DRAW_VERTICES (128u*21u)
static unsigned world_mesh_project(const WorldVertex *input,unsigned quads,const float *view,const BlockProjection *depth,const D3DVIEWPORT9 *vp,BlockSurfaceVertex *output){
 if(!quads || quads>128 || !depth->valid || !vp->Width || !vp->Height)return 0;
 float fx=depth->matrix[0]*vp->Width*.5f,fy=depth->matrix[5]*vp->Height*.5f;
 if(fx<=0 || fy<=0)return 0;
 unsigned count=0;
 for(unsigned q=0;q<quads;q++){
  BlockPoint polygon[12];
  for(unsigned c=0;c<4;c++){polygon[c]=block_transform(view,input[q*4+c].xyz);memcpy(polygon[c].uv,input[q*4+c].uv,8);}
  unsigned n=block_clip_face_near(polygon,4,vp->Width/(2*fx),vp->Height/(2*fy),depth->near_plane);
  BlockSurfaceVertex v[9];for(unsigned c=0;c<n;c++){
   float rhw=1.f/fmaxf(depth->near_plane,-polygon[c].v[2]);
   v[c]=(BlockSurfaceVertex){vp->X+vp->Width*.5f+fx*polygon[c].v[0]*rhw,vp->Y+vp->Height*.5f-fy*polygon[c].v[1]*rhw,vp->MinZ+(vp->MaxZ-vp->MinZ)*block_depth(depth,rhw),rhw,input[q*4].color,polygon[c].uv[0],polygon[c].uv[1]};
  }
  for(unsigned c=1;c+1<n;c++){if(count+3>WORLD_DRAW_VERTICES)return 0;output[count++]=v[0];output[count++]=v[c];output[count++]=v[c+1];}
 }
 return count;
}
#endif
