#ifndef RE4_CUBE_POLICY_H
#define RE4_CUBE_POLICY_H
#include <stdint.h>
typedef struct {uint32_t magic,version,sequence,stamp,enabled,epoch,mode,reserved;float bounds[6];} CubeCommand;
_Static_assert(sizeof(CubeCommand)==56,"Cube command v2 layout");
typedef struct {uint32_t epoch,blocked,blocked_reason;unsigned fatal,lost,anchored;float bounds[6];} CubeGate;
enum {CUBE_OFF=1,CUBE_STALE,CUBE_F8,CUBE_LOST,CUBE_RESTORE,CUBE_INTERFACE,CUBE_CAMERA,CUBE_TARGET,CUBE_GRAPHICS};
enum {CUBE_BEGIN,CUBE_FRAME,CUBE_DRAW,CUBE_FAILURE,CUBE_RESTORE_FAILURE,CUBE_ESCAPE,
      CUBE_RESET,CUBE_RESET_FAILURE,CUBE_PRESENT_FAILURE,CUBE_REJECT,CUBE_CAMERA_REJECT,CUBE_PASS_SKIP,CUBE_ACTIVE};
static inline int cube_gate(CubeGate *gate,const CubeCommand *command,uint32_t now,int f8) {
    if (gate->fatal) return CUBE_RESTORE;
    if (!command || !command->enabled || !command->epoch || command->enabled!=1) return CUBE_OFF;
    if (f8) {gate->blocked=command->epoch;gate->blocked_reason=CUBE_F8;return CUBE_F8;}
    if (gate->blocked==command->epoch) return gate->blocked_reason?gate->blocked_reason:CUBE_F8;
    if (gate->lost) {gate->blocked=command->epoch;gate->blocked_reason=CUBE_LOST;return CUBE_LOST;}
    if ((uint32_t)(now-command->stamp)>=500u) return CUBE_STALE;
    if (gate->epoch!=command->epoch) {gate->epoch=command->epoch;gate->anchored=0;}
    return 0;
}
#endif
