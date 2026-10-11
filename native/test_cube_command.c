#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "cube_policy.h"
static CubeCommand shared,*cube_command=&shared;
static unsigned mode,barriers;
static void MemoryBarrier(void) {
    barriers++;
    if (mode==1 && barriers==1) {shared.sequence=3;shared.epoch=99;}
    if (mode==1 && barriers==2) {shared.stamp=41;shared.epoch=9;shared.sequence=4;}
    if (mode==2) shared.sequence|=1;
}
/* ACTUAL_READER */
int main(void) {
    shared=(CubeCommand){.magic=0x52453454,.version=4,.sequence=2,.enabled=1,.epoch=7,.stamp=40};
    CubeCommand out={0};CubeCommand before=shared;
    assert(cube_command_read(&out) && out.epoch==7 && !memcmp(&shared,&before,sizeof(shared)));
    mode=1;barriers=0;assert(cube_command_read(&out) && out.epoch==9 && out.stamp==41 && out.sequence==4 && barriers==4);
    mode=2;barriers=0;assert(!cube_command_read(&out) && barriers==6);
    mode=0;shared.sequence=4;shared.reserved=1;assert(!cube_command_read(&out));shared.reserved=0;
    shared.magic=0;assert(!cube_command_read(&out));shared.magic=0x52453454;shared.version=1;assert(!cube_command_read(&out));
    shared.version=2;assert(!cube_command_read(&out));shared.version=3;assert(!cube_command_read(&out));shared.version=4;shared.mode=3;assert(!cube_command_read(&out));
    shared.mode=1;shared.bounds[0]=123;assert(cube_command_read(&out) && out.mode==1 && out.bounds[0]==123);
    shared.mode=2;assert(!cube_command_read(&out));shared.count=2;
    shared.origin[0]=999;shared.boxes[1][5]=1000;
    assert(cube_command_read(&out) && out.count==2 && out.origin[0]==999 && out.boxes[1][5]==1000);
    shared.count=CUBE_MAX_BOXES+1;assert(!cube_command_read(&out));shared.count=2;
    shared.style=2;assert(!cube_command_read(&out));shared.style=0;
    shared.style=1;assert(cube_command_read(&out) && out.style==1);
    shared.mode=1;assert(!cube_command_read(&out));
    cube_command=NULL;assert(!cube_command_read(&out));
    puts("Cube command reader: readonly complete snapshot, torn retry, odd/malformed rejection and no cached activation passed");
}
