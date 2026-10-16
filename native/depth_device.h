/* Draw world additions while the game's opaque pass still owns its Z surface.
 * Later native draws participate in the same test; the HUD remains at EndScene. */
static void cube_depth_after_draw(CubeDevice *f){
 if(!f->scene || !depth_requested(NULL))return;
 if(!world_mesh_requested()){f->world_drawn=1;world_mesh_release(f);}
 if(!cube_command_requested())f->depth_drawn=1;
 if(f->depth_drawn && f->world_drawn)return;
 IDirect3DDevice9 *d=f->inner;DWORD z=0,write=0,compare=0;
 if(FAILED(d->lpVtbl->GetRenderState(d,D3DRS_ZENABLE,&z)) || FAILED(d->lpVtbl->GetRenderState(d,D3DRS_ZWRITEENABLE,&write)) || FAILED(d->lpVtbl->GetRenderState(d,D3DRS_ZFUNC,&compare)) || z!=D3DZB_TRUE || !write)return;
 IDirect3DSurface9 *target=NULL,*back=NULL;
 HRESULT target_hr=d->lpVtbl->GetRenderTarget(d,0,&target),back_hr=d->lpVtbl->GetBackBuffer(d,0,0,D3DBACKBUFFER_TYPE_MONO,&back);
 int matches=SUCCEEDED(target_hr) && SUCCEEDED(back_hr) && target && back && target==back;
 if(target)target->lpVtbl->Release(target);
 if(back)back->lpVtbl->Release(back);
 if(!matches)return;
 if(!f->depth_seen){
  f->depth_gpu_valid=0;
  if(!cube_projection(f->depth_gx))return;
  f->depth_seen=1;f->depth_world_compare=compare;
  float view[12],fov;uint32_t player;D3DVIEWPORT9 vp;
  if(cube_camera(view,&fov,&player) && SUCCEEDED(d->lpVtbl->GetViewport(d,&vp))){
   float constants[256*4];
   if(SUCCEEDED(d->lpVtbl->GetVertexShaderConstantF(d,0,constants,256))){
    BlockProjection projection;
    if(block_shader_projection(constants,256,fov,(float)vp.Width,(float)vp.Height,compare,&projection)){
     memcpy(f->depth_gpu,projection.matrix,64);f->depth_gpu_valid=1;
    }
   }
  }
 }
 f->depth_stage=2;
 if(!f->depth_drawn)cube_render_device(f);
 world_mesh_render(f);
 f->depth_stage=0;
}
