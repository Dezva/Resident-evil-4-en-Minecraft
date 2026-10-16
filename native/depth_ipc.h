#include "block_depth.h"
#define RE4_DEPTH_RUNTIME 1
typedef struct {uint32_t magic,version;volatile LONG sequence;uint32_t build,pid,reason,attempts,draws,stage,width,height;float near_plane,far_plane;uint32_t reverse,source,reserved;float projection[16],gx_projection[16],fov;uint32_t compare,world_draws,last_hresult;volatile LONG command_sequence;uint32_t stamp,enabled,generation;uint32_t tail[8];} DepthShared;
_Static_assert(sizeof(DepthShared)==256,"Depth diagnostic/command ABI");
_Static_assert(offsetof(DepthShared,command_sequence)==208,"Separate host-owned depth command");
static DepthShared *depth_shared;static HANDLE depth_mapping;
static void depth_initialize(DWORD pid){
 wchar_t name[80];swprintf(name,80,L"Local\\RE4Bridge-Depth-%lu",(unsigned long)pid);
 HANDLE map=CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(DepthShared),name);if(!map)return;
 if(GetLastError()==ERROR_ALREADY_EXISTS){CloseHandle(map);return;}
 DepthShared *p=MapViewOfFile(map,FILE_MAP_ALL_ACCESS,0,0,sizeof(*p));if(!p){CloseHandle(map);return;}
 memset(p,0,sizeof(*p));p->magic=0x52453444;p->version=1;p->build=456;p->pid=pid;p->reason=1;depth_shared=p;depth_mapping=map;
}
static int depth_requested(uint32_t *generation){
 if(!depth_shared)return 0;
 uint32_t before=(uint32_t)depth_shared->command_sequence;MemoryBarrier();
 uint32_t stamp=depth_shared->stamp,enabled=depth_shared->enabled,gen=depth_shared->generation;MemoryBarrier();
 if((before&1) || before!=(uint32_t)depth_shared->command_sequence || enabled!=1 || !gen || GetTickCount()-stamp>=500)return 0;
 if(generation){*generation=gen;}return 1;
}
static void depth_report(unsigned reason,unsigned stage,const BlockProjection *projection,const D3DVIEWPORT9 *viewport,int drawn){
 if(!depth_shared)return;
 InterlockedIncrement(&depth_shared->sequence);MemoryBarrier();depth_shared->reason=reason;depth_shared->stage=stage;depth_shared->attempts++;depth_shared->draws+=!!drawn;
 if(viewport){depth_shared->width=viewport->Width;depth_shared->height=viewport->Height;}
 if(projection){depth_shared->near_plane=projection->near_plane;depth_shared->far_plane=projection->far_plane;depth_shared->reverse=projection->reverse;memcpy(depth_shared->projection,projection->matrix,64);}
 MemoryBarrier();InterlockedIncrement(&depth_shared->sequence);
}

static void depth_observe(unsigned source,const float *gx,float fov,unsigned compare,unsigned seen){
 if(!depth_shared)return;
 InterlockedIncrement(&depth_shared->sequence);MemoryBarrier();
 depth_shared->source=source;memcpy(depth_shared->gx_projection,gx,64);depth_shared->fov=fov;depth_shared->compare=compare;depth_shared->world_draws=seen;
 MemoryBarrier();InterlockedIncrement(&depth_shared->sequence);
}

/* Separate count: a compatibility draw is never reported as native occlusion. */
static void depth_fallback_draw(void){
 if(!depth_shared)return;
 InterlockedIncrement(&depth_shared->sequence);MemoryBarrier();depth_shared->reserved++;
 MemoryBarrier();InterlockedIncrement(&depth_shared->sequence);
}
