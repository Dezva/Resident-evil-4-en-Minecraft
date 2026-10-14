#ifndef RE4_BLOCK_DEBRIS_H
#define RE4_BLOCK_DEBRIS_H
#include <math.h>
#include <string.h>
#include "cube_policy.h"
/* Render-only prediction between 20 Hz snapshots. Raw collider bounds and
   shared memory are never modified. A fresh snapshot owns fragment lifetime. */
typedef struct {
    unsigned valid,count,colliders;
    uint32_t stamp,epoch,generation,identity[CUBE_MAX_BOXES];
    float origin[4],boxes[CUBE_MAX_BOXES][6],velocity[CUBE_MAX_BOXES][3];
} BlockDebris;
static inline uint32_t block_debris_identity(const CubeCommand *c,unsigned i){
    uint32_t hash=2166136261u;
    for(unsigned face=0;face<6;face++){
        const BlockFaceMaterial *m=&c->materials[i][face];
        if(m->tile>=c->tile_count || m->tile>=BLOCK_MAX_TILES)return 0;
        const unsigned char *uv=(const unsigned char*)m->uv;
        for(unsigned j=0;j<sizeof(m->uv);j++)hash=(hash^uv[j])*16777619u;
        for(unsigned j=0;j<256;j++)hash=(hash^c->pixels[m->tile][j])*16777619u;
    }
    return hash;
}
static inline void block_debris_predict(BlockDebris *d,CubeCommand *c,uint32_t now){
    unsigned colliders=block_collision_count(c);
    if(c->mode!=2 || c->style!=3 || c->count>CUBE_MAX_BOXES || colliders>=c->count){d->valid=0;return;}
    if(!d->valid || d->stamp!=c->stamp || d->epoch!=c->epoch || d->generation!=c->arm_generation){
        uint32_t interval=c->stamp-d->stamp;
        int compatible=d->valid && interval>=10 && interval<=100 && d->count==c->count
            && d->colliders==colliders && d->epoch==c->epoch && d->generation==c->arm_generation
            && !memcmp(d->origin,c->origin,sizeof(d->origin));
        for(unsigned i=colliders;i<c->count;i++){
            uint32_t identity=block_debris_identity(c,i);
            int same=compatible && identity && identity==d->identity[i];
            for(unsigned a=0;a<3;a++){
                float size=c->boxes[i][a+3]-c->boxes[i][a];
                float previous_size=d->boxes[i][a+3]-d->boxes[i][a];
                float delta=c->boxes[i][a]-d->boxes[i][a];
                if(!isfinite(delta+size) || size<=0 || fabsf(size-previous_size)>.001f
                    || fabsf(delta)>4*size)same=0;
            }
            for(unsigned a=0;a<3;a++)d->velocity[i][a]=same?(c->boxes[i][a]-d->boxes[i][a])/interval:0;
            memcpy(d->boxes[i],c->boxes[i],sizeof(d->boxes[i]));d->identity[i]=identity;
        }
        d->valid=1;d->count=c->count;d->colliders=colliders;d->epoch=c->epoch;
        d->generation=c->arm_generation;d->stamp=c->stamp;memcpy(d->origin,c->origin,sizeof(d->origin));
    }
    uint32_t age=now-c->stamp;
    /* Do not let a delayed packet keep debris flying or extend its lease. */
    float elapsed=age<100?(float)(age>50?50:age):0;
    for(unsigned i=colliders;i<c->count;i++)for(unsigned a=0;a<3;a++){
        float delta=d->velocity[i][a]*elapsed;
        c->boxes[i][a]+=delta;c->boxes[i][a+3]+=delta;
    }
}
#endif
