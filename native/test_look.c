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
    assert(re4_valid_look(100, 85, 160, 3));
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
    assert(target[0] == 0 && target[1] == 0 && target[2] == 1);
    /* Turning changes only gaze, never the eye's world position. */
    assert(re4_eye_frame(feet,1.570796327f,0,1600,frame,pos,target));
    assert(target[0] > 0.999f && fabsf(target[2]) < 0.00001f);
    assert(frame[3] == feet[0] && frame[7] == 1370 && frame[11] == feet[2]);
    for (int sign = -1; sign <= 1; sign += 2) {
        assert(re4_eye_frame(feet,2,sign*85,2500,frame,pos,target));
        float length = target[0]*target[0] + target[1]*target[1] + target[2]*target[2];
        assert(fabsf(length-1) < 0.00001f && target[1]*sign < 0);
    }
    assert(!re4_eye_frame(feet,NAN,0,1600,frame,pos,target));
    feet[1] = INFINITY;
    assert(!re4_eye_frame(feet,0,0,1600,frame,pos,target));
    unsigned saved = 0xA531;
    unsigned hidden = re4_hide_model(saved);
    assert((hidden & RE4_MODEL_DRAW_BITS) == 0 && (hidden & 0x21) == (saved & 0x21));
    assert(re4_restore_model(hidden | 0x80000000u,saved) == (saved | 0x80000000u));
    assert(re4_restore_model(re4_hide_model(0x21),0x21) == 0x21);
    assert(fabsf(re4_wrap_yaw(7)) <= 3.141593f);
    puts("Native look policy: cumulative yaw, bounds, pitch and eye-point math passed");
}
