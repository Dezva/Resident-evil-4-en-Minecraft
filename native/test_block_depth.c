#include <assert.h>
#include "block_depth.h"
static void check(float a,float b,unsigned reverse){
 float m[16]={.75,0,0,0,0,1,0,0,0,0,a,1,0,0,b,0};BlockProjection p;
 assert(block_projection(m,90,640,480,&p));assert(p.reverse==reverse);assert(fabsf(p.near_plane-10)<.001);
 assert(fabsf(block_depth(&p,.1f)-(reverse?1:0))<.0001);
 assert(fabsf(block_depth(&p,1.f/10000)-(reverse?0:1))<.0001);
 m[10]=-a;m[11]=-1;assert(block_projection(m,90,640,480,&p));
 m[5]=2;assert(!block_projection(m,90,640,480,&p));
}
int main(void){
 check(10000.f/9990,-100000.f/9990,0);check(-10.f/9990,100000.f/9990,1);
 float gx[16]={.75,0,0,0,0,1,0,0,0,0,-10.f/9990,-100000.f/9990,0,0,-1,0};BlockProjection p;
 assert(block_gx_projection(gx,90,640,480,4,&p));assert(!p.reverse);assert(fabsf(block_depth(&p,.1))<.0001);
 assert(block_gx_projection(gx,90,640,480,7,&p));assert(p.reverse);assert(fabsf(block_depth(&p,.1)-1)<.0001);
 assert(!block_gx_projection(gx,90,640,480,8,&p));
 gx[5]=NAN;assert(!block_gx_projection(gx,90,640,480,4,&p));
}
