#include "block_geometry.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    float bounds[6]={-500,0,-3500,500,1000,-2500};
    float view[12]={1,0,0,0,0,1,0,0,0,0,1,0};BlockVertex vertices[24];
    assert(block_lines(bounds,view,50,1920,1080,vertices)==24);
    float left=vertices[0].x;view[3]=-100;
    assert(block_lines(bounds,view,50,1920,1080,vertices)==24);
    assert(vertices[0].x<left); /* Fixed world block shifts left when viewer moves right. */
    assert(bounds[0]==-500 && bounds[5]==-2500); /* No anchor follows player updates. */
    for (unsigned i=0;i<24;++i) assert(vertices[i].rhw==1 && vertices[i].x>=0 && vertices[i].x<=1920);
    view[3]=0;bounds[2]=2500;bounds[5]=3500;
    assert(block_lines(bounds,view,50,1920,1080,vertices)==0); /* Behind viewer. */
    bounds[0]=40;bounds[3]=80;bounds[1]=40;bounds[4]=80;bounds[2]=-500;bounds[5]=500;
    unsigned n=block_lines(bounds,view,50,1920,1080,vertices);
    assert(n>0 && n<=24 && !(n&1)); /* Crosses near plane; no behind-camera mirroring. */
    for (unsigned i=0;i<n;++i) assert(isfinite(vertices[i].x+vertices[i].y));
    float a[2]={-100,50},b[2]={200,50};assert(block_clip_line(a,b,100,100));
    assert(a[0]==0 && b[0]==100);
    a[0]=a[1]=-100;b[0]=b[1]=-50;assert(!block_clip_line(a,b,100,100));
    assert(!block_lines(bounds,view,NAN,1920,1080,vertices));
    view[4]=1;view[5]=0;assert(!block_lines(bounds,view,50,1920,1080,vertices));
    bounds[3]=NAN;assert(!block_valid_bounds(bounds));
    float camera[12]={1,0,0,-10,0,1,0,-20,0,0,1,-30},composed[12];
    float origin[4]={10,20,30,1.57079632679f};
    assert(block_scene_view(camera,origin,composed));
    // Translating to the camera origin removes translation; +X rotates to +Z.
    assert(fabsf(composed[3])<.001f && fabsf(composed[7])<.001f && fabsf(composed[11])<.001f);
    assert(fabsf(composed[0])<.001f && fabsf(composed[8]-1)<.001f && fabsf(composed[2]+1)<.001f);
    origin[3]=NAN;assert(!block_scene_view(camera,origin,composed));
    float thin[6]={0,0,-600,10,500,-500};assert(block_valid_shape_bounds(thin) && !block_valid_bounds(thin));
    thin[3]=0;assert(!block_valid_shape_bounds(thin));
    puts("Block geometry: fixed anchor, near-plane/viewport clipping, behind camera and finite bounds passed");
}
