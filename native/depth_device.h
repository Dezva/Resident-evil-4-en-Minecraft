/* Select a complete world pass before latching anything. Sky/clear/auxiliary
 * draws must neither poison the frame projection nor clear collision readiness. */
static void cube_depth_after_draw(CubeDevice *f){
 if(!f->scene || !depth_requested(NULL))return;
 if(!world_mesh_requested()){f->world_drawn=1;world_mesh_release(f);}
 if(!cube_command_requested())f->depth_drawn=1;
 if(f->depth_drawn && f->world_drawn)return;
 IDirect3DDevice9 *d=f->inner;DWORD z=0,write=0,compare=0;
 if(FAILED(d->lpVtbl->GetRenderState(d,D3DRS_ZFUNC,&compare)) || !block_depth_world_compare(compare))return;
 if(FAILED(d->lpVtbl->GetRenderState(d,D3DRS_ZENABLE,&z)) || FAILED(d->lpVtbl->GetRenderState(d,D3DRS_ZWRITEENABLE,&write)) || z!=D3DZB_TRUE || !write)return;
 IDirect3DSurface9 *target=NULL,*back=NULL;
 HRESULT target_hr=d->lpVtbl->GetRenderTarget(d,0,&target),back_hr=d->lpVtbl->GetBackBuffer(d,0,0,D3DBACKBUFFER_TYPE_MONO,&back);
 int matches=SUCCEEDED(target_hr) && SUCCEEDED(back_hr) && target && back && target==back;
 if(target)target->lpVtbl->Release(target);
 if(back)back->lpVtbl->Release(back);
 if(!matches)return;
 if(!f->depth_seen){
  float view[12],fov,gx[16];uint32_t player;D3DVIEWPORT9 vp;
  if(!cube_camera(view,&fov,&player) || FAILED(d->lpVtbl->GetViewport(d,&vp)) || !isfinite(vp.MinZ+vp.MaxZ) || vp.MinZ<0 || vp.MaxZ>1 || vp.MinZ>=vp.MaxZ || vp.Width<32 || vp.Height<32 || vp.Width>16384 || vp.Height>16384 || !cube_projection(gx))return;
  if(!f->depth_gpu_checked){
   f->depth_gpu_checked=1;f->depth_gpu_valid=0;
   float constants[256*4];BlockProjection candidate;
   if(SUCCEEDED(d->lpVtbl->GetVertexShaderConstantF(d,0,constants,256)) && block_shader_projection(constants,256,fov,(float)vp.Width,(float)vp.Height,compare,&candidate)){
    memcpy(f->depth_gpu,candidate.matrix,64);f->depth_gpu_valid=1;
   }
  }
  BlockProjection candidate;
  int gpu=f->depth_gpu_valid && block_projection(f->depth_gpu,fov,(float)vp.Width,(float)vp.Height,&candidate) && candidate.reverse==(compare==5 || compare==7);
  if(!gpu && !block_gx_projection(gx,fov,(float)vp.Width,(float)vp.Height,compare,&candidate))return;
  f->depth_gpu_valid=gpu;memcpy(f->depth_gx,gx,64);f->depth_world_compare=compare;f->depth_seen=1;
 }
 f->depth_stage=2;
 /* A torn command or a rejected camera gets another chance next frame, never
  * hundreds of geometry builds/GPU queries in one native frame. */
 if(!f->depth_drawn){cube_render_device(f);f->depth_drawn=1;}
 if(!f->world_drawn){world_mesh_render(f);f->world_drawn=1;}
 f->depth_stage=0;
}
