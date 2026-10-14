#include <assert.h>
#include <stdio.h>
#include "runtime_discovery.h"
int main(void){
    unsigned char a[18]={0},b[18]={0};unsigned char *sites[]={a,b};uint32_t operand=0x8000,targets[64];
    memcpy(a+13,&operand,4);memcpy(b+13,&operand,4);
    assert(re4_distinct_operands(sites,2,13,0x1000,0x10000,32,targets,64)==1 && targets[0]==operand);
    operand+=4;memcpy(b+13,&operand,4);
    assert(re4_distinct_operands(sites,2,13,0x1000,0x10000,32,targets,64)==2);
    assert(!re4_distinct_operands(sites,2,13,0x1000,0x10000,32,targets,1));
    operand=0x8001;memcpy(a+13,&operand,4);assert(!re4_distinct_operands(sites,1,13,0x1000,0x10000,32,targets,64));
    operand=0x10ff0;memcpy(a+13,&operand,4);assert(!re4_distinct_operands(sites,1,13,0x1000,0x10000,32,targets,64));
    assert(!re4_distinct_operands(sites,0,13,0x1000,0x10000,32,targets,64));assert(!re4_distinct_operands(sites,65,13,0x1000,0x10000,32,targets,64));
    uint32_t headers[3][4]={{0x1234,0x100000,60,3592},{0x1234,0x200000,100,468},{0x1234,0x300000,380,988}};
    assert(re4_cem_pool_header_valid(headers[0],0x1000,0x10000));
    assert(!re4_cem_pool_header_valid(headers[1],0x1000,0x10000));assert(!re4_cem_pool_header_valid(headers[2],0x1000,0x10000));
    headers[0][2]=0;assert(!re4_cem_pool_header_valid(headers[0],0x1000,0x10000));
    headers[0][2]=4097;assert(!re4_cem_pool_header_valid(headers[0],0x1000,0x10000));
    puts("Runtime discovery: distinct SDK operands collected; observed60/3592 cEm pool distinguished from100/468 and380/988 pools");
}
