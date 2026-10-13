#ifndef RE4_BLOCK_GEOMETRY_H
#define RE4_BLOCK_GEOMETRY_H
#include <stdint.h>
#include <math.h>
typedef struct { float x,y,z,rhw; uint32_t color; } BlockVertex;
_Static_assert(sizeof(BlockVertex)==20,"D3D9 screen vertex stride");
static inline int block_valid_bounds(const float *b) {
    for (unsigned i=0;i<6;++i) if (!isfinite(b[i]) || fabsf(b[i])>20000000.f) return 0;
    for (unsigned i=0;i<3;++i) if (b[i+3]-b[i]<40.f || b[i+3]-b[i]>2500.f) return 0;
    return 1;
}
static inline int block_valid_shape_bounds(const float *b) {
    for (unsigned i=0;i<6;++i) if (!isfinite(b[i]) || fabsf(b[i])>20000000.f) return 0;
    for (unsigned i=0;i<3;++i) if (b[i+3]-b[i]<1.f || b[i+3]-b[i]>7500.f) return 0;
    return 1;
}
/* Compose CAMERA inverse with the grid's rigid local-to-world rotation. */
static inline int block_scene_view(const float *camera,const float *origin,float *view) {
    for (unsigned i=0;i<4;++i) if (!isfinite(origin[i]) || fabsf(origin[i])>(i==3?3.141593f:20000000.f)) return 0;
    float c=cosf(origin[3]),s=sinf(origin[3]);
    for (unsigned r=0;r<3;++r) {
        view[4*r]=camera[4*r]*c+camera[4*r+2]*s;
        view[4*r+1]=camera[4*r+1];
        view[4*r+2]=-camera[4*r]*s+camera[4*r+2]*c;
        view[4*r+3]=camera[4*r+3];
        for (unsigned axis=0;axis<3;++axis) view[4*r+3]+=camera[4*r+axis]*origin[axis];
    }
    return 1;
}
/* Clip in screen coordinates, then interpolate endpoints. No giant offscreen vertices. */
static inline int block_clip_line(float *a,float *b,float width,float height) {
    float dx=b[0]-a[0],dy=b[1]-a[1],lo=0,hi=1;
    float p[4]={-dx,dx,-dy,dy},q[4]={a[0],width-a[0],a[1],height-a[1]};
    for (unsigned i=0;i<4;++i) {
        if (fabsf(p[i])<1e-10f) { if (q[i]<0) return 0; }
        else { float t=q[i]/p[i]; if (p[i]<0) { if (t>hi) return 0; if (t>lo) lo=t; }
            else { if (t<lo) return 0; if (t<hi) hi=t; } }
    }
    b[0]=a[0]+dx*hi;b[1]=a[1]+dy*hi;a[0]+=dx*lo;a[1]+=dy*lo; return 1;
}
/* CAMERA.v_mat is a 3x4 rigid inverse; front is negative camera Z.
   Screen-space diagnostic lines have no depth/terrain occlusion. */
static inline int block_valid_view(const float *view,float fovy,float width,float height) {
    if (!isfinite(fovy) || fovy<5 || fovy>170
            || width<1 || height<1 || width>16384 || height>16384) return 0;
    for (unsigned i=0;i<12;++i) if (!isfinite(view[i]) || fabsf(view[i])>200000000.f) return 0;
    for (unsigned r=0;r<3;++r) {
        float norm=0;for (unsigned c=0;c<3;++c) norm+=view[4*r+c]*view[4*r+c];
        if (norm<0.9f || norm>1.1f) return 0;
        for (unsigned other=0;other<r;++other) {
            float dot=0;for (unsigned c=0;c<3;++c) dot+=view[4*r+c]*view[4*other+c];
            if (fabsf(dot)>0.1f) return 0;
        }
    }
    return 1;
}
static inline unsigned block_lines_projected(const float *bounds,const float *view,float fovy,
        float width,float height,BlockVertex *vertices,float focal_x) {
    if (!block_valid_shape_bounds(bounds) || !block_valid_view(view,fovy,width,height)) return 0;
    float points[8][3];
    for (unsigned i=0;i<8;++i) for (unsigned r=0;r<3;++r) {
        points[i][r]=view[4*r+3];
        for (unsigned c=0;c<3;++c) points[i][r]+=view[4*r+c]*bounds[c+((i&(1u<<c))?3:0)];
    }
    float focal=height/(2*tanf(fovy*0.00872664625997165f));
    if(!isfinite(focal_x) || focal_x<0 || focal_x>100000000.f)return 0;
    if(focal_x==0)focal_x=focal;
    unsigned count=0;
    for (unsigned i=0;i<8;++i) for (unsigned axis=0;axis<3;++axis) {
        if (i&(1u<<axis)) continue;
        float a[3],b[3];
        for (unsigned r=0;r<3;++r) {a[r]=points[i][r];b[r]=points[i|(1u<<axis)][r];}
        if (a[2]>-1 && b[2]>-1) continue;
        if (a[2]>-1 || b[2]>-1) {
            float t=(-1-a[2])/(b[2]-a[2]); float *p=a[2]>-1?a:b;
            for (unsigned r=0;r<3;++r) p[r]=a[r]+t*(b[r]-a[r]);
        }
        float sa[2]={width/2+focal_x*a[0]/-a[2],height/2-focal*a[1]/-a[2]};
        float sb[2]={width/2+focal_x*b[0]/-b[2],height/2-focal*b[1]/-b[2]};
        if (!isfinite(sa[0]+sa[1]+sb[0]+sb[1]) || !block_clip_line(sa,sb,width,height)) continue;
        vertices[count++]=(BlockVertex){sa[0],sa[1],0,1,0xFF40FF40};
        vertices[count++]=(BlockVertex){sb[0],sb[1],0,1,0xFF40FF40};
    }
    return count;
}
static inline unsigned block_lines(const float *bounds,const float *view,float fovy,float width,float height,BlockVertex *vertices){return block_lines_projected(bounds,view,fovy,width,height,vertices,0);}
#endif
