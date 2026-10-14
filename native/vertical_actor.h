/* Absolute Minecraft feet become the real cCoord::pos_94.y. No velocity,
   old position, animation, floor flags or other actors are modified. */
static uint32_t vertical_actor_player,vertical_actor_generation;static float vertical_actor_origin;
static void runtime_vertical_actor(const ControlFrame *control,int complete,uint32_t now){
    runtime_actor_active=0;
    if(!runtime_vertical_valid || !complete || !state->armed || !control->armed || state->stop
        || control->generation!=runtime_lift_generation || control->generation!=state->arm_id || (control->flags&3u)!=3u
        || (uint32_t)(now-control->stamp)>=500 || (uint32_t)(now-runtime_request_stamp)>=500
        || runtime_scripted() || (GetAsyncKeyState(VK_F8)&0x8000))return;
    uint32_t player=current_player();
    if(!player || !writable((void*)(uintptr_t)(player+0x94),12))return;
    float raw[3];memcpy(raw,(void*)(uintptr_t)(player+0x94),12);
    for(unsigned i=0;i<3;i++)if(!isfinite(raw[i]) || fabsf(raw[i])>20000000.f)return;
    if(!isfinite(runtime_physics_y) || fabsf(runtime_physics_y)>20000000.f)return;
    if(vertical_actor_generation!=control->generation){
        vertical_actor_generation=control->generation;vertical_actor_player=player;vertical_actor_origin=raw[1];
    }
    if(vertical_actor_player!=player || fabsf(runtime_physics_y-vertical_actor_origin)>128*runtime_vertical_scale){
        InterlockedCompareExchange(&state->stop,1,0);return;
    }
    if(*player_slot!=player || state->stop || !state->armed || state->arm_id!=control->generation || (GetAsyncKeyState(VK_F8)&0x8000)
        || (uint32_t)(GetTickCount()-control->stamp)>=500
        || memcmp(raw,(void*)(uintptr_t)(player+0x94),12))return;
    memcpy((void*)(uintptr_t)(player+0x98),&runtime_physics_y,4);
    runtime_actor_y=runtime_physics_y;runtime_actor_active=1;runtime_vertical_writes++;
}
