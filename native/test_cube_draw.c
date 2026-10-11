#include "test_cube_platform.h"
#include "cube_policy.h"
#include "block_geometry.h"
typedef struct {IDirect3DDevice9 *inner;CubeGate gate;unsigned scene,escaped,resetting;uint32_t player;} CubeDevice;
typedef struct {DWORD render[256],stage[2][8],fvf,vertex,pixel,texture,stream,decl;} TestState;
static TestState current,saved,original;
static unsigned events[32],surface_refs,backup_refs,creates,captures,applies,releases,setters,draws;
static unsigned create_fail,capture_fail,apply_fail,setter_fail,draw_fail,offscreen,camera_invalid,viewport_fail,back_fail;
static unsigned f8;static uint32_t player_id=42;static uint32_t now=1000;static float shift;
static unsigned last_vertices;
static float first_x;
static CubeCommand command={.enabled=1,.epoch=7,.stamp=1000};
static int cube_command_read(CubeCommand *out) {*out=command;return 1;}
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
static HRESULT fvf(IDirect3DDevice9 *d,DWORD flags) {(void)d;assert(flags==(D3DFVF_XYZRHW|D3DFVF_DIFFUSE));current.fvf=flags;current.decl=0;return changed();}
static HRESULT texture(IDirect3DDevice9 *d,DWORD stage,IDirect3DBaseTexture9 *t) {(void)d;assert(!stage && !t);current.texture=0;return changed();}
static HRESULT stage(IDirect3DDevice9 *d,DWORD n,D3DTEXTURESTAGESTATETYPE type,DWORD value) {(void)d;assert(n<2 && type<8);current.stage[n][type]=value;return changed();}
static HRESULT render(IDirect3DDevice9 *d,D3DRENDERSTATETYPE type,DWORD value) {(void)d;assert(type<256);current.render[type]=value;return changed();}
static HRESULT draw(IDirect3DDevice9 *d,D3DPRIMITIVETYPE type,UINT count,const void *data,UINT stride) {
    (void)d;assert(type==D3DPT_LINELIST && count>0 && count<=12*CUBE_MAX_BOXES && stride==20);const BlockVertex *vertices=data;
    last_vertices=count*2;first_x=vertices[0].x;
    for(unsigned i=0;i<count*2;++i) assert(isfinite(vertices[i].x+vertices[i].y) && vertices[i].x>=10 && vertices[i].x<=650 && vertices[i].y>=20 && vertices[i].y<=500 && vertices[i].color==0xFF40FF40);
    draws++;current.stream=0;return draw_fail?E_FAIL:S_OK;
}
static IDirect3DDevice9Vtbl table={.GetRenderTarget=get_target,.GetBackBuffer=get_back,.GetViewport=get_view,
    .CreateStateBlock=create_backup,.SetVertexShader=vertex_shader,.SetPixelShader=pixel_shader,.SetFVF=fvf,
    .SetTexture=texture,.SetTextureStageState=stage,.SetRenderState=render,.DrawPrimitiveUP=draw};
static IDirect3DDevice9 inner={&table};
#include "cube_draw.h"
static CubeDevice reset(void) {
    memset(events,0,sizeof(events));memset(&current,0x37,sizeof(current));original=current;
    surface_refs=backup_refs=creates=captures=applies=releases=setters=draws=0;
    create_fail=capture_fail=apply_fail=setter_fail=draw_fail=offscreen=camera_invalid=viewport_fail=back_fail=f8=0;
    shift=0;player_id=42;now=1000;command=(CubeCommand){.enabled=1,.epoch=7,.stamp=1000};
    return (CubeDevice){.inner=&inner,.scene=1};
}
int main(void) {
    CubeDevice f=reset();cube_render_device(&f);
    assert(events[CUBE_DRAW]==1 && draws==1 && creates==1 && captures==1 && applies==1 && releases==1 && !backup_refs && !surface_refs);
    assert(!memcmp(&current,&original,sizeof(current)) && f.gate.anchored && f.gate.bounds[2]==-650);
    float fixed[6];memcpy(fixed,f.gate.bounds,sizeof(fixed));shift=10;cube_render_device(&f);assert(!memcmp(fixed,f.gate.bounds,sizeof(fixed)));
    command.epoch=8;cube_render_device(&f);assert(f.gate.bounds[0]==-60);
    /* Minecraft supplies explicit world bounds. Moving the camera or changing
       the player pointer must not relocate them; a new placement replaces them. */
    f=reset();command.mode=1;
    float supplied[6]={-50,-50,-650,50,50,-550};memcpy(command.bounds,supplied,sizeof(supplied));
    cube_render_device(&f);assert(draws==1 && !memcmp(f.gate.bounds,supplied,sizeof(supplied)));
    shift=10;player_id=99;cube_render_device(&f);assert(draws==2 && !memcmp(f.gate.bounds,supplied,sizeof(supplied)));
    command.enabled=0;cube_render_device(&f);assert(draws==2);
    command.enabled=1;command.epoch++;command.bounds[0]+=10;command.bounds[3]+=10;
    cube_render_device(&f);assert(draws==3 && f.gate.bounds[0]==-40);
    unsigned prior_explicit=draws;command.bounds[3]=command.bounds[0];
    cube_render_device(&f);assert(draws==prior_explicit && events[CUBE_CAMERA_REJECT]==1);
    command.bounds[3]=NAN;cube_render_device(&f);assert(draws==prior_explicit && events[CUBE_CAMERA_REJECT]==2);
    /* One stable rigid grid, two boxes and a half-height slab, one state backup. */
    f=reset();command.mode=2;command.count=3;command.origin[2]=-600;
    float cube[6]={-50,-50,-50,50,50,50};
    for (unsigned i=0;i<3;++i) {memcpy(command.boxes[i],cube,sizeof(cube));command.boxes[i][0]+=i*100;command.boxes[i][3]+=i*100;}
    command.boxes[2][4]=0;cube_render_device(&f);
    assert(draws==1 && last_vertices==72 && creates==1 && applies==1 && !memcmp(&current,&original,sizeof(current)));
    float native_origin[4];memcpy(native_origin,command.origin,sizeof(native_origin));float first=first_x;
    shift=10;player_id=99;cube_render_device(&f);
    assert(draws==2 && first_x>first && !memcmp(native_origin,command.origin,sizeof(native_origin)));
    command.count=2;cube_render_device(&f);assert(last_vertices==48); // Individual removal.
    command.origin[3]=0.5f;cube_render_device(&f);assert(draws==4 && last_vertices==48);
    command.boxes[1][0]=NAN;cube_render_device(&f);assert(draws==4 && events[CUBE_CAMERA_REJECT]==1);
    command.boxes[1][0]=0;command.origin[3]=NAN;cube_render_device(&f);assert(draws==4);
    f=reset();command.mode=2;command.count=CUBE_MAX_BOXES;command.origin[2]=-600;
    for (unsigned i=0;i<CUBE_MAX_BOXES;++i) memcpy(command.boxes[i],cube,sizeof(cube));
    cube_render_device(&f);assert(last_vertices==24*CUBE_MAX_BOXES && draws==1 && creates==1 && applies==1 && !surface_refs && !backup_refs);
    command.count=CUBE_MAX_BOXES+1;cube_render_device(&f);assert(draws==1);
    for(unsigned failure=1;failure<=23;++failure) {
        f=reset();setter_fail=failure;cube_render_device(&f);assert(events[CUBE_FAILURE]==1 && !draws && applies==1 && releases==1 && !backup_refs && !surface_refs && !memcmp(&current,&original,sizeof(current)));
    }
    f=reset();draw_fail=1;cube_render_device(&f);assert(draws==1 && events[CUBE_FAILURE]==1 && applies==1 && !memcmp(&current,&original,sizeof(current)));
    cube_render_device(&f);assert(draws==1 && events[CUBE_REJECT]==1);
    f=reset();create_fail=1;cube_render_device(&f);assert(events[CUBE_FAILURE]==1 && !setters && !captures && !releases && !backup_refs);
    f=reset();capture_fail=1;cube_render_device(&f);assert(events[CUBE_FAILURE]==1 && !setters && !applies && releases==1 && !backup_refs && !memcmp(&current,&original,sizeof(current)));
    f=reset();apply_fail=1;cube_render_device(&f);assert(f.gate.fatal && events[CUBE_RESTORE_FAILURE]==1 && !events[CUBE_DRAW] && !backup_refs);
    unsigned prior=draws;command.epoch=8;cube_render_device(&f);assert(draws==prior && events[CUBE_REJECT]==1);
    f=reset();f8=1;cube_render_device(&f);f8=0;cube_render_device(&f);assert(!draws);command.epoch=8;cube_render_device(&f);assert(draws==1);
    f=reset();now=1500;cube_render_device(&f);assert(!draws && !creates && events[CUBE_REJECT]==1);
    f=reset();command.enabled=0;cube_render_device(&f);assert(!creates);
    f=reset();f.scene=0;cube_render_device(&f);assert(!creates);
    f=reset();f.resetting=1;cube_render_device(&f);assert(!creates);
    f=reset();f.escaped=1;cube_render_device(&f);assert(!creates);
    f=reset();offscreen=1;cube_render_device(&f);assert(!draws && events[CUBE_PASS_SKIP]==1 && !surface_refs);
    f=reset();back_fail=1;cube_render_device(&f);assert(!creates && !surface_refs);
    f=reset();camera_invalid=1;cube_render_device(&f);assert(!creates && events[CUBE_CAMERA_REJECT]==1 && !surface_refs);
    f=reset();viewport_fail=1;cube_render_device(&f);assert(!creates && events[CUBE_FAILURE]==1);
    puts("Cube rendering: all setter/draw failures restore full state, temporary resources balanced, restore-failure latch, F8/TTL, offscreen rejection and world anchor passed");
}
