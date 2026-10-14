#include <assert.h>
#include <stdio.h>
#include "block_debris.h"
static CubeCommand raw;
static CubeCommand snapshot(uint32_t stamp,float x){
    CubeCommand c=raw;c.stamp=stamp;c.boxes[1][0]=x;c.boxes[1][3]=x+90;return c;
}
int main(void){
    raw=(CubeCommand){.mode=2,.style=3,.count=2,.material_reserved=2,.tile_count=1,.epoch=7,.arm_generation=9};
    float solid[6]={0,0,0,1000,1000,1000},particle[6]={0,1100,0,90,1190,90};
    memcpy(raw.boxes[0],solid,sizeof(solid));memcpy(raw.boxes[1],particle,sizeof(particle));
    for(unsigned j=0;j<256;j++)raw.pixels[0][j]=0xff887766;
    BlockDebris d={0};CubeCommand c=snapshot(1000,0);block_debris_predict(&d,&c,1000);
    c=snapshot(1050,50);block_debris_predict(&d,&c,1050);assert(c.boxes[1][0]==50);
    for(unsigned age=1;age<50;age++){
        c=snapshot(1050,50);block_debris_predict(&d,&c,1050+age);
        assert(c.boxes[1][0]==50+age && c.boxes[1][3]==140+age);
        assert(!memcmp(c.boxes[0],solid,sizeof(solid)));assert(raw.boxes[1][0]==0);
    }
    c=snapshot(1100,100);block_debris_predict(&d,&c,1100);assert(c.boxes[1][0]==100); // continuous at next snapshot
    c=snapshot(1100,100);block_debris_predict(&d,&c,1160);assert(c.boxes[1][0]==150); // bounded prediction
    c=snapshot(1100,100);block_debris_predict(&d,&c,1250);assert(c.boxes[1][0]==100); // stale command not animated
    c=snapshot(1150,150);c.arm_generation++;block_debris_predict(&d,&c,1175);assert(c.boxes[1][0]==150);
    c=snapshot(1200,200);c.pixels[0][0]++;block_debris_predict(&d,&c,1225);assert(c.boxes[1][0]==200);
    c=snapshot(1250,10000);block_debris_predict(&d,&c,1275);assert(c.boxes[1][0]==10000); // replacement is not extrapolated
    c=snapshot(1300,100);c.count=1;block_debris_predict(&d,&c,1325);assert(!d.valid); // expired suffix
    c=snapshot(0xfffffff0u,0);block_debris_predict(&d,&c,c.stamp);
    c=snapshot(34,50);block_debris_predict(&d,&c,59);assert(c.boxes[1][0]==75); // uint32 clock wrap
    puts("Debris: smooth render frames, solid prefix unchanged, expiry/generation/material/replacement and clock wrap passed");
}
