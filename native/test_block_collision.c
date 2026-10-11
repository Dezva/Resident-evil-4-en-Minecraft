#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "block_collision.h"
static CubeCommand command;
static int run(const float *start,const float *end,float *out){return block_resolve_horizontal(&command,start,end,out);}
int main(void) {
    command.mode=2;command.count=1;float wall[6]={0,0,-500,1000,1000,500};memcpy(command.boxes[0],wall,sizeof(wall));
    float a[3]={-300,0,0},b[3]={100,10,100},out[3];assert(run(a,b,out)==1 && fabsf(out[0]+250.5f)<.001f && out[1]==10 && fabsf(out[2]-100)<.001f);
    float far[3]={1500,0,0};assert(run(a,far,out)==1 && out[0]<-250); // Whole-wall tunnelling test.
    float inside[3]={0,0,0},exit[3]={-500,0,0};assert(run(inside,exit,out)==0 && out[0]==-500); // Do not trap a newly enclosed player.
    a[1]=1001;b[1]=1001;assert(run(a,b,out)==0);a[1]=-1601;b[1]=-1601;assert(run(a,b,out)==0);
    a[1]=b[1]=0;command.count=2;float corner[6]={-500,0,0,500,1000,1000};memcpy(command.boxes[1],corner,sizeof(corner));
    a[2]=-300;b[2]=100;assert(run(a,b,out)==2 && out[0]<-250 && out[2]<-250);
    command.count=1;command.origin[0]=-100000;command.origin[2]=2000;command.origin[3]=1.5707963f;
    float world_a[3]={-100000,0,1700},world_b[3]={-100100,17,2100};assert(run(world_a,world_b,out)==1 && out[2]<1750 && out[1]==17);
    command.origin[3]=NAN;assert(run(a,b,out)==-1);command.origin[3]=0;command.origin[0]=command.origin[2]=0;
    command.boxes[0][3]=command.boxes[0][0];assert(run(a,b,out)==-1);memcpy(command.boxes[0],wall,sizeof(wall));
    for(unsigned i=0;i<10000;++i){a[0]=-300-(float)(i%1000);a[2]=(float)(i%1200)-600;b[0]=1500;b[2]=a[2];int hit=run(a,b,out);assert(hit>=0 && isfinite(out[0]+out[1]+out[2]));if(fabsf(a[2])<750)assert(hit==1 && out[0]<-250);}
    puts("Horizontal collision: swept walls, sliding, corners, body height, spawn escape, rigid grid and bounded cases passed");
}
