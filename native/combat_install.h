#include "combat_profile.h"
/* Reuse the executable's incremental-link thunk instead of relocating any
 * instructions. Already-detoured functions and unknown builds remain disabled. */
static void combat_engine_install(unsigned char *base,DWORD size,IMAGE_NT_HEADERS32 *nt){
 combat_engine_ready=combat_hazards_ready=0;combat_hook_reason=1;
 if(nt->FileHeader.TimeDateStamp!=COMBAT_PROFILE_TIMESTAMP || size<=COMBAT_RAY_RVA+32)return;
 unsigned char *damage=base+COMBAT_DAMAGE_RVA,*ray=base+COMBAT_RAY_RVA,*message=base+COMBAT_MESSAGE_RVA;
 unsigned char *entries[8];unsigned count=0;IMAGE_SECTION_HEADER *sections=IMAGE_FIRST_SECTION(nt);
 for(unsigned target=0;target<3;target++){
  unsigned char *p=target==0?damage:target==1?ray:message;MEMORY_BASIC_INFORMATION m;
  if(!readable(p,32) || !VirtualQuery(p,&m,sizeof(m)) || m.State!=MEM_COMMIT || !(m.Protect&(PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)) || !combat_profile_entry(p,target)){combat_hook_reason=2;return;}
 }
 for(unsigned s=0;s<nt->FileHeader.NumberOfSections;s++){
  DWORD rva=sections[s].VirtualAddress,length=sections[s].Misc.VirtualSize;
  if(!(sections[s].Characteristics&IMAGE_SCN_MEM_EXECUTE) || rva>=size || length>size-rva || length<15)continue;
  if(!readable_span(base+rva,length))continue;
  for(DWORD i=5;i+10<=length && rva+i<0x20000;i++){
   unsigned char *p=base+rva+i;if(!combat_ilt_slot(p))continue;int32_t offset;memcpy(&offset,p+1,4);
   if((uintptr_t)(p+5)+(intptr_t)offset!=(uintptr_t)damage)continue;
   if(count==8){combat_hook_reason=3;return;}entries[count++]=p;
  }
 }
 /* One unambiguous shared thunk is required. Never scan/patch raw E8 bytes. */
 if(count!=1){combat_hook_reason=3;return;}
 unsigned calls=0;
 for(unsigned s=0;s<nt->FileHeader.NumberOfSections;s++){
  DWORD rva=sections[s].VirtualAddress,length=sections[s].Misc.VirtualSize;
  if(!(sections[s].Characteristics&IMAGE_SCN_MEM_EXECUTE) || rva>=size || length>size-rva || length<5 || !readable_span(base+rva,length))continue;
  for(DWORD i=0;i+5<=length;i++)if(base[rva+i]==0xE8){int32_t offset;memcpy(&offset,base+rva+i+1,4);if((uintptr_t)(base+rva+i+5)+(intptr_t)offset==(uintptr_t)entries[0])calls++;}
 }
 if(calls!=91){combat_hook_reason=4;return;}
 DWORD old,ignored;if(!VirtualProtect(entries[0],5,PAGE_EXECUTE_READWRITE,&old)){combat_hook_reason=5;return;}
 combat_original_damage=(CombatDamageFn)damage;combat_native_ray=(CombatRayFn)ray;combat_native_message=(CombatMessageFn)message;
 int32_t saved;memcpy(&saved,entries[0]+1,4);
 int32_t jump=(int32_t)((uintptr_t)combat_damage_hook-(uintptr_t)(entries[0]+5));memcpy(entries[0]+1,&jump,4);
 FlushInstructionCache(GetCurrentProcess(),entries[0],5);
 if(!VirtualProtect(entries[0],5,old,&ignored)){
  memcpy(entries[0]+1,&saved,4);FlushInstructionCache(GetCurrentProcess(),entries[0],5);VirtualProtect(entries[0],5,old,&ignored);combat_hook_reason=5;return;
 }
 combat_hook_reason=0;combat_engine_ready=1;
 combat_hazards_ready=1;
 for(unsigned mine=0;mine<2;mine++){
  DWORD rva=mine?COMBAT_MINE_IMPACT_RVA:COMBAT_TRAP_IMPACT_RVA;MEMORY_BASIC_INFORMATION m;
  if(rva>size || size-rva<0xC8 || !readable(base+rva,0xC8) || !VirtualQuery(base+rva,&m,sizeof(m)) || m.State!=MEM_COMMIT
   || !(m.Protect&(PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)) || !combat_profile_hazard(base+rva,mine))combat_hazards_ready=0;
 }
}
