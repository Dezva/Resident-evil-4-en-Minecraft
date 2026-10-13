#include <assert.h>
#include <stddef.h>
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
 // Actual observed CAMERA matrix: virtual 4:3 at 1366x768, FOV50, near100/far1e6.
 float observed[16]={1.608380913734436f,0,0,0,0,2.144507884979248f,0,0,0,0,-0.00010001000191550702f,-100.01000213623047f,0,0,-1,0};
 assert(block_gx_projection(observed,49.99998092651367f,1366,768,4,&p));assert(!p.reverse);
 assert(fabsf(p.near_plane-100)<.01f && p.far_plane>990000 && p.far_plane<1010000);
 assert(fabsf(block_depth(&p,.01f))<.00001f && fabsf(block_depth(&p,.000001f)-1)<.00001f);
 float bad[16];for(unsigned i=0;i<16;i++)bad[i]=observed[i];bad[0]=0;assert(!block_gx_projection(bad,50,1366,768,4,&p));
 bad[0]=.01f;assert(!block_gx_projection(bad,50,1366,768,4,&p));
 int32_t rect[4]={10,20,1376,788};assert(block_depth_full_clear(0,NULL,10,20,1366,768));assert(block_depth_full_clear(1,rect,10,20,1366,768));
 rect[2]--;assert(!block_depth_full_clear(1,rect,10,20,1366,768));assert(!block_depth_full_clear(2,rect,10,20,1366,768));assert(!block_depth_full_clear(1,NULL,10,20,1366,768));
 gx[5]=NAN;assert(!block_gx_projection(gx,90,640,480,4,&p));
}
