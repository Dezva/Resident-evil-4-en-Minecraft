#ifndef RE4_BLOCK_COLLISION_H
#define RE4_BLOCK_COLLISION_H
#include "cube_policy.h"
#include "block_geometry.h"
/* Upright conservative square footprint. Horizontal response only; native Y/ground is preserved. */
static inline int block_collision_scene_valid(const CubeCommand *command) {
    if(command->mode!=2 || !block_collision_count(command) || command->count>CUBE_MAX_BOXES || block_collision_count(command)>command->count)return 0;
    for(unsigned i=0;i<4;++i)if(!isfinite(command->origin[i]) || fabsf(command->origin[i])>(i==3?3.141593f:20000000.f))return 0;
    for(unsigned i=0;i<block_collision_count(command);++i)if(!block_valid_shape_bounds(command->boxes[i]))return 0;
    return 1;
}
static inline void block_local_point(const float *origin,const float *point,float *local) {
    float x=point[0]-origin[0],z=point[2]-origin[2],c=cosf(origin[3]),s=sinf(origin[3]);
    local[0]=c*x+s*z;local[1]=point[1]-origin[1];local[2]=-s*x+c*z;
}
static inline int block_sweep_horizontal_skin(const float *box,const float *start,const float *delta,float radius,float height,float skin,float *time,unsigned *axis,float *normal) {
    const unsigned axes[2]={0,2};float low[3]={box[0]-radius,box[1]-height,box[2]-radius},high[3]={box[3]+radius,box[4],box[5]+radius};
    if(start[0]>low[0]+.1f && start[0]<high[0]-.1f && start[2]>low[2]+.1f && start[2]<high[2]-.1f && start[1]>low[1] && start[1]<high[1]-skin)return 0;
    float entry=-INFINITY,exit=INFINITY,sign=0;unsigned hit_axis=0;
    for(unsigned i=0;i<2;++i) {
        unsigned a=axes[i];
        if(fabsf(delta[a])<.00001f){if(start[a]<=low[a] || start[a]>=high[a])return 0;continue;}
        float t_enter=(low[a]-start[a])/delta[a],t_exit=(high[a]-start[a])/delta[a],n=-1;
        if(t_enter>t_exit){float swap=t_enter;t_enter=t_exit;t_exit=swap;n=1;}
        if(t_enter>entry){entry=t_enter;hit_axis=a;sign=n;}if(t_exit<exit)exit=t_exit;
    }
    if(!isfinite(entry) || entry<-.00001f || entry>1 || exit<entry || exit<0)return 0;
    entry=fmaxf(0,entry);float y=start[1]+delta[1]*entry;
    if(y<=low[1]+.1f || y>=high[1]-skin)return 0;
    *time=entry;*axis=hit_axis;*normal=sign;return 1;
}
/* Sweeps rather than checking only the final position, so sprint cannot tunnel through a wall. */
static inline int block_resolve_horizontal_actor_skin(const CubeCommand *command,const float *before,const float *after,float *result,float radius,float height,float skin) {
    if(!isfinite(radius+height+skin) || radius<100 || radius>1000 || height<300 || height>3000 || skin<.1f || skin>25)return -1;
    float start[3],end[3],delta[3];block_local_point(command->origin,before,start);block_local_point(command->origin,after,end);
    for(unsigned i=0;i<3;++i){if(!isfinite(before[i]+after[i]) || fabsf(before[i])>20000000.f || fabsf(after[i])>20000000.f)return -1;delta[i]=end[i]-start[i];}
    int contacts=0;
    for(unsigned iteration=0;iteration<4;++iteration) {
        float earliest=1,normal=0;unsigned axis=0;int found=0;
        for(unsigned i=0;i<block_collision_count(command);++i){float t,n;unsigned a;if(block_sweep_horizontal_skin(command->boxes[i],start,delta,radius,height,skin,&t,&a,&n) && (!found || t<earliest)){earliest=t;axis=a;normal=n;found=1;}}
        if(!found){for(unsigned i=0;i<3;++i)start[i]+=delta[i];break;}
        for(unsigned i=0;i<3;++i){start[i]+=delta[i]*earliest;delta[i]*=1-earliest;}
        start[axis]+=normal*.5f;delta[axis]=0;contacts++;
    }
    float c=cosf(command->origin[3]),s=sinf(command->origin[3]);
    result[0]=command->origin[0]+c*start[0]-s*start[2];result[2]=command->origin[2]+s*start[0]+c*start[2];result[1]=after[1];return contacts;
}
static inline int block_resolve_horizontal_actor_validated(const CubeCommand *command,const float *before,const float *after,float *result,float radius,float height){return block_resolve_horizontal_actor_skin(command,before,after,result,radius,height,.1f);}
static inline int block_depenetrate_horizontal_skin(const CubeCommand *command,float *world,float radius,float height,float skin){
    float p[3];block_local_point(command->origin,world,p);
    unsigned count=block_collision_count(command);int inside=0;
    for(unsigned i=0;i<count;i++){
        const float *b=command->boxes[i];
        if(p[1]>b[1]-height+.1f && p[1]<b[4]-skin && p[0]>b[0]-radius && p[0]<b[3]+radius && p[2]>b[2]-radius && p[2]<b[5]+radius){inside=1;break;}
    }
    if(!inside)return 0;
    float best=INFINITY,endpoint=0;unsigned best_axis=0;int contacts=0;
    /* Find the nearest exit from the connected union along four horizontal
       rays. A per-box minimum can oscillate forever between adjacent blocks. */
    for(unsigned direction=0;direction<4;direction++){
        unsigned axis=direction<2?0:2,other=axis==0?2:0;float candidate=p[axis];int hits=0;
        for(unsigned pass=0;pass<=count;pass++){
            int changed=0;
            for(unsigned i=0;i<count;i++){
                const float *b=command->boxes[i];
                if(p[1]<=b[1]-height+.1f || p[1]>=b[4]-skin || p[other]<=b[other]-radius || p[other]>=b[other+3]+radius
                    || candidate<=b[axis]-radius || candidate>=b[axis+3]+radius)continue;
                candidate=direction&1?b[axis+3]+radius+.5f:b[axis]-radius-.5f;hits++;changed=1;
            }
            if(!changed)break;
        }
        float distance=fabsf(candidate-p[axis]);
        if(distance<best){best=distance;endpoint=candidate;best_axis=axis;contacts=hits;}
    }
    p[best_axis]=endpoint;
    float c=cosf(command->origin[3]),s=sinf(command->origin[3]);world[0]=command->origin[0]+c*p[0]-s*p[2];world[2]=command->origin[2]+s*p[0]+c*p[2];return contacts;
}
static inline int block_depenetrate_horizontal(const CubeCommand *command,float *world,float radius,float height){return block_depenetrate_horizontal_skin(command,world,radius,height,.1f);}
static inline int block_resolve_horizontal_validated(const CubeCommand *command,const float *before,const float *after,float *result){return block_resolve_horizontal_actor_validated(command,before,after,result,250,1600);}
static inline int block_resolve_horizontal(const CubeCommand *command,const float *before,const float *after,float *result) {
    return block_collision_scene_valid(command)?block_resolve_horizontal_validated(command,before,after,result):-1;
}
#endif
