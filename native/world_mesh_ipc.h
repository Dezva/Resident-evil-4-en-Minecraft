/* Host-owned bounded geometry. No process pointers or executable commands. */
#define RE4_WORLD_MESH_RUNTIME 1
#define WORLD_MAX_QUADS 128u
#define WORLD_TEXTURE_SIZE 128u
typedef struct {float xyz[3],uv[2];uint32_t color;} WorldVertex;
typedef struct {uint32_t magic,version,build,pid;volatile LONG sequence;uint32_t stamp,enabled,generation,count,reserved[7];WorldVertex vertices[WORLD_MAX_QUADS*4];uint32_t pixels[128*128];} WorldMeshCommand;
_Static_assert(sizeof(WorldVertex)==24,"World vertex ABI");
_Static_assert(offsetof(WorldMeshCommand,vertices)==64,"World geometry offset");
_Static_assert(sizeof(WorldMeshCommand)==77888,"World mesh ABI");
static WorldMeshCommand *world_mesh_shared;static HANDLE world_mesh_mapping;
static void world_mesh_initialize(DWORD pid){
 wchar_t name[80];swprintf(name,80,L"Local\\RE4Bridge-WorldMesh-%lu",(unsigned long)pid);
 HANDLE map=CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(WorldMeshCommand),name);if(!map)return;
 if(GetLastError()==ERROR_ALREADY_EXISTS){CloseHandle(map);return;}
 WorldMeshCommand *p=MapViewOfFile(map,FILE_MAP_ALL_ACCESS,0,0,sizeof(*p));if(!p){CloseHandle(map);return;}
 memset(p,0,sizeof(*p));p->magic=0x52453457;p->version=1;p->build=457;p->pid=pid;world_mesh_shared=p;world_mesh_mapping=map;
}
static int world_mesh_requested(void){return world_mesh_shared && world_mesh_shared->enabled==1 && GetTickCount()-world_mesh_shared->stamp<500;}
static int world_mesh_read(WorldMeshCommand *out){
 if(!world_mesh_shared)return 0;
 for(unsigned attempt=0;attempt<3;attempt++){
  uint32_t before=(uint32_t)world_mesh_shared->sequence;MemoryBarrier();
  if(before&1)continue;
  memcpy(out,world_mesh_shared,sizeof(*out));MemoryBarrier();
  if(before!=(uint32_t)world_mesh_shared->sequence)continue;
  if(out->magic!=0x52453457 || out->version!=1 || out->build!=457 || out->pid!=GetCurrentProcessId() || out->enabled!=1 || !out->generation || !out->count || out->count>WORLD_MAX_QUADS || GetTickCount()-out->stamp>=500)return 0;
  for(unsigned i=0;i<7;i++)if(out->reserved[i])return 0;
  for(unsigned i=0;i<out->count*4;i++){
   WorldVertex *v=&out->vertices[i];for(unsigned c=0;c<3;c++)if(!isfinite(v->xyz[c]) || fabsf(v->xyz[c])>20000000.f)return 0;
   for(unsigned c=0;c<2;c++)if(!isfinite(v->uv[c]) || v->uv[c]<0 || v->uv[c]>1)return 0;
  }
  return 1;
 }
 return 0;
}
