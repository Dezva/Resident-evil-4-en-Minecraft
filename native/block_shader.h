#ifndef RE4_BLOCK_SHADER_H
#define RE4_BLOCK_SHADER_H
#include <stdint.h>
/* Recognize only a straight-line VS3 position computed from v0 * c0..c3.
 * These are D3D9 token fields, not copied game shader bytecode. Unknown flow,
 * relative addressing, partial writes or later position edits are rejected. */
static inline unsigned block_shader_reg(uint32_t p){return ((p>>28)&7u)|((p>>8)&24u);}
static inline int block_shader_src(uint32_t p,unsigned kind,unsigned index,unsigned swizzle){return block_shader_reg(p)==kind && (p&2047u)==index && !(p&0x0f002000u) && ((p>>16)&255u)==swizzle;}
static inline int block_shader_dst(uint32_t p,unsigned kind,unsigned index){return block_shader_reg(p)==kind && (p&2047u)==index && !(p&0x0ff02000u) && ((p>>16)&15u)==15;}
static inline int block_shader_wvp(const uint32_t *code,unsigned words){
 if(words<2 || words>4096 || code[0]!=0xfffe0300u)return 0;
 unsigned output=~0u,input=0,stage=0,temp=~0u,valid=0;
 for(unsigned at=1;at<words;){
  uint32_t token=code[at];unsigned op=token&65535u,n=(token>>24)&15u;
  if(op==65535)return valid && input && at==words-1;
  if(op==65534){n=(token>>16)&32767u;if(n>words-at-1)return 0;at+=n+1;continue;}
  if(!n || n>words-at-1 || (token&0x10000000u))return 0;
  const uint32_t *p=code+at+1;
  if(op!=81)for(unsigned i=0;i<n;i++)if(!(p[i]&0x80000000u))return 0;
  if(op==31){
   if(n!=2)return 0;
   if((p[0]&0x000f000fu)==0){
    if(block_shader_reg(p[1])==6)output=p[1]&2047u;
    if(block_shader_reg(p[1])==1 && (p[1]&2047u)==0)input=1;
   }
   at+=n+1;continue;
  }
  // Permit only arithmetic, definitions and declarations; no loops/calls/branches.
  if(!((op>=1 && op<=24) || (op>=32 && op<=37) || op==46 || op==78 || op==79 || op==81 || op==88 || op==90))return 0;
  if(op==81){if(block_shader_reg(p[0])==2 && (p[0]&2047u)<4)return 0;at+=n+1;continue;}
  unsigned destination=p[0]&2047u,kind=block_shader_reg(p[0]);
  if(kind==6 && destination==output)valid=0;
  if(stage>=1 && stage<=3 && op==4 && n==4 &&
     block_shader_src(p[1],2,stage==1?0:stage==2?2:3,228) &&
     block_shader_src(p[2],1,0,stage==1?0:stage==2?170:255) &&
     block_shader_src(p[3],0,temp,228) &&
     (block_shader_dst(p[0],0,temp) || (stage==3 && block_shader_dst(p[0],6,output)))){
   if(kind==6){valid=1;stage=0;}else stage++;
  }else if(stage==4 && op==1 && n==2 && block_shader_dst(p[0],6,output) && block_shader_src(p[1],0,temp,228)){
   valid=1;stage=0;
  }else{
   if(stage<4 || (kind==0 && destination==temp))stage=0;
   if(op==5 && n==3 && kind==0 && block_shader_dst(p[0],0,destination) && block_shader_src(p[1],2,1,228) && block_shader_src(p[2],1,0,85)){
    stage=1;temp=destination;
   }
  }
  at+=n+1;
 }
 return 0;
}
#endif
