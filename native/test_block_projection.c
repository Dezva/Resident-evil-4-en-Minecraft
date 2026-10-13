#include <assert.h>
#include "block_depth.h"
#include "block_surface.h"
int main(void){
 float gx[16]={1.608380913734436f,0,0,0,0,2.144507884979248f,0,0,0,0,-0.00010001000191550702f,-100.01000213623047f,0,0,-1,0};
 BlockProjection p;assert(block_gx_projection(gx,50,1366,768,4,&p));
 float view[12]={1,0,0,0,0,1,0,0,0,0,1,0};
 float box[1][6]={{-100,-100,-2000,100,100,-1000}};BlockSurfaceMesh *mesh=malloc(sizeof(*mesh));assert(mesh);
 float fx=p.matrix[0]*1366/2,fy=p.matrix[5]*768/2;
 assert(fabsf(fx-fy)<.001f); // A frontal unit square must stay square in widescreen.
 unsigned n=block_surfaces_materials_projected(box,1,view,50,1366,768,mesh,NULL,p.near_plane,fx);assert(n==6);
 for(unsigned i=0;i<n;i++){
  assert(fabsf(fabsf(mesh->vertices[i].x-683)-fx*.1f)<.001f);
  assert(fabsf(fabsf(mesh->vertices[i].y-384)-fy*.1f)<.001f);
  assert(block_depth(&p,mesh->vertices[i].rhw)>=0 && block_depth(&p,mesh->vertices[i].rhw)<=1);
 }
 BlockVertex lines[24];n=block_lines_projected(box[0],view,50,1366,768,lines,fx);assert(n==24);
 for(unsigned i=0;i<n;i++)assert(fabsf(fabsf(lines[i].x-683)-fx*.1f)<.001f || fabsf(fabsf(lines[i].x-683)-fx*.05f)<.001f);
 box[0][0]=500;box[0][3]=900;n=block_surfaces_materials_projected(box,1,view,50,1366,768,mesh,NULL,p.near_plane,fx);assert(n>0);
 for(unsigned i=0;i<n;i++)assert(mesh->vertices[i].x>=0 && mesh->vertices[i].x<=1366 && mesh->vertices[i].y>=0 && mesh->vertices[i].y<=768);
 assert(!block_surfaces_materials_projected(box,1,view,50,1366,768,mesh,NULL,p.near_plane,NAN));
 free(mesh);
}
