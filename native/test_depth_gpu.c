#define CINTERFACE
#include <windows.h>
#include <d3d9.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "block_depth.h"
#include "depth_gpu.h"
static unsigned refs,fail,shader_reads,constant_reads;
static uint32_t program[]={0xfffe0300,0x0200001f,0x80000000,0x900f0000,0x0200001f,0x80000000,0xe00f0000,
 0x03000005,0x800f0001,0xa0e40001,0x90550000,
 0x04000004,0x800f0001,0xa0e40000,0x90000000,0x80e40001,
 0x04000004,0x800f0001,0xa0e40002,0x90aa0000,0x80e40001,
 0x04000004,0xe00f0000,0xa0e40003,0x90ff0000,0x80e40001,0x0000ffff};
static ULONG WINAPI release_shader(IDirect3DVertexShader9 *s){(void)s;assert(refs);return --refs;}
static HRESULT WINAPI function(IDirect3DVertexShader9 *s,void *p,UINT *bytes){
 (void)s;shader_reads++;
 if(fail==2){*bytes=20000;return S_OK;}
 if(fail==3 && p)return E_FAIL;
 if(!p){*bytes=sizeof(program);return S_OK;}
 assert(*bytes==sizeof(program));memcpy(p,program,sizeof(program));return S_OK;
}
static IDirect3DVertexShader9Vtbl shader_methods={.Release=release_shader,.GetFunction=function};
static IDirect3DVertexShader9 shader={&shader_methods};
static HRESULT WINAPI get_shader(IDirect3DDevice9 *d,IDirect3DVertexShader9 **out){(void)d;if(fail==1)return E_FAIL;*out=&shader;refs++;return S_OK;}
static HRESULT WINAPI constants(IDirect3DDevice9 *d,UINT start,float *out,UINT count){
 (void)d;constant_reads++;assert(!start && count==4);if(fail==4)return E_FAIL;
 float wvp[16]={1,0,.6003f,.6f,0,2,-.8004f,-.8f,3,0,0,0,2000,400,2926.4625f,3000};
 if(fail==5)wvp[2]=1;
 memcpy(out,wvp,64);return S_OK;
}
static IDirect3DDevice9Vtbl methods={.GetVertexShader=get_shader,.GetVertexShaderConstantF=constants};
static IDirect3DDevice9 inner={&methods};
int main(void){
 depth_gpu_initialize(GetCurrentProcessId());assert(depth_gpu_shared && depth_gpu_mapping);
 float gx[16]={1.60838f,0,0,0,0,2.144507f,0,0,0,0,-.00010001f,-100.010002f,0,0,-1,0};
 D3DVIEWPORT9 vp={0,0,1366,768,0,1};BlockProjection p;
 for(fail=0;fail<6;fail++){
  assert(depth_wvp_gpu(&inner,gx,50,&vp,4,&p)==!fail);assert(!refs);
  assert(depth_gpu_shared->reason==(fail==0?0:fail<=3?1:fail==4?2:3));
  assert(depth_gpu_shared->samples==fail+1 && depth_gpu_shared->recovered==1);
 }
 assert(shader_reads==9 && constant_reads==3);
 UnmapViewOfFile(depth_gpu_shared);CloseHandle(depth_gpu_mapping);
 return 0;
}
