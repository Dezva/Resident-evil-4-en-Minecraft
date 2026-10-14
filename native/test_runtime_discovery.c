#include <assert.h>
#include <stdio.h>
#include "runtime_discovery.h"
int main(void){
    unsigned char a[18]={0},b[18]={0};unsigned char *sites[]={a,b};uint32_t operand=0x8000;
    memcpy(a+13,&operand,4);memcpy(b+13,&operand,4);
    assert(re4_shared_operand(sites,1,13,0x1000,0x10000,32)==operand);
    assert(re4_shared_operand(sites,2,13,0x1000,0x10000,32)==operand); // repeated SDK instruction, one manager
    operand+=4;memcpy(b+13,&operand,4);assert(!re4_shared_operand(sites,2,13,0x1000,0x10000,32));
    operand=0x8001;memcpy(a+13,&operand,4);assert(!re4_shared_operand(sites,1,13,0x1000,0x10000,32));
    operand=0x10ff0;memcpy(a+13,&operand,4);assert(!re4_shared_operand(sites,1,13,0x1000,0x10000,32));
    assert(!re4_shared_operand(sites,0,13,0x1000,0x10000,32));assert(!re4_shared_operand(sites,65,13,0x1000,0x10000,32));
    puts("Runtime discovery: repeated shared operand accepted; ambiguous, unaligned, out of range and overflowing sites rejected");
}
