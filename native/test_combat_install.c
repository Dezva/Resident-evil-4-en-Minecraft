#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "combat_profile.h"
typedef uint32_t DWORD;
typedef struct {DWORD VirtualAddress;union {DWORD VirtualSize;} Misc;DWORD Characteristics;} IMAGE_SECTION_HEADER;
typedef struct {struct {DWORD TimeDateStamp;unsigned NumberOfSections;} FileHeader;IMAGE_SECTION_HEADER sections[1];} IMAGE_NT_HEADERS32;
#define IMAGE_FIRST_SECTION(nt) ((nt)->sections)
#define IMAGE_SCN_MEM_EXECUTE 0x20000000
#define MEM_COMMIT 1
#define PAGE_EXECUTE_READ 2
#define PAGE_EXECUTE_READWRITE 4
#define PAGE_EXECUTE_WRITECOPY 8
typedef struct {unsigned State,Protect;} MEMORY_BASIC_INFORMATION;
static int bad_region,protect_fail,restore_fail,patches;
static int readable(const void *p,size_t n){return p && n;}
static int readable_span(const void *p,size_t n){return readable(p,n);}
static int VirtualQuery(const void *p,MEMORY_BASIC_INFORMATION *out,size_t size){(void)p;(void)size;*out=(MEMORY_BASIC_INFORMATION){MEM_COMMIT,bad_region?0:PAGE_EXECUTE_READ};return 1;}
static int VirtualProtect(void *p,size_t n,DWORD protect,DWORD *old){(void)p;assert(n==5);*old=PAGE_EXECUTE_READ;if(protect==PAGE_EXECUTE_READWRITE){patches++;return !protect_fail;}return !restore_fail;}
static void *GetCurrentProcess(void){return NULL;}
static void FlushInstructionCache(void *process,const void *p,size_t n){(void)process;(void)p;assert(n==5);}
typedef void *CombatDamageFn;typedef void *CombatRayFn;typedef void *CombatMessageFn;
static CombatDamageFn combat_original_damage;static CombatRayFn combat_native_ray;static CombatMessageFn combat_native_message;
static unsigned combat_engine_ready,combat_hook_reason,combat_hazards_ready;
static void combat_damage_hook(void){}
#include "combat_install.h"
static unsigned char image[0x200000];
static IMAGE_NT_HEADERS32 reset(void){
 memset(image,0,sizeof(image));patches=bad_region=protect_fail=restore_fail=0;combat_engine_ready=0;
 memcpy(image+COMBAT_DAMAGE_RVA,"\x55\x8b\xec\x53\x56\x57\xe8\x3f\xbc\xe4\xff\x0f\xb6\xf0\xc1\xe6",16);
 memcpy(image+COMBAT_RAY_RVA,"\x55\x8b\xec\x83\xec\x7c\xa1\x10\x5e\xc8\x00\x33\xc5\x89\x45\xfc",16);
 memcpy(image+COMBAT_MESSAGE_RVA,"\x55\x8b\xec\x8a\x45\x08\xd9\xee\x0c\x01\x56\x8b\xf1\x8a\x4d\x0c",16);
 memcpy(image+COMBAT_TRAP_IMPACT_RVA,"\x80\xbe\x2c\x03\0\0\0\x0f\x84\xb5\0\0\0",13);
 memcpy(image+COMBAT_TRAP_IMPACT_RVA+0xB8,"\xc7\x86\xfc\0\0\0\1\3\0\0",10);
 memcpy(image+COMBAT_MINE_IMPACT_RVA,"\x53\x56\x8b\xf0\x33\xdb\x66\x39\x9e\x24\x03\0\0\x7e\x35",15);
 memcpy(image+COMBAT_MINE_IMPACT_RVA+0xA1,"\xc7\x86\xfc\0\0\0\1\7\0\0",10);
 image[0x1100-5]=image[0x1100]=image[0x1100+5]=0xE9;int32_t jump=COMBAT_DAMAGE_RVA-0x1105;memcpy(image+0x1101,&jump,4);
 for(unsigned i=0;i<91;i++){unsigned call=0x5000+i*10;image[call]=0xE8;jump=(int32_t)(0x1100-call-5);memcpy(image+call+1,&jump,4);}
 return (IMAGE_NT_HEADERS32){.FileHeader={COMBAT_PROFILE_TIMESTAMP,1},.sections={{.VirtualAddress=0x1000,.Misc={0x1FF000},.Characteristics=IMAGE_SCN_MEM_EXECUTE}}};
}
int main(void){
 IMAGE_NT_HEADERS32 nt=reset();memset(image+COMBAT_RAY_RVA+7,0xAA,4);assert(combat_profile_entry(image+COMBAT_RAY_RVA,1));combat_engine_install(image,sizeof(image),&nt);assert(combat_engine_ready && combat_hazards_ready && combat_hook_reason==0 && patches==1);
 assert(combat_original_damage==image+COMBAT_DAMAGE_RVA && combat_native_ray==image+COMBAT_RAY_RVA && combat_native_message==image+COMBAT_MESSAGE_RVA);
 int32_t jump;memcpy(&jump,image+0x1101,4);assert((uintptr_t)(image+0x1105)+(intptr_t)jump==(uintptr_t)combat_damage_hook);
 for(unsigned failure=0;failure<4;failure++){
  nt=reset();unsigned rva=failure<2?COMBAT_TRAP_IMPACT_RVA:COMBAT_MINE_IMPACT_RVA;
  image[rva+((failure&1)?(failure<2?0xBE:0xA7):0)]^=1;
  combat_engine_install(image,sizeof(image),&nt);assert(combat_engine_ready && !combat_hazards_ready); // only unverified hazards disabled
 }
 for(unsigned failure=0;failure<10;failure++){
  nt=reset();
  if(failure==0)nt.FileHeader.TimeDateStamp++;
  if(failure==1)bad_region=1;
  if(failure==2)memset(image+COMBAT_RAY_RVA,0,4);
  if(failure==3)image[0x10FB]=0;
  if(failure==4)image[0x5000]=0;
  if(failure==5)protect_fail=1;
  if(failure==6){image[0x11FB]=image[0x1200]=image[0x1205]=0xE9;jump=COMBAT_DAMAGE_RVA-0x1205;memcpy(image+0x1201,&jump,4);}
  if(failure==7)image[COMBAT_DAMAGE_RVA]=0xE9;
  if(failure==8)restore_fail=1;
  if(failure==9)image[COMBAT_MESSAGE_RVA+15]^=1;
  unsigned char before[5];memcpy(before,image+0x1100,5);combat_engine_install(image,sizeof(image),&nt);assert(!combat_engine_ready && combat_hook_reason && !memcmp(before,image+0x1100,5));
 }
 puts("Native reaction profile: timestamp, executable/prologue checks, unique shared ILT, call evidence, no instruction relocation and rejected profiles leave code unchanged");
}
