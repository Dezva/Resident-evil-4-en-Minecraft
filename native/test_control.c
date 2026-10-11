#include "control_logic.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    assert(re4_buttons(2,2,100,50,1,0,1)==1);
    assert(re4_buttons(2,2,550,50,1,0,1)==0);
    assert(re4_buttons(2,2,100,50,0,0,1)==0);
    assert(re4_buttons(2,2,100,50,1,1,1)==0);
    assert(re4_buttons(1,1,100,50,1,0,1)==0);
    assert(re4_buttons(2,4,100,50,1,0,1)==0);
    assert(re4_buttons(2,2,100,50,1,0,15)==0);
    assert(re4_buttons(2,2,100,50,1,0,128)==0);
    assert(re4_buttons(2,2,100,50,1,0,65)==65);
    assert(re4_buttons(2,2,100,50,1,0,69)==69);
    assert(re4_relative_input(69)==65); /* Forward-right + sprint keeps KEY_RUN. */
    assert(re4_relative_input(72)==65); /* FP lateral motion is native forward. */
    assert(re4_buttons(2,2,100,50,1,0,64)==0);
    assert(re4_buttons(2,2,100,50,1,0,67)==0); /* W+S cancels sprint too. */
    assert(re4_buttons(2,2,100,50,1,0,76)==0); /* A+D cancels sprint too. */
    assert(re4_buttons(2,2,100,50,1,0,77)==65); /* Cancel A+D, keep W+run. */
    assert(re4_buttons(2,2,550,50,1,0,65)==0);
    assert(re4_buttons(2,2,100,50,0,0,65)==0);
    assert(re4_buttons(2,2,100,50,1,1,65)==0);
    assert(re4_buttons(2,2,100,50,1,0,16)==0); /* Aim remains forbidden. */
    assert(re4_buttons(2,2,100,50,1,0,32)==0); /* Reload remains forbidden. */
    assert(re4_relative_input(64)==0);
    assert(re4_buttons(2,2,10,UINT32_MAX-20,1,0,5)==5);
    assert(re4_cached_frame_usable(1,100,50,1,1,0));
    assert(!re4_cached_frame_usable(0,100,50,1,1,0));
    assert(!re4_cached_frame_usable(1,100,50,1,0,0));
    assert(!re4_cached_frame_usable(1,100,50,1,1,1));
    assert(!re4_cached_frame_usable(1,100,50,0,1,0));
    assert(re4_cached_frame_usable(1,10,UINT32_MAX-20,1,1,0));
    /* A writer left on an odd sequence must not renew a lease indefinitely. */
    for (unsigned now=50;now<550;++now) assert(re4_cached_frame_usable(1,now,50,1,1,0));
    assert(!re4_cached_frame_usable(1,550,50,1,1,0));
    puts("Native control policy: TTL, wrap, torn writes, disarm, stop and key masks passed");
}
