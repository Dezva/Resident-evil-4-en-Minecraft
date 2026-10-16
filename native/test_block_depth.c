#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "block_depth.h"
#include "block_shader.h"
static void check(float a,float b,unsigned reverse){
 float m[16]={.75,0,0,0,0,1,0,0,0,0,a,1,0,0,b,0};BlockProjection p;
 assert(block_projection(m,90,640,480,&p));assert(p.reverse==reverse);assert(fabsf(p.near_plane-10)<.001);
 assert(fabsf(block_depth(&p,.1f)-(reverse?1:0))<.0001);
 assert(fabsf(block_depth(&p,1.f/10000)-(reverse?0:1))<.0001);
 m[10]=-a;m[11]=-1;assert(block_projection(m,90,640,480,&p));
 m[5]=2;assert(!block_projection(m,90,640,480,&p));
}
int main(void){
 // Own synthetic VS3 program: full position = v0 * c0..c3, no game assets.
 uint32_t code[]={0xfffe0300,0x0200001f,0x80000000,0x900f0000,0x0200001f,0x80000000,0xe00f0000,
  0x03000005,0x800f0001,0xa0e40001,0x90550000,
  0x04000004,0x800f0001,0xa0e40000,0x90000000,0x80e40001,
  0x04000004,0x800f0001,0xa0e40002,0x90aa0000,0x80e40001,
  0x04000004,0xe00f0000,0xa0e40003,0x90ff0000,0x80e40001,0x0000ffff};
 assert(block_shader_wvp(code,sizeof(code)/4));
 for(unsigned i=0;i<sizeof(code)/4;i++){uint32_t saved=code[i];code[i]=0;assert(!block_shader_wvp(code,sizeof(code)/4));code[i]=saved;}
 check(10000.f/9990,-100000.f/9990,0);check(-10.f/9990,100000.f/9990,1);
 float gx[16]={.75,0,0,0,0,1,0,0,0,0,-10.f/9990,-100000.f/9990,0,0,-1,0};BlockProjection p;
 assert(block_gx_projection(gx,90,640,480,4,&p));assert(!p.reverse);assert(fabsf(block_depth(&p,.1))<.0001);
 assert(block_gx_projection(gx,90,640,480,7,&p));assert(p.reverse);assert(fabsf(block_depth(&p,.1)-1)<.0001);
 assert(!block_gx_projection(gx,90,640,480,8,&p));
 // Actual observed CAMERA matrix: virtual 4:3 at 1366x768, FOV50, near100/far1e6.
 float observed[16]={1.608380913734436f,0,0,0,0,2.144507884979248f,0,0,0,0,-0.00010001000191550702f,-100.01000213623047f,0,0,-1,0};
 assert(block_gx_projection(observed,49.99998092651367f,1366,768,4,&p));assert(!p.reverse);
 // Combined WVP: rotation, nonuniform object scale and world translation.
 // No standalone projection exists in these four GPU registers.
 float a=1.0005f,b=-75.0375f;
 float combined[16]={1,0,a*.6f,.6f, 0,2,a*-.8f,-.8f, 3,0,0,0, 2000,400,a*3000+b,3000};
 assert(block_wvp_projection(combined,observed,50,1366,768,4,&p));
 assert(fabsf(p.a-a)<.000001f && fabsf(p.b-b)<.001f && fabsf(p.near_plane-75)<.001f);
 assert(!block_shader_projection(combined,4,50,1366,768,4,&p));
 assert(!block_wvp_projection(combined,observed,50,1366,768,7,&p));
 combined[2]+=.1f;assert(!block_wvp_projection(combined,observed,50,1366,768,4,&p));combined[2]-=.1f;
 combined[3]=combined[7]=0;assert(!block_wvp_projection(combined,observed,50,1366,768,4,&p));
 assert(block_gx_projection(observed,50,1366,768,4,&p));
 assert(fabsf(p.near_plane-100)<.01f && p.far_plane>990000 && p.far_plane<1010000);
 assert(fabsf(block_depth(&p,.01f))<.00001f && fabsf(block_depth(&p,.000001f)-1)<.00001f);
 float bad[16];for(unsigned i=0;i<16;i++)bad[i]=observed[i];bad[0]=0;assert(!block_gx_projection(bad,50,1366,768,4,&p));
 bad[0]=.01f;assert(!block_gx_projection(bad,50,1366,768,4,&p));
 int32_t rect[4]={10,20,1376,788};assert(block_depth_full_clear(0,NULL,10,20,1366,768));assert(block_depth_full_clear(1,rect,10,20,1366,768));
 rect[2]--;assert(!block_depth_full_clear(1,rect,10,20,1366,768));assert(!block_depth_full_clear(2,rect,10,20,1366,768));assert(!block_depth_full_clear(1,NULL,10,20,1366,768));
 float constants[256*4]={0};float actual[16]={.75,0,0,0,0,1,0,0,0,0,10000.f/9990,-1,0,0,-100000.f/9990,0};
 // Right-handed D3D: A changes sign with clip W.
 actual[10]=-actual[10];for(unsigned i=0;i<16;i++)constants[48+i]=actual[i];
 assert(block_shader_projection(constants,256,90,640,480,4,&p));assert(fabsf(p.near_plane-10)<.001);
 assert(!block_shader_projection(constants,256,90,640,480,7,&p));
 memset(constants,0,sizeof(constants));for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++)constants[32+r*4+c]=actual[c*4+r];
 assert(block_shader_projection(constants,256,90,640,480,4,&p));
 memset(constants,0,sizeof(constants));assert(!block_shader_projection(constants,256,90,640,480,4,&p));
 for(unsigned c=1;c<=8;c++)assert(block_depth_world_compare(c)==(c==2 || c==4 || c==5 || c==7));
 assert(!block_projection(actual,NAN,640,480,&p));assert(!block_projection(actual,90,INFINITY,480,&p));
 gx[5]=NAN;assert(!block_gx_projection(gx,90,640,480,4,&p));
}
