#include "overlay_state.h"
#include "block_surface.h"
#include "block_texture.h"
/* Executes only within a successful BeginScene, on a freshly wrapped device.
 * Texture pixels persist per facade; state backups never retain game resources between draws. */
static void __attribute__((noinline)) cube_render_enabled_device(CubeDevice *f) {
    CubeCommand command;
    int complete=cube_command_read(&command);
    int reason=cube_gate(&f->gate,complete?&command:NULL,GetTickCount(),GetAsyncKeyState(VK_F8)&0x8000);
    if (reason || f->escaped || f->resetting || !f->scene) {block_texture_cache_release(&f->block_texture);cube_record(CUBE_REJECT,f->escaped?CUBE_INTERFACE:reason?reason:CUBE_OFF);return;}
    IDirect3DDevice9 *device=f->inner;
    IDirect3DSurface9 *target=NULL,*back=NULL;
    HRESULT hr=device->lpVtbl->GetRenderTarget(device,0,&target);
    if (FAILED(hr) || !target) {f->gate.blocked=command.epoch;f->gate.blocked_reason=CUBE_GRAPHICS;cube_record(CUBE_FAILURE,(uint32_t)(FAILED(hr)?hr:E_FAIL));return;}
    hr=device->lpVtbl->GetBackBuffer(device,0,0,D3DBACKBUFFER_TYPE_MONO,&back);
    int matches=SUCCEEDED(hr) && back && target==back;
    if (SUCCEEDED(hr) && back) back->lpVtbl->Release(back);
    target->lpVtbl->Release(target);
    if (FAILED(hr)) {f->gate.blocked=command.epoch;f->gate.blocked_reason=CUBE_GRAPHICS;cube_record(CUBE_FAILURE,(uint32_t)hr);return;}
    if (!matches) {cube_record(CUBE_PASS_SKIP,0);return;}
    float view[12],fov;uint32_t player;
    if (!cube_camera(view,&fov,&player)) {cube_record(CUBE_CAMERA_REJECT,0);return;}
    if (f->player!=player) {f->player=player;f->gate.anchored=0;}
    D3DVIEWPORT9 viewport;
    hr=device->lpVtbl->GetViewport(device,&viewport);
    if (FAILED(hr)) {f->gate.blocked=command.epoch;f->gate.blocked_reason=CUBE_GRAPHICS;cube_record(CUBE_FAILURE,(uint32_t)hr);return;}
    BlockVertex lines[24*CUBE_MAX_BOXES];void *vertices=lines;unsigned count=0;
    BlockSurfaceMesh *surface=NULL;
    int solid=command.mode==2 && command.style!=0;
    int textured=solid && command.style>=2;
    int atlas=solid && command.style==3;
    if (command.mode==2) {
        float scene_view[12];
        if (!command.count || command.count>CUBE_MAX_BOXES || command.style>3 || !block_scene_view(view,command.origin,scene_view)) {
            cube_record(CUBE_CAMERA_REJECT,0);return;
        }
        // Validate the entire bounded snapshot before rendering any part of it.
        for (unsigned i=0;i<command.count;++i) if (!block_valid_shape_bounds(command.boxes[i])) {
            cube_record(CUBE_CAMERA_REJECT,0);return;
        }
        if (solid) {
            surface=HeapAlloc(GetProcessHeap(),0,sizeof(*surface));
            if (!surface) {f->gate.blocked=command.epoch;f->gate.blocked_reason=CUBE_GRAPHICS;cube_record(CUBE_FAILURE,(uint32_t)E_OUTOFMEMORY);return;}
            if(atlas) {
                if(!command.tile_count || command.tile_count>BLOCK_MAX_TILES){HeapFree(GetProcessHeap(),0,surface);cube_record(CUBE_CAMERA_REJECT,0);return;}
                for(unsigned i=0;i<command.count;++i)for(unsigned j=0;j<6;++j) {
                    if(command.materials[i][j].tile>=command.tile_count){HeapFree(GetProcessHeap(),0,surface);cube_record(CUBE_CAMERA_REJECT,0);return;}
                    for(unsigned k=0;k<8;++k)if(!isfinite(command.materials[i][j].uv[k]) || command.materials[i][j].uv[k]<0 || command.materials[i][j].uv[k]>1){HeapFree(GetProcessHeap(),0,surface);cube_record(CUBE_CAMERA_REJECT,0);return;}
                }
            }
            vertices=surface->vertices;
            count=block_surfaces_materials(command.boxes,command.count,scene_view,fov,(float)viewport.Width,(float)viewport.Height,surface,atlas?command.materials:NULL);
        } else for (unsigned i=0;i<command.count;++i)
            count+=block_lines(command.boxes[i],scene_view,fov,(float)viewport.Width,(float)viewport.Height,lines+count);
    } else if (command.mode==1) {
        if (!block_valid_bounds(command.bounds)) {cube_record(CUBE_CAMERA_REJECT,0);return;}
        memcpy(f->gate.bounds,command.bounds,sizeof(f->gate.bounds));
        f->gate.anchored=1;
    } else if (!f->gate.anchored) {
        /* Invert the rigid view matrix and put the center 600 native units
           along negative camera Z. The anchor stays fixed until a new lease. */
        for (unsigned axis=0;axis<3;++axis) {
            float eye=0;for (unsigned row=0;row<3;++row) eye-=view[4*row+axis]*view[4*row+3];
            float center=eye-600.f*view[8+axis];
            f->gate.bounds[axis]=center-50.f;f->gate.bounds[axis+3]=center+50.f;
        }
        if (!block_lines(f->gate.bounds,view,fov,(float)viewport.Width,(float)viewport.Height,lines)) {
            cube_record(CUBE_CAMERA_REJECT,0);return;
        }
        f->gate.anchored=1;
    }
    if (command.mode!=2) count=block_lines(f->gate.bounds,view,fov,(float)viewport.Width,(float)viewport.Height,lines);
    if (!count) {if (surface) HeapFree(GetProcessHeap(),0,surface);cube_record(CUBE_PASS_SKIP,0);return;}
    for (unsigned i=0;i<count;++i) {
        if (solid) {
            surface->vertices[i].x+=viewport.X;surface->vertices[i].y+=viewport.Y;
            if (textured) {unsigned shade=(surface->vertices[i].color&255)*255/184;surface->vertices[i].color=0xFF000000|(shade*0x010101);}
        } else {lines[i].x+=viewport.X;lines[i].y+=viewport.Y;}
    }
    IDirect3DStateBlock9 *backup=NULL;
    hr=device->lpVtbl->CreateStateBlock(device,D3DSBT_ALL,&backup);
    if (FAILED(hr) || !backup) {if (surface) HeapFree(GetProcessHeap(),0,surface);f->gate.blocked=command.epoch;f->gate.blocked_reason=CUBE_GRAPHICS;cube_record(CUBE_FAILURE,(uint32_t)(FAILED(hr)?hr:E_FAIL));return;}
    hr=backup->lpVtbl->Capture(backup);
    if (SUCCEEDED(hr)) {
        if (textured) hr=block_texture_cached(device,&f->block_texture,atlas?&command:NULL);
        else block_texture_cache_release(&f->block_texture);
        if(SUCCEEDED(hr))hr=overlay_prepare_state(device,textured?(IDirect3DBaseTexture9*)f->block_texture.texture:NULL,textured,atlas);
        if (SUCCEEDED(hr)) hr=device->lpVtbl->DrawPrimitiveUP(device,solid?D3DPT_TRIANGLELIST:D3DPT_LINELIST,count/(solid?3:2),vertices,solid?sizeof(BlockSurfaceVertex):sizeof(BlockVertex));
        HRESULT restored=backup->lpVtbl->Apply(backup);
        if (FAILED(restored)) {f->gate.fatal=1;cube_record(CUBE_RESTORE_FAILURE,(uint32_t)restored);}
        else if (SUCCEEDED(hr)) cube_record(CUBE_DRAW,(uint32_t)hr);
    }
    backup->lpVtbl->Release(backup);
    if (surface) HeapFree(GetProcessHeap(),0,surface);
    if (FAILED(hr) || f->gate.fatal)block_texture_cache_release(&f->block_texture);
    if (FAILED(hr)) {f->gate.blocked=command.epoch;f->gate.blocked_reason=CUBE_GRAPHICS;cube_record(CUBE_FAILURE,(uint32_t)hr);}
}
/* Keep the large scene/vertex stack frame out of ordinary, dormant RE4 frames. */
static void cube_render_device(CubeDevice *f){
    if(!cube_command_requested()){
        block_texture_cache_release(&f->block_texture);cube_record(CUBE_REJECT,CUBE_OFF);return;
    }
    cube_render_enabled_device(f);
}
