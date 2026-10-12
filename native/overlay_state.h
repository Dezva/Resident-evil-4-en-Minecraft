#ifndef RE4_OVERLAY_STATE_H
#define RE4_OVERLAY_STATE_H
/* Call only after capturing D3DSBT_ALL; every setter failure reaches restoration. */
static HRESULT overlay_prepare_state(IDirect3DDevice9 *device,IDirect3DBaseTexture9 *texture,int textured,int alpha){
    HRESULT hr=S_OK;
        if (SUCCEEDED(hr)) hr=device->lpVtbl->SetVertexShader(device,NULL);
        if (SUCCEEDED(hr)) hr=device->lpVtbl->SetPixelShader(device,NULL);
        if (SUCCEEDED(hr)) hr=device->lpVtbl->SetFVF(device,D3DFVF_XYZRHW|D3DFVF_DIFFUSE|(textured?D3DFVF_TEX1:0));
        if (SUCCEEDED(hr)) hr=device->lpVtbl->SetTexture(device,0,texture);
        if (SUCCEEDED(hr)) hr=device->lpVtbl->SetTextureStageState(device,0,D3DTSS_COLOROP,textured?D3DTOP_MODULATE:D3DTOP_SELECTARG1);
        if (SUCCEEDED(hr)) hr=device->lpVtbl->SetTextureStageState(device,0,D3DTSS_COLORARG1,textured?D3DTA_TEXTURE:D3DTA_DIFFUSE);
        if (SUCCEEDED(hr)) hr=device->lpVtbl->SetTextureStageState(device,0,D3DTSS_ALPHAOP,alpha?D3DTOP_MODULATE:D3DTOP_SELECTARG1);
        if (SUCCEEDED(hr)) hr=device->lpVtbl->SetTextureStageState(device,0,D3DTSS_ALPHAARG1,alpha?D3DTA_TEXTURE:D3DTA_DIFFUSE);
        if (SUCCEEDED(hr)) hr=device->lpVtbl->SetTextureStageState(device,1,D3DTSS_COLOROP,D3DTOP_DISABLE);
        if (SUCCEEDED(hr)) hr=device->lpVtbl->SetTextureStageState(device,0,D3DTSS_COLORARG2,D3DTA_DIFFUSE);
        if (SUCCEEDED(hr)) hr=device->lpVtbl->SetTextureStageState(device,0,D3DTSS_TEXCOORDINDEX,0);
        if (SUCCEEDED(hr)) hr=device->lpVtbl->SetTextureStageState(device,0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE);
        if (SUCCEEDED(hr)) hr=device->lpVtbl->SetTextureStageState(device,0,D3DTSS_RESULTARG,D3DTA_CURRENT);
        if(alpha && SUCCEEDED(hr))hr=device->lpVtbl->SetTextureStageState(device,0,D3DTSS_ALPHAARG2,D3DTA_DIFFUSE);
        if(alpha && SUCCEEDED(hr))hr=device->lpVtbl->SetRenderState(device,D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
        if(alpha && SUCCEEDED(hr))hr=device->lpVtbl->SetRenderState(device,D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
        if(alpha && SUCCEEDED(hr))hr=device->lpVtbl->SetRenderState(device,D3DRS_BLENDOP,D3DBLENDOP_ADD);
        if(alpha && SUCCEEDED(hr))hr=device->lpVtbl->SetRenderState(device,D3DRS_SEPARATEALPHABLENDENABLE,FALSE);
        static const struct {D3DSAMPLERSTATETYPE state;DWORD value;} samplers[]={
            {D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP},{D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP},
            {D3DSAMP_MINFILTER,D3DTEXF_POINT},{D3DSAMP_MAGFILTER,D3DTEXF_POINT},{D3DSAMP_MIPFILTER,D3DTEXF_NONE},
            {D3DSAMP_MAXMIPLEVEL,0},{D3DSAMP_MIPMAPLODBIAS,0},{D3DSAMP_SRGBTEXTURE,FALSE}};
        if (textured) for (unsigned i=0;SUCCEEDED(hr) && i<sizeof(samplers)/sizeof(samplers[0]);++i)
            hr=device->lpVtbl->SetSamplerState(device,0,samplers[i].state,samplers[i].value);
        static const struct {D3DRENDERSTATETYPE state;DWORD value;} states[]={
            {D3DRS_WRAP0,0},{D3DRS_ZENABLE,FALSE},{D3DRS_ZWRITEENABLE,FALSE},{D3DRS_ALPHABLENDENABLE,FALSE},
            {D3DRS_ALPHATESTENABLE,FALSE},{D3DRS_LIGHTING,FALSE},{D3DRS_FOGENABLE,FALSE},
            {D3DRS_SCISSORTESTENABLE,FALSE},{D3DRS_CLIPPLANEENABLE,0},{D3DRS_COLORWRITEENABLE,15},
            {D3DRS_CULLMODE,D3DCULL_NONE},{D3DRS_SRGBWRITEENABLE,FALSE},{D3DRS_STENCILENABLE,FALSE},
            {D3DRS_COLORWRITEENABLE1,0},{D3DRS_COLORWRITEENABLE2,0},{D3DRS_COLORWRITEENABLE3,0}};
        for (unsigned i=0;SUCCEEDED(hr) && i<sizeof(states)/sizeof(states[0]);++i)
            hr=device->lpVtbl->SetRenderState(device,states[i].state,alpha && states[i].state==D3DRS_ALPHABLENDENABLE?1:states[i].value);
        if (SUCCEEDED(hr)) hr=device->lpVtbl->SetRenderState(device,D3DRS_FILLMODE,D3DFILL_SOLID);
    return hr;
}
#endif
