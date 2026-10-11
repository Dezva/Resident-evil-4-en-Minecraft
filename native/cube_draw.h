/* Executes only within a successful BeginScene, on a freshly wrapped device.
 * All temporary COM references are released in this call; no persistent GPU resources. */
static void cube_render_device(CubeDevice *f) {
    CubeCommand command={0};
    int complete=cube_command_read(&command);
    int reason=cube_gate(&f->gate,complete?&command:NULL,GetTickCount(),GetAsyncKeyState(VK_F8)&0x8000);
    if (reason || f->escaped || f->resetting || !f->scene) {cube_record(CUBE_REJECT,f->escaped?CUBE_INTERFACE:reason?reason:CUBE_OFF);return;}
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
    BlockVertex vertices[24];
    if (command.mode==1) {
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
        if (!block_lines(f->gate.bounds,view,fov,(float)viewport.Width,(float)viewport.Height,vertices)) {
            cube_record(CUBE_CAMERA_REJECT,0);return;
        }
        f->gate.anchored=1;
    }
    unsigned count=block_lines(f->gate.bounds,view,fov,(float)viewport.Width,(float)viewport.Height,vertices);
    if (!count) {cube_record(CUBE_PASS_SKIP,0);return;}
    for (unsigned i=0;i<count;++i) {vertices[i].x+=viewport.X;vertices[i].y+=viewport.Y;}
    IDirect3DStateBlock9 *backup=NULL;
    hr=device->lpVtbl->CreateStateBlock(device,D3DSBT_ALL,&backup);
    if (FAILED(hr) || !backup) {f->gate.blocked=command.epoch;f->gate.blocked_reason=CUBE_GRAPHICS;cube_record(CUBE_FAILURE,(uint32_t)(FAILED(hr)?hr:E_FAIL));return;}
    hr=backup->lpVtbl->Capture(backup);
    if (SUCCEEDED(hr)) {
        hr=device->lpVtbl->SetVertexShader(device,NULL);
        if (SUCCEEDED(hr)) hr=device->lpVtbl->SetPixelShader(device,NULL);
        if (SUCCEEDED(hr)) hr=device->lpVtbl->SetFVF(device,D3DFVF_XYZRHW|D3DFVF_DIFFUSE);
        if (SUCCEEDED(hr)) hr=device->lpVtbl->SetTexture(device,0,NULL);
        if (SUCCEEDED(hr)) hr=device->lpVtbl->SetTextureStageState(device,0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
        if (SUCCEEDED(hr)) hr=device->lpVtbl->SetTextureStageState(device,0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);
        if (SUCCEEDED(hr)) hr=device->lpVtbl->SetTextureStageState(device,0,D3DTSS_ALPHAOP,D3DTOP_DISABLE);
        if (SUCCEEDED(hr)) hr=device->lpVtbl->SetTextureStageState(device,1,D3DTSS_COLOROP,D3DTOP_DISABLE);
        static const struct {D3DRENDERSTATETYPE state;DWORD value;} states[]={
            {D3DRS_ZENABLE,FALSE},{D3DRS_ZWRITEENABLE,FALSE},{D3DRS_ALPHABLENDENABLE,FALSE},
            {D3DRS_ALPHATESTENABLE,FALSE},{D3DRS_LIGHTING,FALSE},{D3DRS_FOGENABLE,FALSE},
            {D3DRS_SCISSORTESTENABLE,FALSE},{D3DRS_CLIPPLANEENABLE,0},{D3DRS_COLORWRITEENABLE,15},
            {D3DRS_CULLMODE,D3DCULL_NONE},{D3DRS_SRGBWRITEENABLE,FALSE},{D3DRS_STENCILENABLE,FALSE},
            {D3DRS_COLORWRITEENABLE1,0},{D3DRS_COLORWRITEENABLE2,0},{D3DRS_COLORWRITEENABLE3,0}};
        for (unsigned i=0;SUCCEEDED(hr) && i<sizeof(states)/sizeof(states[0]);++i)
            hr=device->lpVtbl->SetRenderState(device,states[i].state,states[i].value);
        if (SUCCEEDED(hr)) hr=device->lpVtbl->DrawPrimitiveUP(device,D3DPT_LINELIST,count/2,vertices,sizeof(BlockVertex));
        HRESULT restored=backup->lpVtbl->Apply(backup);
        if (FAILED(restored)) {f->gate.fatal=1;cube_record(CUBE_RESTORE_FAILURE,(uint32_t)restored);}
        else if (SUCCEEDED(hr)) cube_record(CUBE_DRAW,(uint32_t)hr);
    }
    backup->lpVtbl->Release(backup);
    if (FAILED(hr)) {f->gate.blocked=command.epoch;f->gate.blocked_reason=CUBE_GRAPHICS;cube_record(CUBE_FAILURE,(uint32_t)hr);}
}
