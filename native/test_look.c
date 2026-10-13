#include "look_logic.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    float consumed = 0;
    assert(fabsf(re4_take_yaw(10, &consumed) + 0.174532925f) < 0.00001f);
    assert(re4_take_yaw(10, &consumed) == 0); /* Same total must not rotate twice. */
    re4_take_yaw(90, &consumed); assert(consumed == 40);
    re4_take_yaw(90, &consumed); re4_take_yaw(90, &consumed); assert(consumed == 90);
    assert(re4_take_yaw(NAN, &consumed) == 0 && consumed == 90);
    consumed=0;assert(fabsf(re4_take_view_yaw(120,&consumed)+2.0943951f)<.00001f && consumed==120);
    assert(re4_take_view_yaw(120,&consumed)==0);
    assert(re4_take_view_yaw(NAN,&consumed)==0 && consumed==120);
    assert(re4_valid_look(100, 85, 160, 3));
    assert(re4_valid_look(100,85,160,11)); // GUI hold keeps first-person camera valid
    assert(!re4_valid_look(100,85,160,19));
    assert(!re4_valid_look(100, 86, 160, 3));
    assert(!re4_valid_look(100, 0, 160, 2));
    assert(!re4_valid_look(INFINITY, 0, 160, 1));
    assert(!re4_valid_look(0, 0, 0, 1));
    assert(re4_valid_look(0, 0, 1600, 3));
    assert(re4_valid_look(0, 0, -2000, 3));
    assert(!re4_valid_look(0, 0, 2501, 3));
    assert(!re4_valid_look(0, 0, -2501, 3));
    float pos[3], target[3], frame[12];
    float feet[3] = {-104567, -230, -2622};
    assert(re4_eye_frame(feet,0,0,1600,frame,pos,target));
    assert(frame[3] == feet[0] && frame[7] == 1370 && frame[11] == feet[2]);
    assert(pos[0] == 0 && pos[1] == 0 && pos[2] == 0);
    assert(target[0] == 0 && target[1] == 0 && target[2] == 1000);
    /* Turning changes only gaze, never the eye's world position. */
    assert(re4_eye_frame(feet,1.570796327f,0,1600,frame,pos,target));
    assert(target[0] > 999.f && fabsf(target[2]) < 0.001f);
    assert(frame[3] == feet[0] && frame[7] == 1370 && frame[11] == feet[2]);
    for (int sign = -1; sign <= 1; sign += 2) {
        assert(re4_eye_frame(feet,2,sign*85,2500,frame,pos,target));
        float length = target[0]*target[0] + target[1]*target[1] + target[2]*target[2];
        assert(fabsf(length-1000000.f) < 1.f && target[1]*sign < 0);
    }
    /* Reproduce the uploaded trace: large world X, yaw -2.297626, pitch 54.
       Compare the gaze after single-precision world translation. */
    assert(re4_eye_frame(feet,-2.297626f,54.f,1600,frame,pos,target));
    float world_x = frame[3] + target[0], world_z = frame[11] + target[2];
    float observed = atan2f(world_x-frame[3],world_z-frame[11]);
    assert(fabsf(observed-(-2.297626f)) < 0.00002f);
    assert(!re4_eye_frame(feet,NAN,0,1600,frame,pos,target));
    feet[1] = INFINITY;
    assert(!re4_eye_frame(feet,0,0,1600,frame,pos,target));
    assert(re4_smoothing_flags(0xFFFFFFFFu,0) == 0xFFFFFFFFu);
    assert(re4_smoothing_flags(0xFFFFFFFFu,1) == 0xFFFFFFBFu);
    assert(re4_smoothing_flags(0x213,1) == 0x213);
    assert(re4_distinct_model(0x10000,0));
    assert(!re4_distinct_model(0x10000,0x10000)); /* Two slots can share one model. */
    assert(!re4_distinct_model(0,0));
    assert(re4_model_slot_matches(0x10000,0x10000,0x20000,0x20000));
    assert(!re4_model_slot_matches(0x10000,0,0x20000,0x20000));
    assert(!re4_model_slot_matches(0x10000,0x30000,0x20000,0x20000));
    assert(!re4_model_slot_matches(0x10000,0x10000,0x20000,0x40000));
    unsigned saved = 0xA531;
    unsigned hidden = re4_hide_model(saved);
    assert((hidden & RE4_MODEL_DRAW_BITS) == 0 && (hidden & 0x21) == (saved & 0x21));
    assert(re4_restore_model(hidden | 0x80000000u,saved) == (saved | 0x80000000u));
    assert(re4_restore_model(re4_hide_model(0x21),0x21) == 0x21);
    assert(fabsf(re4_wrap_yaw(7)) <= 3.141593f);
    assert(fabsf(re4_walk_heading(0,1)) < 0.00001f);
    assert(fabsf(re4_walk_heading(0,4)+1.570796327f) < 0.00001f);
    assert(fabsf(re4_walk_heading(0,8)-1.570796327f) < 0.00001f);
    assert(fabsf(fabsf(re4_walk_heading(0,2))-3.141592654f) < 0.00001f);
    assert(fabsf(re4_walk_heading(0,5)+0.785398163f) < 0.00001f);
    assert(fabsf(re4_walk_heading(0,9)-0.785398163f) < 0.00001f);
    /* Walk direction never alters gaze; all diagonals use one native forward
       input rather than adding independent world velocities. */
    float camera[12], view[12], right[3], up[3], back[3];
    float eye[3] = {-91878.88f,1568.57f,-4360.27f};
    re4_view_frame(eye,1.841018f,4.349857f,camera,view,right,up,back);
    for (int row=0;row<3;++row) {
        float projected_eye=view[4*row]*eye[0]+view[4*row+1]*eye[1]+view[4*row+2]*eye[2]+view[4*row+3];
        assert(fabsf(projected_eye) < 0.02f);
        for (int col=0;col<3;++col) {
            float product=0;
            for (int k=0;k<3;++k) product+=view[4*row+k]*camera[4*k+col];
            assert(fabsf(product-(row==col)) < 0.00001f);
        }
    }
    assert(fabsf(re4_wrap_yaw(atan2f(-back[0],-back[2])-1.841018f)) < 0.00001f);
    puts("Native look policy: cumulative yaw, bounds, pitch and eye-point math passed");
}
