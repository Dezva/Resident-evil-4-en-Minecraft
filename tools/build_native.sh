#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
native_cc=${RE4_MINGW_CC:-/workspace/tools/mingw32/usr/bin/i686-w64-mingw32-gcc-posix}
mkdir -p native/build
"$native_cc" -std=c11 -O2 -Wall -Wextra -Werror -Wno-cast-function-type -shared \
  native/re4_control.c -o native/build/dinput8.dll -Wl,--kill-at,--no-insert-timestamp -static-libgcc
gcc -std=c11 -Wall -Wextra -Werror native/test_control.c -o native/build/test-control
native/build/test-control
