#ifndef RE4_HUD_POLICY_H
#define RE4_HUD_POLICY_H
#include <stdint.h>
typedef struct {uint32_t magic,version,sequence,stamp,enabled,generation,pixels[256*32];} HudCommand;
_Static_assert(sizeof(HudCommand)==32792,"HUD v1 layout");
static inline int hud_valid(const HudCommand *c){return c->magic==0x52453448 && c->version==1 && !(c->sequence&1) && c->enabled<=1 && (!c->enabled || (c->generation>0 && c->generation<=2147483647));}
#endif
