#include "hud_policy.h"
static HudCommand *hud_command;static HANDLE hud_mapping;
static void hud_initialize(DWORD pid){
    wchar_t name[80];swprintf(name,80,L"Local\\RE4Bridge-HudCommand-%lu",(unsigned long)pid);
    HANDLE mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(HudCommand),name);
    if(!mapping)return;
    if(GetLastError()==ERROR_ALREADY_EXISTS){CloseHandle(mapping);return;}
    HudCommand *view=MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(HudCommand));
    if(!view){CloseHandle(mapping);return;}
    memset(view,0,sizeof(*view));view->magic=0x52453448;view->version=1;hud_command=view;hud_mapping=mapping;
}
static int hud_command_read(HudCommand *out){
    if(!hud_command)return 0;
    for(unsigned i=0;i<3;i++){
        uint32_t before=((volatile HudCommand *)hud_command)->sequence;
        MemoryBarrier();memcpy(out,hud_command,sizeof(*out));MemoryBarrier();
        if(before==((volatile HudCommand *)hud_command)->sequence && before==out->sequence && hud_valid(out))return 1;
    }
    return 0;
}
