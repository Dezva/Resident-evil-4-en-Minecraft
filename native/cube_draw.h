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
#ifdef RE4_DEPTH_RUNTIME
    BlockProjection depth={0};uint32_t depth_generation=0;
    int use_depth=solid && depth_requested(&depth_generation),depth_fallback=0;
    if(use_depth){
        // A stale generation is a control rejection, never a compatibility fallback.
        if(depth_generation!=command.arm_generation){depth_report(8,f->depth_stage,NULL,&viewport,0);return;}
        if(!isfinite(viewport.MinZ+viewport.MaxZ) || viewport.MinZ<0 || viewport.MaxZ>1 || viewport.MinZ>=viewport.MaxZ){
            depth_report(2,f->depth_stage,NULL,&viewport,0);use_depth=0;depth_fallback=2;
        }
        if(use_depth){
            IDirect3DSurface9 *depth_surface=NULL;
            hr=device->lpVtbl->GetDepthStencilSurface(device,&depth_surface);
            if(FAILED(hr) || !depth_surface){depth_report(3,f->depth_stage,NULL,&viewport,0);use_depth=0;depth_fallback=3;}
            if(depth_surface)depth_surface->lpVtbl->Release(depth_surface);
        }
        if(use_depth){
            float projection[16]={0};hr=device->lpVtbl->GetTransform(device,D3DTS_PROJECTION,(D3DMATRIX*)projection);
            unsigned source=1;
            if(FAILED(hr) || !block_projection(projection,fov,(float)viewport.Width,(float)viewport.Height,&depth)){
                source=2;
                if(!f->depth_seen || !block_gx_projection(f->depth_gx,fov,(float)viewport.Width,(float)viewport.Height,f->depth_world_compare,&depth)){
                    BlockProjection rejected={0};for(unsigned i=0;i<16;i++)if(isfinite(projection[i]))rejected.matrix[i]=projection[i];
                    depth_report(2,f->depth_stage,&rejected,&viewport,0);
                    depth_observe(0,f->depth_gx,fov,f->depth_world_compare,f->depth_seen);
                    use_depth=0;depth_fallback=2;
                }
            }
            if(use_depth)depth_observe(source,f->depth_gx,fov,f->depth_world_compare,f->depth_seen);
        }
        // Flat compatibility drawing belongs at the final pass, after world geometry.
        // Leave the validated depth path at the earlier pre-HUD depth clear.
        if(depth_fallback && f->depth_stage==1)return;
    }
#endif
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
            #ifdef RE4_DEPTH_RUNTIME
            count=block_surfaces_materials_projected(command.boxes,command.count,scene_view,fov,(float)viewport.Width,(float)viewport.Height,surface,atlas?command.materials:NULL,use_depth?depth.near_plane:1.f,use_depth?depth.matrix[0]*viewport.Width*.5f:0);
#else
            count=block_surfaces_materials(command.boxes,command.count,scene_view,fov,(float)viewport.Width,(float)viewport.Height,surface,atlas?command.materials:NULL);
#endif
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
#ifdef RE4_DEPTH_RUNTIME
            if(use_depth)surface->vertices[i].z=viewport.MinZ+(viewport.MaxZ-viewport.MinZ)*block_depth(&depth,surface->vertices[i].rhw);
#endif
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
#ifdef RE4_DEPTH_RUNTIME
        if(use_depth && SUCCEEDED(hr))hr=device->lpVtbl->SetRenderState(device,D3DRS_ZENABLE,D3DZB_TRUE);
        if(use_depth && SUCCEEDED(hr))hr=device->lpVtbl->SetRenderState(device,D3DRS_ZWRITEENABLE,TRUE);
        if(use_depth && SUCCEEDED(hr))hr=device->lpVtbl->SetRenderState(device,D3DRS_ZFUNC,depth.reverse?D3DCMP_GREATEREQUAL:D3DCMP_LESSEQUAL);
        if(use_depth && atlas && SUCCEEDED(hr))hr=device->lpVtbl->SetRenderState(device,D3DRS_ALPHATESTENABLE,TRUE);
        if(use_depth && atlas && SUCCEEDED(hr))hr=device->lpVtbl->SetRenderState(device,D3DRS_ALPHAFUNC,D3DCMP_GREATER);
        if(use_depth && atlas && SUCCEEDED(hr))hr=device->lpVtbl->SetRenderState(device,D3DRS_ALPHAREF,0);
#endif
        if (SUCCEEDED(hr)) hr=device->lpVtbl->DrawPrimitiveUP(device,solid?D3DPT_TRIANGLELIST:D3DPT_LINELIST,count/(solid?3:2),vertices,solid?sizeof(BlockSurfaceVertex):sizeof(BlockVertex));
        HRESULT restored=backup->lpVtbl->Apply(backup);
        if (FAILED(restored)) {f->gate.fatal=1;cube_record(CUBE_RESTORE_FAILURE,(uint32_t)restored);}
        else if (SUCCEEDED(hr)) {cube_record(CUBE_DRAW,(uint32_t)hr);
#ifdef RE4_DEPTH_RUNTIME
            if(use_depth){f->depth_drawn=1;depth_report(0,f->depth_stage,&depth,&viewport,1);}
            else if(depth_fallback){f->depth_drawn=1;depth_fallback_draw();}
#endif
        }
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
