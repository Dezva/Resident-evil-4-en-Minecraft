"""Generate typed standard D3D9 ABI forwarding from the installed MinGW SDK.
Only interface declarations are used, not third-party hook/game implementation.
Run with --sdk path/to/d3d9.h; checked-in output builds without a generator.
"""
import argparse
from pathlib import Path
import re

SPECIAL={'QueryInterface','AddRef','Release','BeginScene','EndScene','Reset','Present'}

def methods(sdk):
    source=sdk.read_text()
    body=source.split('DECLARE_INTERFACE_(IDirect3DDevice9,IUnknown)',1)[1].split('#undef INTERFACE',1)[0]
    result=[]
    pattern=r'STDMETHOD(?:_\(\s*(\w+)\s*,\s*(\w+)\s*\)|\(\s*(\w+)\s*\))\s*\((.*?)\)\s*PURE;'
    for match in re.finditer(pattern,body,re.S):
        returns,name=match.group(1) or 'HRESULT',match.group(2) or match.group(3)
        args=re.sub(r'^THIS_?\s*','',match.group(4).strip())
        parameters=[]
        for i,arg in enumerate(args.split(',') if args else []):
            arg=' '.join(arg.split());last=re.search(r'(\w+)\s*$',arg)
            if last and arg[:last.start()].strip():
                typ=arg[:last.start()].strip();argument=last.group(1)
            else:typ=arg;argument=f'arg{i}'
            parameters.append((typ,argument))
        result.append((returns,name,parameters))
    if len(result)!=119:raise ValueError(f'Expected119 SDK methods, found{len(result)}')
    return result

def generate(sdk,root):
    signatures=methods(sdk)
    output=['/* Generated standard IDirect3DDevice9 ABI. See tools/generate_device_forwarders.py. */']
    api=['/* Portable fixture types from the standard SDK declarations. No driver execution. */']
    types=set()
    for returns,name,params in signatures:
        for typ,_ in params:
            base=re.sub(r'\bconst\b|\*','',typ).strip();types.add(base)
        if name in SPECIAL:continue
        declaration=', '.join(['IDirect3DDevice9 *iface']+[f'{t} {a}' for t,a in params])
        call=', '.join(['f->inner']+[a for _,a in params])
        output.extend([f'static {returns} WINAPI cube_{name}({declaration}) {{',
          '    CubeDevice *f=(CubeDevice *)iface;cube_method_enter(f);',
          f'    '+('' if returns=='void' else f'{returns} result=')+f'f->inner->lpVtbl->{name}({call});',
          '    DWORD error=GetLastError();cube_method_leave(f,error);',
          '    return;' if returns=='void' else '    return result;','}'])
    output.append('static IDirect3DDevice9Vtbl cube_device_vtable={'+','.join('cube_'+n for _,n,_ in signatures)+'};')
    (root/'native/device_forwarders.h').write_text('\n'.join(output)+'\n')
    reserved={'REFIID','DWORD','UINT','ULONG','INT','WINBOOL','HWND','HANDLE','float','int','void',
              'IDirect3DDevice9','D3DPRESENT_PARAMETERS','D3DVIEWPORT9','IDirect3DStateBlock9','IDirect3DSurface9'}
    api+=['typedef int '+t+';' for t in sorted(types-reserved)]
    api+=['typedef struct IDirect3DDevice9 IDirect3DDevice9;','typedef struct {']
    for returns,name,params in signatures:
        declaration=', '.join(['IDirect3DDevice9 *self']+[f'{t} {a}' for t,a in params])
        api.append(f'    {returns} (*{name})({declaration});')
    api+=['} IDirect3DDevice9Vtbl;','struct IDirect3DDevice9 {IDirect3DDevice9Vtbl *lpVtbl;};']
    (root/'native/test_device_api.h').write_text('\n'.join(api)+'\n')
    # Distinct expected argument/result sentinels exercise every passthrough slot.
    fake=[];calls=[]
    for i,(returns,name,params) in enumerate(signatures):
        if name in SPECIAL:continue
        values=[f'({t})'+(f'{i+j+1}.25f' if t=='float' else f'(uintptr_t){0x10000+i*128+j*8}') for j,(t,_) in enumerate(params)]
        result=f'({returns})'+(f'{i+1}.5f' if returns=='float' else f'(uintptr_t){0x7000+i}')
        declaration=', '.join(['IDirect3DDevice9 *self']+[f'{t} {a}' for t,a in params])
        checks=''.join(f'assert({a}=={v});' for (_,a),v in zip(params,values))
        fake.append(f'static {returns} fake_{name}({declaration}) '+'{assert(self==&inner);'+checks+'forwarded++;last_error=444;'+('' if returns=='void' else 'return '+result+';')+'}')
        call=f'w->lpVtbl->{name}('+', '.join(['w']+values)+')'
        calls.append((call+';' if returns=='void' else 'assert('+call+'=='+result+');')+'assert(last_error==444);')
    fake.append('static IDirect3DDevice9Vtbl native_table={'+','.join('fake_'+n for _,n,_ in signatures)+'};')
    (root/'native/test_device_passthrough.h').write_text('\n'.join(fake)+'\n')
    (root/'native/test_device_calls.h').write_text('\n'.join(calls)+'\nassert(forwarded==112);\n')

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--sdk',type=Path,required=True)
    args=parser.parse_args();generate(args.sdk,Path(__file__).resolve().parents[1])
