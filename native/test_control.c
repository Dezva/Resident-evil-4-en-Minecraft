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
    assert(re4_buttons(2,2,10,UINT32_MAX-20,1,0,5)==5);
    puts("Native control policy: TTL, wrap, torn writes, disarm, stop and key masks passed");
}
