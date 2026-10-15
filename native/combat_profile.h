#ifndef RE4_COMBAT_PROFILE_H
#define RE4_COMBAT_PROFILE_H
#include <string.h>
/* The public Steam UHD profile is deliberately narrow. No guessed alternative RVAs. */
#define COMBAT_PROFILE_TIMESTAMP 0x5A6192E5u
#define COMBAT_DAMAGE_RVA 0x1B5AF0u
#define COMBAT_RAY_RVA 0x1BB3C0u
#define COMBAT_MESSAGE_RVA 0x1AEEF0u
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
