#ifndef RE4_OVERLAY_POLICY_H
#define RE4_OVERLAY_POLICY_H
#include <stdint.h>
#include <math.h>
#define OVERLAY_MAX_WIDTH 1600u
#define OVERLAY_MAX_HEIGHT 900u
#define OVERLAY_MAX_PIXELS (OVERLAY_MAX_WIDTH*OVERLAY_MAX_HEIGHT)
typedef struct {
 uint32_t magic,version,sequence,stamp,enabled,generation,width,height,flags,revision;
 volatile uint32_t viewport_width,viewport_height,hud_candidates,hud_hidden,reserved0,reserved1;
 uint32_t selection;float origin[4],bounds[6];uint32_t reserved[5];
} OverlayHeader;
_Static_assert(sizeof(OverlayHeader)==128,"Overlay header layout");
typedef struct {OverlayHeader header;uint32_t pixels[OVERLAY_MAX_PIXELS];} OverlayShared;
static inline int overlay_valid(const OverlayHeader *h){
 if(h->magic!=0x5245344f || h->version!=1 || (h->sequence&1) || h->enabled>1 || h->flags>1 || h->selection>1)return 0;
 if(!h->enabled)return 1;
 if(!h->generation || h->generation>2147483647 || h->width<32 || h->width>OVERLAY_MAX_WIDTH || h->height<32 || h->height>OVERLAY_MAX_HEIGHT || !h->revision)return 0;
 if(h->selection){
  if(h->flags || !isfinite(h->origin[3]) || fabsf(h->origin[3])>3.141593f)return 0;
  for(unsigned i=0;i<3;i++)if(!isfinite(h->origin[i]) || fabsf(h->origin[i])>20000000)return 0;
  for(unsigned i=0;i<6;i++)if(!isfinite(h->bounds[i]) || fabsf(h->bounds[i])>20000000)return 0;
  for(unsigned i=0;i<3;i++)if(h->bounds[i+3]-h->bounds[i]<1 || h->bounds[i+3]-h->bounds[i]>7500)return 0;
 }
 return 1;
}
#endif
