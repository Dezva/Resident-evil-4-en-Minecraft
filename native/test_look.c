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
    float pos[3], target[3];
    re4_eye_point(0, 160, pos, target);
    assert(pos[1] == 160 && target[1] == 160 && target[2] == 1);
    re4_eye_point(45, 160, pos, target);
    assert(target[1] < pos[1] && target[2] > 0);
    assert(fabsf(re4_wrap_yaw(7)) <= 3.141593f);
    puts("Native look policy: cumulative yaw, bounds, pitch and eye-point math passed");
}
