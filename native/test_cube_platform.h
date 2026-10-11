#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#define WINAPI
#define HEAP_ZERO_MEMORY 8
#define E_POINTER ((int32_t)0x80004003)
#define E_FAIL ((int32_t)0x80004005)
#define E_NOINTERFACE ((int32_t)0x80004002)
#define S_OK 0
#define SUCCEEDED(hr) ((int32_t)(hr)>=0)
#define FAILED(hr) ((int32_t)(hr)<0)
#define FALSE 0
#define VK_F8 119
#define D3DBACKBUFFER_TYPE_MONO 0
#define D3DSBT_ALL 1
#define D3DFVF_XYZRHW 4
#define D3DFVF_DIFFUSE 64
#define D3DPT_LINELIST 2
#define D3DTSS_COLOROP 1
#define D3DTSS_COLORARG1 2
#define D3DTSS_ALPHAOP 4
#define D3DTOP_SELECTARG1 2
#define D3DTOP_DISABLE 1
#define D3DTA_DIFFUSE 0
#define D3DRS_ZENABLE 7
#define D3DRS_ZWRITEENABLE 14
#define D3DRS_ALPHABLENDENABLE 27
#define D3DRS_ALPHATESTENABLE 15
#define D3DRS_LIGHTING 137
#define D3DRS_FOGENABLE 28
#define D3DRS_SCISSORTESTENABLE 174
#define D3DRS_CLIPPLANEENABLE 152
#define D3DRS_COLORWRITEENABLE 168
#define D3DRS_CULLMODE 22
#define D3DRS_SRGBWRITEENABLE 194
#define D3DRS_STENCILENABLE 52
#define D3DRS_COLORWRITEENABLE1 190
#define D3DRS_COLORWRITEENABLE2 191
#define D3DRS_COLORWRITEENABLE3 192
#define D3DCULL_NONE 1
typedef int32_t HRESULT,LONG,INT,WINBOOL;
typedef uint32_t UINT,DWORD,ULONG;
typedef void *HWND,*HANDLE;
typedef struct {uint32_t Data1;uint16_t Data2,Data3;uint8_t Data4[8];} GUID;
typedef const GUID *REFIID;
typedef struct {unsigned marker;} D3DPRESENT_PARAMETERS;
typedef struct {DWORD X,Y,Width,Height;float MinZ,MaxZ;} D3DVIEWPORT9;
typedef struct IDirect3DStateBlock9 IDirect3DStateBlock9;
typedef struct {ULONG (*Release)(IDirect3DStateBlock9 *);HRESULT (*Capture)(IDirect3DStateBlock9 *);HRESULT (*Apply)(IDirect3DStateBlock9 *);} TestStateVtbl;
struct IDirect3DStateBlock9 {TestStateVtbl *lpVtbl;};
typedef struct IDirect3DSurface9 IDirect3DSurface9;
typedef struct {ULONG (*Release)(IDirect3DSurface9 *);} TestSurfaceVtbl;
struct IDirect3DSurface9 {TestSurfaceVtbl *lpVtbl;};
typedef struct {unsigned held;} CRITICAL_SECTION;
#include "test_device_api.h"
