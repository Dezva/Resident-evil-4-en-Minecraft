#ifndef RE4_COMBAT_PROFILE_H
#define RE4_COMBAT_PROFILE_H
#include <string.h>
/* The public Steam UHD profile is deliberately narrow. No guessed alternative RVAs. */
#define COMBAT_PROFILE_TIMESTAMP 0x5A6192E5u
#define COMBAT_DAMAGE_RVA 0x1B5AF0u
#define COMBAT_RAY_RVA 0x1BB3C0u
#define COMBAT_MESSAGE_RVA 0x1AEEF0u
#define COMBAT_TRAP_IMPACT_RVA 0xA8840u
#define COMBAT_MINE_IMPACT_RVA 0xAA920u
/* Read-only trap diagnostic: these handlers consume the same native message,
 * set HP zero themselves and dispatch native closing/explosion routines. */
static inline int combat_profile_hazard(const unsigned char *p,unsigned mine){
 static const unsigned char trap[]={0x80,0xBE,0x2C,0x03,0,0,0,0x0F,0x84,0xB5,0,0,0};
 static const unsigned char bomb[]={0x53,0x56,0x8B,0xF0,0x33,0xDB,0x66,0x39,0x9E,0x24,0x03,0,0,0x7E,0x35};
 static const unsigned char close[]={0xC7,0x86,0xFC,0,0,0,1,3,0,0},explode[]={0xC7,0x86,0xFC,0,0,0,1,7,0,0};
 return mine?(!memcmp(p,bomb,sizeof(bomb)) && !memcmp(p+0xA1,explode,sizeof(explode))):(!memcmp(p,trap,sizeof(trap)) && !memcmp(p+0xB8,close,sizeof(close)));
}
/* Prefixes observed in the user's read-only 2026-10-15 diagnostic. The ray
 * cookie address is relocated by ASLR and is deliberately not compared. */
static inline int combat_profile_entry(const unsigned char *p,unsigned role){
 static const unsigned char damage[16]={0x55,0x8B,0xEC,0x53,0x56,0x57,0xE8,0x3F,0xBC,0xE4,0xFF,0x0F,0xB6,0xF0,0xC1,0xE6};
 static const unsigned char message[16]={0x55,0x8B,0xEC,0x8A,0x45,0x08,0xD9,0xEE,0x0C,0x01,0x56,0x8B,0xF1,0x8A,0x4D,0x0C};
 static const unsigned char ray[7]={0x55,0x8B,0xEC,0x83,0xEC,0x7C,0xA1},tail[5]={0x33,0xC5,0x89,0x45,0xFC};
 if(role==0)return !memcmp(p,damage,sizeof(damage));
 if(role==1)return !memcmp(p,ray,sizeof(ray)) && !memcmp(p+11,tail,sizeof(tail));
 return role==2 && !memcmp(p,message,sizeof(message));
}
/* An ILT slot must be an E9 between other five-byte E9 slots. */
static inline int combat_ilt_slot(const unsigned char *p){return p[-5]==0xE9 && p[0]==0xE9 && p[5]==0xE9;}
#endif
