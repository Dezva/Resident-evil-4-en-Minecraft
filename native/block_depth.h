#ifndef RE4_BLOCK_DEPTH_H
#define RE4_BLOCK_DEPTH_H
#include <math.h>
#include <stdint.h>
#include <string.h>
typedef struct {float matrix[16],a,b,near_plane,far_plane;unsigned valid,reverse;} BlockProjection;
static inline int block_depth_world_compare(unsigned compare){return compare==2 || compare==4 || compare==5 || compare==7;}
static inline int block_projection(const float *m,float fov,float width,float height,BlockProjection *out){
    *out=(BlockProjection){0};for(unsigned i=0;i<16;i++)if(!isfinite(m[i]) || fabsf(m[i])>100000000.f)return 0;
    if(!isfinite(fov+width+height) || fov<5 || fov>170 || width<1 || height<1 || fabsf(m[11])<.99f || fabsf(m[11])>1.01f || fabsf(m[15])>.001f)return 0;
    const unsigned zeros[]={1,2,3,4,6,7,8,9,12,13};for(unsigned i=0;i<10;i++)if(fabsf(m[zeros[i]])>.001f)return 0;
    float focal=1.f/tanf(fov*.00872664625997165f);
    /* CAMERA may retain a virtual 4:3 frustum in a widescreen viewport.
     * Validate its own aspect, and use its actual X focal for projection. */
    if(fabsf(m[5]-focal)>focal*.02f || m[0]<=0 || m[5]/m[0]<.5f || m[5]/m[0]>4.f)return 0;
    float a=m[10]*(m[11]>0?1.f:-1.f),b=m[14];
    if(fabsf(a)<1e-7f || fabsf(a-1.f)<1e-7f)return 0;
    float zero=-b/a,one=-b/(a-1.f);if(zero<=0 || one<=0 || !isfinite(zero+one))return 0;
    float near_distance=fminf(zero,one),far_distance=fmaxf(zero,one);if(near_distance<.001f || far_distance<near_distance*1.01f || far_distance>100000000.f)return 0;
    for(unsigned i=0;i<16;i++){out->matrix[i]=m[i];}out->a=a;out->b=b;out->near_plane=near_distance;out->far_plane=far_distance;out->reverse=one<zero;out->valid=1;return 1;
}
/* SDK CAMERA.ProjMat_64 is GX column-vector clip Z in [-W,0].
 * Convert to D3D [0,W] using the observed opaque pass's depth comparison. */
static inline int block_gx_projection(const float *gx,float fov,float width,float height,unsigned compare,BlockProjection *out){
    if(!block_depth_world_compare(compare))return 0;
    float d3d[16];for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++)d3d[r*4+c]=gx[c*4+r];
    if(fabsf(gx[14]+1.f)>.001f || gx[10]>=0 || gx[11]>=0)return 0;
    unsigned reverse=compare==5 || compare==7;
    d3d[10]=reverse?-gx[10]:gx[10]-1.f;d3d[14]=reverse?-gx[11]:gx[11];
    if(!block_projection(d3d,fov,width,height,out))return 0;
    /* UHD's shader corrects the virtual GX aspect for the output viewport.
     * Native Z planes stay unchanged; equal pixel focal lengths avoid anamorphic cubes. */
    out->matrix[0]=out->matrix[5]*height/width;
    return 1;
}
/* Only accept a complete perspective matrix in actual GPU constants.
 * GX-only matrices fail the D3D near/far checks rather than masquerading as Z. */
static inline int block_shader_projection(const float *constants,unsigned registers,float fov,float width,float height,unsigned compare,BlockProjection *out){
 if(!block_depth_world_compare(compare))return 0;
 for(unsigned start=0;start+4<=registers;start++)for(unsigned transpose=0;transpose<2;transpose++){
  float matrix[16];for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++)matrix[r*4+c]=constants[start*4+(transpose?c*4+r:r*4+c)];
  BlockProjection candidate;
  if(block_projection(matrix,fov,width,height,&candidate) && candidate.reverse==(compare==5 || compare==7)){
   *out=candidate;return 1;
  }
 }
 return 0;
}
/* UHD uploads a combined affine model/view/projection at c0..c3, not an
 * isolated projection. With row-vector input, Z.xyz = A * W.xyz and
 * Z.w = A * W.w + B. Recover A/B without guessing model position or scale. */
static inline int block_wvp_projection(const float *wvp,const float *gx,float fov,float width,float height,unsigned compare,BlockProjection *out){
 BlockProjection shape;if(!block_gx_projection(gx,fov,width,height,compare,&shape))return 0;
 double ww=0,zw=0;for(unsigned i=0;i<16;i++)if(!isfinite(wvp[i]) || fabsf(wvp[i])>100000000.f)return 0;
 for(unsigned i=0;i<3;i++){double w=wvp[i*4+3];ww+=w*w;zw+=w*wvp[i*4+2];}
 if(ww<1e-12 || ww>1e12)return 0;
 double a=zw/ww,b=wvp[14]-a*wvp[15],error=0;
 for(unsigned i=0;i<3;i++){double e=wvp[i*4+2]-a*wvp[i*4+3];error+=e*e;}
 if(error>ww*1e-10 || !isfinite(a+b) || fabs(b)<.001 || fabs(b)>10000000)return 0;
 float matrix[16];memcpy(matrix,shape.matrix,64);matrix[10]=-(float)a;matrix[14]=(float)b;
 BlockProjection candidate;if(!block_projection(matrix,fov,width,height,&candidate) || candidate.reverse!=(compare==5 || compare==7))return 0;
 *out=candidate;return 1;
}
/* Clear accepts either no rectangles or one absolute rectangle covering the viewport. */
static inline int block_depth_full_clear(unsigned count,const int32_t *rect,uint32_t x,uint32_t y,uint32_t width,uint32_t height){
    if(!count)return 1;
    return count==1 && rect && width && height && rect[0]<=(int64_t)x && rect[1]<=(int64_t)y && rect[2]>=(int64_t)x+width && rect[3]>=(int64_t)y+height;
}
static inline float block_depth(const BlockProjection *p,float reciprocal){return p->a+p->b*reciprocal;}
#endif
