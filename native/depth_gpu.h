#include "block_shader.h"
/* Optional, native-owned read-only telemetry. No commands and no GPU readback. */
typedef struct {uint32_t magic,version,build,pid;volatile LONG sequence;uint32_t stamp,reason,compare,shader_words;float wvp[16];float a,b,near_plane,far_plane;uint32_t samples,recovered,reserved;} DepthGpuShared;
_Static_assert(sizeof(DepthGpuShared)==128,"GPU projection telemetry");
static DepthGpuShared *depth_gpu_shared;static HANDLE depth_gpu_mapping;
static void depth_gpu_initialize(DWORD pid){
 wchar_t name[80];swprintf(name,80,L"Local\\RE4Bridge-GpuDepth-%lu",(unsigned long)pid);
 HANDLE map=CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(DepthGpuShared),name);if(!map)return;
 if(GetLastError()==ERROR_ALREADY_EXISTS){CloseHandle(map);return;}
 DepthGpuShared *p=MapViewOfFile(map,FILE_MAP_ALL_ACCESS,0,0,sizeof(*p));if(!p){CloseHandle(map);return;}
 memset(p,0,sizeof(*p));p->magic=0x52453447;p->version=1;p->build=457;p->pid=pid;depth_gpu_shared=p;depth_gpu_mapping=map;
}
static int depth_wvp_gpu(IDirect3DDevice9 *d,const float *gx,float fov,const D3DVIEWPORT9 *vp,unsigned compare,BlockProjection *out){
 IDirect3DVertexShader9 *shader=NULL;uint32_t code[4096];UINT bytes=0;float wvp[16]={0};unsigned reason=1;
 HRESULT hr=d->lpVtbl->GetVertexShader(d,&shader);
 if(SUCCEEDED(hr) && shader){
  hr=shader->lpVtbl->GetFunction(shader,NULL,&bytes);
  if(SUCCEEDED(hr) && bytes>=8 && bytes<=sizeof(code) && !(bytes&3)){
   UINT expected=bytes;hr=shader->lpVtbl->GetFunction(shader,code,&bytes);
   if(SUCCEEDED(hr) && bytes==expected && block_shader_wvp(code,bytes/4)){
    reason=2;
    if(SUCCEEDED(d->lpVtbl->GetVertexShaderConstantF(d,0,wvp,4))){
     reason=3;
     if(block_wvp_projection(wvp,gx,fov,(float)vp->Width,(float)vp->Height,compare,out))reason=0;
    }
   }
  }
 }
 if(shader)shader->lpVtbl->Release(shader);
 if(depth_gpu_shared){
  DepthGpuShared *p=depth_gpu_shared;InterlockedIncrement(&p->sequence);MemoryBarrier();
  p->stamp=GetTickCount();p->reason=reason;p->compare=compare;p->shader_words=bytes/4;p->samples++;p->recovered+=reason==0;
  for(unsigned i=0;i<16;i++)p->wvp[i]=isfinite(wvp[i])?wvp[i]:0;
  p->a=reason?0:out->a;p->b=reason?0:out->b;p->near_plane=reason?0:out->near_plane;p->far_plane=reason?0:out->far_plane;
  MemoryBarrier();InterlockedIncrement(&p->sequence);
 }
 return reason==0;
}
