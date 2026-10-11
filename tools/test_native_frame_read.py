"""Run the actual native seqlock reader with controlled publication interleavings."""
from pathlib import Path
import subprocess
import tempfile
import unittest


class NativeFrameTests(unittest.TestCase):
    def test_partial_publication_cache_disarm_stop_and_expiry(self):
        root = Path(__file__).resolve().parents[1]
        source = (root/'native/re4_control.c').read_text()
        start = source.index('static int read_control_frame(')
        opening = source.index('{',start)
        depth = 1
        end = opening+1
        while depth:
            if source[end]=='{': depth+=1
            elif source[end]=='}': depth-=1
            end+=1
        reader = source[start:end]
        harness = r'''
#include <assert.h>
#include <stdint.h>
#include "control_logic.h"
typedef int32_t LONG;
typedef struct {
    volatile LONG sequence;
    volatile uint32_t stamp,armed,buttons,look_flags,arm_id;
    volatile float look_total,pitch,eye_height;
    volatile LONG stop;
} Shared;
typedef struct {
    uint32_t sequence,stamp,armed,buttons,flags,generation;
    float total,pitch,eye;
} ControlFrame;
static Shared memory, *state=&memory;
static ControlFrame cached_frame;
static int have_cached_frame,barriers,inject;
static uint32_t clock_now;
static uint32_t GetTickCount(void) { return clock_now; }
static void MemoryBarrier(void) {
    ++barriers;
    /* Change publication after fields were copied: reject mixed version. */
    if (inject && barriers==2) {
        state->sequence=4; state->buttons=8; state->look_total=30;
    }
}
'''+reader+r'''
int main(void) {
    ControlFrame out;
    clock_now=100; state->sequence=1; state->armed=1;
    assert(!read_control_frame(&out));
    state->sequence=2; state->stamp=50; state->buttons=4;
    state->look_flags=3; state->arm_id=7; state->look_total=20;
    state->eye_height=1600;
    assert(read_control_frame(&out) && out.sequence==2 && out.total==20);
    state->sequence=3; state->look_total=999; state->buttons=2;
    assert(read_control_frame(&out) && out.sequence==2 && out.total==20 && out.buttons==4);
    assert(cached_frame.stamp==50); /* No lease extension while writer is odd. */
    state->armed=0; assert(!read_control_frame(&out));
    state->armed=1; state->stop=1; assert(!read_control_frame(&out));
    state->stop=0; clock_now=550; assert(!read_control_frame(&out));
    clock_now=100; state->sequence=2; state->look_total=20;
    inject=1; barriers=0;
    assert(read_control_frame(&out) && out.sequence==4 && out.total==30 && out.buttons==8);
    assert(cached_frame.sequence==4); /* Torn first attempt never became cache. */
    inject=0; state->stamp=UINT32_MAX-20; state->sequence=6;
    clock_now=10; assert(read_control_frame(&out));
    state->sequence=7; assert(read_control_frame(&out));
    clock_now=600; assert(!read_control_frame(&out));
    return 0;
}
'''
        with tempfile.TemporaryDirectory(prefix='re4-frame-test-') as temporary:
            path = Path(temporary)
            (path/'test.c').write_text(harness)
            subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-I',str(root/'native'),
                            str(path/'test.c'),'-o',str(path/'test')],check=True,capture_output=True)
            subprocess.run([str(path/'test')],check=True,capture_output=True)


if __name__=='__main__': unittest.main()
