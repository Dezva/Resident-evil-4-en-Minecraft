#include <assert.h>
#include <stdio.h>
#include "block_surface.h"
static void valid(const BlockSurfaceMesh *mesh,unsigned n) {
    assert(n<=BLOCK_SURFACE_MAX_VERTICES && n%3==0);
    for(unsigned i=0;i<n;++i) {
        BlockSurfaceVertex v=mesh->vertices[i];assert(isfinite(v.x+v.y) && v.x>=0 && v.x<=640 && v.y>=0 && v.y<=480);
        assert(v.z==0 && isfinite(v.rhw) && v.rhw>0 && v.rhw<=1 && v.u>=0 && v.u<=1 && v.v>=0 && v.v<=1 && (v.color>>24)==255);
    }
}
static float area(const BlockSurfaceMesh *mesh,unsigned n) {
    float total=0;
    for(unsigned i=0;i<n;i+=3) {BlockSurfaceVertex a=mesh->vertices[i],b=mesh->vertices[i+1],c=mesh->vertices[i+2];total+=fabsf((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x))*.5f;}
    return total;
}
int main(void) {
    BlockSurfaceMesh *mesh=malloc(sizeof(*mesh));assert(mesh);
    BlockPoint clipped[12]={{{-1,-1,-.5f},{0,0}},{{1,-1,-1.5f},{1,0}},{{1,1,-1.5f},{1,1}},{{-1,1,-.5f},{0,1}}};
    unsigned clipped_count=block_clip_face(clipped,4,10,10),intersections=0;assert(clipped_count==4);
    for(unsigned i=0;i<clipped_count;++i) if(clipped[i].v[2]==-1) {assert(fabsf(clipped[i].uv[0]-.5f)<.00001f);intersections++;}
    assert(intersections==2); // UVs must be clipped in camera space before perspective division.
    float view[12]={1,0,0,0,0,1,0,0,0,0,1,0};
    float boxes[128][6]={{-50,-50,-650,50,50,-550},{-50,-50,-1250,50,50,-1150}};
    unsigned n=block_surfaces(boxes,1,view,90,640,480,mesh);assert(n==6);valid(mesh,n);
    assert(mesh->vertices[0].rhw==1.f/550.f);
    float full=area(mesh,n);boxes[0][4]=0;n=block_surfaces(boxes,1,view,90,640,480,mesh);assert(n==6 && fabsf(area(mesh,n)-full*.5f)<.01f);boxes[0][4]=50;
    n=block_surfaces(boxes,2,view,90,640,480,mesh);assert(n==12 && mesh->faces[0].box==1 && mesh->faces[1].box==0);valid(mesh,n);
    n=block_surfaces(boxes,1,view,90,640,480,mesh);assert(n==6); // Removed far cube is absent.
    float adjacent[2][6]={{-100,-100,-200,0,0,-100},{0,-100,-200,100,0,-100}};
    assert(block_face_covered(adjacent,2,0,0,1));assert(block_face_covered(adjacent,2,1,0,0));assert(!block_face_covered(adjacent,1,0,0,1));
    view[3]=70;view[7]=-100;n=block_surfaces(boxes,1,view,90,640,480,mesh);assert(n==18);valid(mesh,n);
    int top=0,side=0;for(unsigned i=0;i<n;++i){top|=mesh->vertices[i].color==0xFFB8B8B8;side|=mesh->vertices[i].color==0xFF858585;}assert(top && side);
    view[3]=view[7]=0;
    float near[128][6];for(unsigned i=0;i<128;++i){float b[6]={2,-100,-100,102,100,1};memcpy(near[i],b,sizeof(b));}
    n=block_surfaces(near,128,view,90,640,480,mesh);assert(n>0);valid(mesh,n);
    float huge[1][6]={{-3750,-3750,-2,3750,3750,-1}};n=block_surfaces(huge,1,view,90,640,480,mesh);assert(n==6);valid(mesh,n);assert(fabsf(area(mesh,n)-640*480)<100);
    assert(!block_surfaces(boxes,0,view,90,640,480,mesh));assert(!block_surfaces(boxes,129,view,90,640,480,mesh));
    boxes[0][0]=NAN;assert(!block_surfaces(boxes,1,view,90,640,480,mesh));boxes[0][0]=-50;
    view[0]=2;assert(!block_surfaces(boxes,1,view,90,640,480,mesh));view[0]=1;
    for(unsigned i=0;i<2000;++i){
        float angle=(float)i*.137f;view[0]=cosf(angle);view[2]=sinf(angle);view[8]=-sinf(angle);view[10]=cosf(angle);
        for(unsigned j=0;j<128;++j){float x=(float)((i*97+j*73)%20000)-10000,y=(float)((i*71+j*83)%20000)-10000,z=(float)((i*47+j*67)%20000)-10000;float b[6]={x,y,z,x+1000,y+500,z+1000};memcpy(near[j],b,sizeof(b));}
        n=block_surfaces(near,128,view,90,640,480,mesh);valid(mesh,n);
    }
    free(mesh);puts("Filled faces: clipping, shading, far-first ordering, internal faces, slabs, removal and bounded randomized geometry passed");
}
