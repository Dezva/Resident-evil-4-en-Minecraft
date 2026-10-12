#include "test_cube_platform.h"
#include "cube_policy.h"
#include "block_surface.h"
#include "block_texture.h"
typedef struct {IDirect3DDevice9 *inner;CubeGate gate;unsigned scene,escaped,resetting;uint32_t player;BlockTextureCache block_texture;} CubeDevice;
typedef struct {DWORD render[256],stage[2][32],sampler[16],fvf,vertex,pixel,texture,stream,decl;} TestState;
static TestState current,saved,original;
static unsigned events[32],surface_refs,backup_refs,creates,captures,applies,releases,setters,draws;
static unsigned create_fail,capture_fail,apply_fail,setter_fail,draw_fail,offscreen,camera_invalid,viewport_fail,back_fail;
static unsigned allocations,frees,heap_fail;
static void *GetProcessHeap(void) {return (void*)1;}
static void *HeapAlloc(void *heap,unsigned flags,size_t size) {assert(heap==(void*)1 && !flags && size==sizeof(BlockSurfaceMesh));if(heap_fail)return NULL;allocations++;return malloc(size);}
static int HeapFree(void *heap,unsigned flags,void *p) {assert(heap==(void*)1 && !flags && p && frees<allocations);frees++;free(p);return 1;}
static unsigned texture_creates,texture_refs,texture_releases,texture_locks,texture_unlocks;
static unsigned texture_create_fail,texture_null,texture_lock_fail,texture_unlock_fail,texture_bad_pitch,texture_null_bits;
static uint32_t tile[256*260];
static unsigned f8;static uint32_t player_id=42;static uint32_t now=1000;static float shift;
static unsigned last_vertices;
static float first_x;
static CubeCommand command={.enabled=1,.epoch=7,.stamp=1000};
static unsigned scene_reads;
static int cube_command_read(CubeCommand *out) {scene_reads++;*out=command;return 1;}
static int cube_command_requested(void){return command.enabled==1 && command.epoch!=0;}
static DWORD GetTickCount(void) {return now;}
static int GetAsyncKeyState(int key) {assert(key==VK_F8);return f8?0x8000:0;}
static void cube_record(unsigned event,uint32_t value) {(void)value;events[event]++;}
static int cube_camera(float *view,float *fov,uint32_t *player) {
    float matrix[12]={1,0,0,0,0,1,0,0,0,0,1,0};memcpy(view,matrix,sizeof(matrix));view[3]=shift;*fov=90;*player=player_id;return !camera_invalid;
}
static ULONG release_surface(IDirect3DSurface9 *s) {(void)s;assert(surface_refs);return --surface_refs;}
static TestSurfaceVtbl surface_table={release_surface};
static IDirect3DSurface9 surface={&surface_table},other={&surface_table};
static ULONG release_backup(IDirect3DStateBlock9 *b) {(void)b;assert(backup_refs);releases++;return --backup_refs;}
static HRESULT capture_backup(IDirect3DStateBlock9 *b) {(void)b;captures++;if(capture_fail)return E_FAIL;saved=current;return S_OK;}
static HRESULT apply_backup(IDirect3DStateBlock9 *b) {(void)b;applies++;if(apply_fail)return E_FAIL;current=saved;return S_OK;}
static TestStateVtbl state_table={release_backup,capture_backup,apply_backup};
static IDirect3DStateBlock9 backup={&state_table};
static HRESULT get_target(IDirect3DDevice9 *d,DWORD index,IDirect3DSurface9 **out) {(void)d;assert(!index);surface_refs++;*out=&surface;return S_OK;}
static HRESULT get_back(IDirect3DDevice9 *d,UINT chain,UINT buffer,D3DBACKBUFFER_TYPE type,IDirect3DSurface9 **out) {(void)d;assert(!chain && !buffer && type==D3DBACKBUFFER_TYPE_MONO);if(back_fail)return E_FAIL;surface_refs++;*out=offscreen?&other:&surface;return S_OK;}
static HRESULT get_view(IDirect3DDevice9 *d,D3DVIEWPORT9 *out) {(void)d;if(viewport_fail)return E_FAIL;*out=(D3DVIEWPORT9){10,20,640,480,0,1};return S_OK;}
static HRESULT create_backup(IDirect3DDevice9 *d,D3DSTATEBLOCKTYPE type,IDirect3DStateBlock9 **out) {(void)d;assert(type==D3DSBT_ALL);creates++;if(create_fail)return E_FAIL;backup_refs++;*out=&backup;return S_OK;}
static HRESULT changed(void) {setters++;return setter_fail==setters?E_FAIL:S_OK;}
static HRESULT vertex_shader(IDirect3DDevice9 *d,IDirect3DVertexShader9 *shader) {(void)d;assert(!shader);current.vertex=0;return changed();}
static HRESULT pixel_shader(IDirect3DDevice9 *d,IDirect3DPixelShader9 *shader) {(void)d;assert(!shader);current.pixel=0;return changed();}
static HRESULT fvf(IDirect3DDevice9 *d,DWORD flags) {(void)d;assert(flags==(D3DFVF_XYZRHW|D3DFVF_DIFFUSE) || flags==(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1));current.fvf=flags;current.decl=0;return changed();}
static HRESULT texture(IDirect3DDevice9 *d,DWORD stage,IDirect3DBaseTexture9 *t) {(void)d;assert(!stage);current.texture=t?1:0;return changed();}
static HRESULT stage(IDirect3DDevice9 *d,DWORD n,D3DTEXTURESTAGESTATETYPE type,DWORD value) {(void)d;assert(n<2 && type<32);current.stage[n][type]=value;return changed();}
static HRESULT render(IDirect3DDevice9 *d,D3DRENDERSTATETYPE type,DWORD value) {(void)d;assert(type<256);current.render[type]=value;return changed();}
static HRESULT sampler_state(IDirect3DDevice9 *d,DWORD n,D3DSAMPLERSTATETYPE type,DWORD value) {(void)d;assert(!n && type<16);current.sampler[type]=value;return changed();}
static ULONG texture_release(IDirect3DTexture9 *t) {(void)t;assert(texture_refs);texture_releases++;return --texture_refs;}
static HRESULT texture_lock(IDirect3DTexture9 *t,UINT level,D3DLOCKED_RECT *out,const void *rect,DWORD flags) {
    (void)t;assert(!level && !rect && flags==D3DLOCK_DISCARD);texture_locks++;
    if(texture_lock_fail)return E_FAIL;
    out->Pitch=texture_bad_pitch?60:command.style==3?1040:80;out->pBits=texture_null_bits?NULL:tile;return S_OK;
}
static HRESULT texture_unlock(IDirect3DTexture9 *t,UINT level) {(void)t;assert(!level);texture_unlocks++;return texture_unlock_fail?E_FAIL:S_OK;}
static TestTextureVtbl texture_table={texture_release,texture_lock,texture_unlock};
static IDirect3DTexture9 stone_texture={&texture_table};
static HRESULT texture_create(IDirect3DDevice9 *d,UINT width,UINT height,UINT levels,DWORD usage,D3DFORMAT format,D3DPOOL pool,IDirect3DTexture9 **out,HANDLE *shared) {
    (void)d;assert(width==(command.style==3?256:16) && height==width && levels==1 && usage==D3DUSAGE_DYNAMIC && format==D3DFMT_A8R8G8B8 && pool==D3DPOOL_DEFAULT && !shared);
    texture_creates++;if(texture_create_fail)return E_FAIL;if(texture_null){*out=NULL;return S_OK;}
    texture_refs++;*out=&stone_texture;memset(tile,0xBA,sizeof(tile));return S_OK;
}
static HRESULT draw(IDirect3DDevice9 *d,D3DPRIMITIVETYPE type,UINT count,const void *data,UINT stride) {
    (void)d;assert((type==D3DPT_LINELIST || type==D3DPT_TRIANGLELIST) && count>0);
    assert(stride==(type==D3DPT_LINELIST?20:28));assert(count<=(type==D3DPT_LINELIST?12*CUBE_MAX_BOXES:BLOCK_SURFACE_MAX_VERTICES/3));
    last_vertices=count*(type==D3DPT_LINELIST?2:3);
    for(unsigned i=0;i<last_vertices;++i) {
        BlockVertex v;memcpy(&v,(const unsigned char*)data+stride*i,sizeof(v));if(!i)first_x=v.x;
        assert(isfinite(v.x+v.y) && v.x>=10 && v.x<=650 && v.y>=20 && v.y<=500);
        if(type==D3DPT_LINELIST)assert(v.color==0xFF40FF40);
        else {assert((v.color>>24)==255 && (v.color&255)==((v.color>>8)&255));const BlockSurfaceVertex *textured=data;assert(textured[i].rhw>0 && textured[i].rhw<=1);}
    }
    if(command.style>=2) {
        assert(texture_refs==1 && current.texture==1 && current.fvf==(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1));
        assert(current.stage[0][D3DTSS_COLOROP]==D3DTOP_MODULATE && current.stage[0][D3DTSS_COLORARG1]==D3DTA_TEXTURE);
        assert(current.sampler[D3DSAMP_MINFILTER]==D3DTEXF_POINT && current.sampler[D3DSAMP_MIPFILTER]==D3DTEXF_NONE);
        if(command.style==2)for(unsigned y=0;y<16;++y)for(unsigned x=16;x<20;++x)assert(tile[y*20+x]==0xBABABABA);
        else {for(unsigned y=0;y<256;++y)for(unsigned x=256;x<260;++x)assert(tile[y*260+x]==0xBABABABA);assert(tile[8*260+8]==command.pixels[0][0] && tile[8*260+40]==command.pixels[command.tile_count>1?1:0][0]);assert(current.render[D3DRS_ALPHABLENDENABLE]==1 && current.stage[0][D3DTSS_ALPHAOP]==D3DTOP_MODULATE);} // Respect row pitch.
    }
    draws++;current.stream=0;return draw_fail?E_FAIL:S_OK;
}
static IDirect3DDevice9Vtbl table={.GetRenderTarget=get_target,.GetBackBuffer=get_back,.GetViewport=get_view,
    .CreateStateBlock=create_backup,.SetVertexShader=vertex_shader,.SetPixelShader=pixel_shader,.SetFVF=fvf,
    .CreateTexture=texture_create,.SetSamplerState=sampler_state,.SetTexture=texture,.SetTextureStageState=stage,.SetRenderState=render,.DrawPrimitiveUP=draw};
static IDirect3DDevice9 inner={&table};
#include "cube_draw.h"
/* Each ordinary failure fixture also simulates device resource teardown. */
static void cube_render_and_teardown(CubeDevice *f){cube_render_device(f);block_texture_cache_release(&f->block_texture);}
static CubeDevice reset(void) {
    memset(events,0,sizeof(events));memset(&current,0x37,sizeof(current));original=current;
    assert(allocations==frees && !texture_refs);texture_creates=texture_releases=texture_locks=texture_unlocks=0;
    texture_create_fail=texture_null=texture_lock_fail=texture_unlock_fail=texture_bad_pitch=texture_null_bits=0;
    assert(allocations==frees);allocations=frees=heap_fail=0;
    surface_refs=backup_refs=creates=captures=applies=releases=setters=draws=0;
    create_fail=capture_fail=apply_fail=setter_fail=draw_fail=offscreen=camera_invalid=viewport_fail=back_fail=f8=0;
    shift=0;player_id=42;now=1000;command=(CubeCommand){.enabled=1,.epoch=7,.stamp=1000};
    return (CubeDevice){.inner=&inner,.scene=1};
}
int main(void) {
    CubeDevice dormant=reset();command.enabled=0;scene_reads=0;
    for(unsigned i=0;i<10000;i++)cube_render_device(&dormant);
    assert(!scene_reads && !creates && !setters && !texture_creates && !draws && !allocations);
    CubeDevice f=reset();cube_render_and_teardown(&f);
    assert(events[CUBE_DRAW]==1 && draws==1 && creates==1 && captures==1 && applies==1 && releases==1 && !backup_refs && !surface_refs);
    assert(!memcmp(&current,&original,sizeof(current)) && f.gate.anchored && f.gate.bounds[2]==-650);
    float fixed[6];memcpy(fixed,f.gate.bounds,sizeof(fixed));shift=10;cube_render_and_teardown(&f);assert(!memcmp(fixed,f.gate.bounds,sizeof(fixed)));
    command.epoch=8;cube_render_and_teardown(&f);assert(f.gate.bounds[0]==-60);
    /* Minecraft supplies explicit world bounds. Moving the camera or changing
       the player pointer must not relocate them; a new placement replaces them. */
    f=reset();command.mode=1;
    float supplied[6]={-50,-50,-650,50,50,-550};memcpy(command.bounds,supplied,sizeof(supplied));
    cube_render_and_teardown(&f);assert(draws==1 && !memcmp(f.gate.bounds,supplied,sizeof(supplied)));
    shift=10;player_id=99;cube_render_and_teardown(&f);assert(draws==2 && !memcmp(f.gate.bounds,supplied,sizeof(supplied)));
    command.enabled=0;cube_render_and_teardown(&f);assert(draws==2);
    command.enabled=1;command.epoch++;command.bounds[0]+=10;command.bounds[3]+=10;
    cube_render_and_teardown(&f);assert(draws==3 && f.gate.bounds[0]==-40);
    unsigned prior_explicit=draws;command.bounds[3]=command.bounds[0];
    cube_render_and_teardown(&f);assert(draws==prior_explicit && events[CUBE_CAMERA_REJECT]==1);
    command.bounds[3]=NAN;cube_render_and_teardown(&f);assert(draws==prior_explicit && events[CUBE_CAMERA_REJECT]==2);
    /* One stable rigid grid, two boxes and a half-height slab, one state backup. */
    f=reset();command.mode=2;command.count=3;command.origin[2]=-600;
    float cube[6]={-50,-50,-50,50,50,50};
    for (unsigned i=0;i<3;++i) {memcpy(command.boxes[i],cube,sizeof(cube));command.boxes[i][0]+=i*100;command.boxes[i][3]+=i*100;}
    command.boxes[2][4]=0;cube_render_and_teardown(&f);
    assert(draws==1 && last_vertices==72 && creates==1 && applies==1 && !memcmp(&current,&original,sizeof(current)));
    float native_origin[4];memcpy(native_origin,command.origin,sizeof(native_origin));float first=first_x;
    shift=10;player_id=99;cube_render_and_teardown(&f);
    assert(draws==2 && first_x>first && !memcmp(native_origin,command.origin,sizeof(native_origin)));
    command.count=2;cube_render_and_teardown(&f);assert(last_vertices==48); // Individual removal.
    command.origin[3]=0.5f;cube_render_and_teardown(&f);assert(draws==4 && last_vertices==48);
    command.boxes[1][0]=NAN;cube_render_and_teardown(&f);assert(draws==4 && events[CUBE_CAMERA_REJECT]==1);
    command.boxes[1][0]=0;command.origin[3]=NAN;cube_render_and_teardown(&f);assert(draws==4);
    f=reset();command.mode=2;command.count=CUBE_MAX_BOXES;command.origin[2]=-600;
    for (unsigned i=0;i<CUBE_MAX_BOXES;++i) memcpy(command.boxes[i],cube,sizeof(cube));
    cube_render_and_teardown(&f);assert(last_vertices==24*CUBE_MAX_BOXES && draws==1 && creates==1 && applies==1 && !surface_refs && !backup_refs);
    command.count=CUBE_MAX_BOXES+1;cube_render_and_teardown(&f);assert(draws==1);
    for(unsigned failure=1;failure<=30;++failure) {
        f=reset();setter_fail=failure;cube_render_and_teardown(&f);assert(events[CUBE_FAILURE]==1 && !draws && applies==1 && releases==1 && !backup_refs && !surface_refs && !memcmp(&current,&original,sizeof(current)));
    }
    f=reset();draw_fail=1;cube_render_and_teardown(&f);assert(draws==1 && events[CUBE_FAILURE]==1 && applies==1 && !memcmp(&current,&original,sizeof(current)));
    cube_render_and_teardown(&f);assert(draws==1 && events[CUBE_REJECT]==1);
    f=reset();create_fail=1;cube_render_and_teardown(&f);assert(events[CUBE_FAILURE]==1 && !setters && !captures && !releases && !backup_refs);
    f=reset();capture_fail=1;cube_render_and_teardown(&f);assert(events[CUBE_FAILURE]==1 && !setters && !applies && releases==1 && !backup_refs && !memcmp(&current,&original,sizeof(current)));
    f=reset();apply_fail=1;cube_render_and_teardown(&f);assert(f.gate.fatal && events[CUBE_RESTORE_FAILURE]==1 && !events[CUBE_DRAW] && !backup_refs);
    unsigned prior=draws;command.epoch=8;cube_render_and_teardown(&f);assert(draws==prior && events[CUBE_REJECT]==1);
    f=reset();f8=1;cube_render_and_teardown(&f);f8=0;cube_render_and_teardown(&f);assert(!draws);command.epoch=8;cube_render_and_teardown(&f);assert(draws==1);
    f=reset();now=1500;cube_render_and_teardown(&f);assert(!draws && !creates && events[CUBE_REJECT]==1);
    f=reset();command.enabled=0;cube_render_and_teardown(&f);assert(!creates);
    f=reset();f.scene=0;cube_render_and_teardown(&f);assert(!creates);
    f=reset();f.resetting=1;cube_render_and_teardown(&f);assert(!creates);
    f=reset();f.escaped=1;cube_render_and_teardown(&f);assert(!creates);
    f=reset();offscreen=1;cube_render_and_teardown(&f);assert(!draws && events[CUBE_PASS_SKIP]==1 && !surface_refs);
    f=reset();back_fail=1;cube_render_and_teardown(&f);assert(!creates && !surface_refs);
    f=reset();camera_invalid=1;cube_render_and_teardown(&f);assert(!creates && events[CUBE_CAMERA_REJECT]==1 && !surface_refs);
    f=reset();viewport_fail=1;cube_render_and_teardown(&f);assert(!creates && events[CUBE_FAILURE]==1);
    /* Filled path uses the same scene/state policy and releases scratch on every exit. */
    f=reset();command.mode=2;command.style=1;command.count=1;command.origin[2]=-600;
    memcpy(command.boxes[0],cube,sizeof(cube));cube_render_and_teardown(&f);
    assert(draws==1 && last_vertices==6 && allocations==1 && frees==1 && setters==30 && !memcmp(&current,&original,sizeof(current)));
    command.style=0;cube_render_and_teardown(&f);assert(draws==2 && last_vertices==24 && allocations==1 && frees==1);
    for(unsigned failure=1;failure<=30;++failure) {
        f=reset();command.mode=2;command.style=1;command.count=1;command.origin[2]=-600;memcpy(command.boxes[0],cube,sizeof(cube));
        setter_fail=failure;cube_render_and_teardown(&f);
        assert(!draws && applies==1 && allocations==1 && frees==1 && !memcmp(&current,&original,sizeof(current)));
    }
    for(unsigned failure=0;failure<6;++failure) {
        f=reset();command.mode=2;command.style=1;command.count=1;command.origin[2]=-600;memcpy(command.boxes[0],cube,sizeof(cube));
        if(failure==0)heap_fail=1;
        if(failure==1)create_fail=1;
        if(failure==2)capture_fail=1;
        if(failure==3)draw_fail=1;
        if(failure==4)apply_fail=1;
        if(failure==5)command.origin[2]=600; // Behind camera: no state modification.
        cube_render_and_teardown(&f);assert(allocations==frees && !surface_refs && !backup_refs);
        if(failure!=4)assert(!memcmp(&current,&original,sizeof(current)));
        if(failure==0 || failure==5)assert(!creates && !draws);
        if(failure==4)assert(f.gate.fatal);
    }
    f=reset();command.mode=2;command.style=1;command.count=1;command.origin[2]=-600;memcpy(command.boxes[0],cube,sizeof(cube));
    f8=1;cube_render_and_teardown(&f);assert(!allocations && !creates);f8=0;command.epoch++;cube_render_and_teardown(&f);assert(draws==1 && allocations==frees);
    now=1500;cube_render_and_teardown(&f);assert(draws==1 && allocations==frees);
    /* Texture create/lock/unlock, sampler failures and restoration use actual draw code. */
    f=reset();command.mode=2;command.style=2;command.count=1;command.origin[2]=-600;memcpy(command.boxes[0],cube,sizeof(cube));
    cube_render_and_teardown(&f);assert(draws==1 && setters==38 && texture_creates==1 && texture_releases==1 && texture_locks==1 && texture_unlocks==1 && !texture_refs && allocations==frees && !memcmp(&current,&original,sizeof(current)));
    command.style=1;cube_render_and_teardown(&f);assert(draws==2 && texture_creates==1);
    f=reset();command.mode=2;command.style=2;command.count=CUBE_MAX_BOXES;command.origin[2]=-600;
    for(unsigned i=0;i<CUBE_MAX_BOXES;++i)memcpy(command.boxes[i],cube,sizeof(cube));
    cube_render_and_teardown(&f);assert(draws==1 && last_vertices==6*CUBE_MAX_BOXES && texture_creates==1 && texture_releases==1 && allocations==frees);
    f=reset();command.mode=2;command.style=2;command.count=1;command.origin[2]=-600;memcpy(command.boxes[0],cube,sizeof(cube));
    f8=1;cube_render_and_teardown(&f);assert(!texture_creates && !allocations);f8=0;command.epoch++;cube_render_and_teardown(&f);assert(draws==1 && !texture_refs);
    now=1500;cube_render_and_teardown(&f);assert(draws==1 && texture_creates==1);
    /* Padded rows are checked at draw time; pixel pattern is deterministic and opaque. */
    unsigned dark=255,light=0;
    for(unsigned y=0;y<16;++y)for(unsigned x=0;x<16;++x){uint32_t pixel=block_stone_pixel(x,y);assert((pixel>>24)==255 && (pixel&255)==((pixel>>8)&255));unsigned value=pixel&255;if(value<dark)dark=value;if(value>light)light=value;}
    assert(light-dark>=40);
    for(unsigned failure=1;failure<=38;++failure) {
        f=reset();command.mode=2;command.style=2;command.count=1;command.origin[2]=-600;memcpy(command.boxes[0],cube,sizeof(cube));
        setter_fail=failure;cube_render_and_teardown(&f);assert(!draws && applies==1 && texture_creates==1 && !texture_refs && allocations==frees && !memcmp(&current,&original,sizeof(current)));
    }
    for(unsigned failure=0;failure<12;++failure) {
        f=reset();command.mode=2;command.style=2;command.count=1;command.origin[2]=-600;memcpy(command.boxes[0],cube,sizeof(cube));
        if(failure==0)texture_create_fail=1;
        if(failure==1)texture_null=1;
        if(failure==2)texture_lock_fail=1;
        if(failure==3)texture_unlock_fail=1;
        if(failure==4)texture_bad_pitch=1;
        if(failure==5)texture_null_bits=1;
        if(failure==6)capture_fail=1;
        if(failure==7)create_fail=1;
        if(failure==8)heap_fail=1;
        if(failure==9)apply_fail=1;
        if(failure==10)draw_fail=1;
        if(failure==11)command.enabled=0;
        cube_render_and_teardown(&f);assert(!texture_refs && allocations==frees && !surface_refs && !backup_refs);
        if(failure!=9)assert(!memcmp(&current,&original,sizeof(current)));
        if(failure<6)assert(!draws && !setters && applies==1 && events[CUBE_FAILURE]==1);
        if(failure==9)assert(f.gate.fatal);
    }
    /* Real atlas path: two distinct colors, border padding, alpha and every setter failure. */
    f=reset();command.mode=2;command.style=3;command.count=1;command.tile_count=2;command.origin[2]=-600;memcpy(command.boxes[0],cube,sizeof(cube));
    for(unsigned i=0;i<256;++i){command.pixels[0][i]=0xFF112233;command.pixels[1][i]=0x80445566;}
    for(unsigned i=0;i<6;++i){command.materials[0][i].tile=1;float uv[8]={0,0,1,0,1,1,0,1};memcpy(command.materials[0][i].uv,uv,sizeof(uv));}
    cube_render_and_teardown(&f);assert(draws==1 && setters==43 && allocations==frees && !texture_refs && !memcmp(&current,&original,sizeof(current)));
    for(unsigned failure=1;failure<=43;++failure){
        f=reset();command.mode=2;command.style=3;command.count=1;command.tile_count=2;command.origin[2]=-600;memcpy(command.boxes[0],cube,sizeof(cube));
        setter_fail=failure;cube_render_and_teardown(&f);assert(!draws && applies==1 && !texture_refs && allocations==frees && !memcmp(&current,&original,sizeof(current)));
    }
    f=reset();command.mode=2;command.style=3;command.count=1;command.tile_count=1;command.origin[2]=-600;memcpy(command.boxes[0],cube,sizeof(cube));
    command.materials[0][0].tile=1;cube_render_and_teardown(&f);assert(!draws && !texture_creates && allocations==frees);
    command.materials[0][0].tile=0;command.materials[0][0].uv[0]=NAN;cube_render_and_teardown(&f);assert(!draws && !texture_creates && allocations==frees);
    /* Stable scenes keep their GPU atlas even when camera/epoch changes. */
    f=reset();command.mode=2;command.style=3;command.count=1;command.tile_count=2;command.origin[2]=-600;memcpy(command.boxes[0],cube,sizeof(cube));
    for(unsigned i=0;i<256;i++){command.pixels[0][i]=0xFF112233;command.pixels[1][i]=0x80445566;}
    for(unsigned frame=0;frame<200;frame++){command.epoch++;cube_render_device(&f);}
    assert(draws==200 && texture_creates==1 && texture_locks==1 && texture_refs==1 && allocations==frees && !backup_refs);
    command.pixels[0][0]=0xFF556677;cube_render_device(&f);
    assert(texture_creates==1 && texture_locks==2 && tile[8*260+8]==0xFF556677);
    command.tile_count=1;cube_render_device(&f);assert(texture_creates==1 && texture_locks==3);
    command.enabled=0;cube_render_device(&f);assert(!texture_refs && !f.block_texture.texture);
    command.enabled=1;command.epoch++;cube_render_device(&f);assert(texture_creates==2 && texture_refs==1);
    command.style=2;cube_render_device(&f);assert(texture_creates==3 && texture_locks==5 && texture_refs==1);
    command.style=3;texture_lock_fail=1;cube_render_device(&f);assert(!texture_refs && events[CUBE_FAILURE]==1 && !memcmp(&current,&original,sizeof(current)));
    block_texture_cache_release(&f.block_texture);assert(!texture_refs && !memcmp(&current,&original,sizeof(current)));
    puts("Cube rendering: 200 stable frames reuse atlas; changed pixels/count/style, teardown, failures, full restoration, F8/TTL and world anchor passed");
}
