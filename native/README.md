# Native RE4 movement input, 0.3.4

Independent Win32/i386 C module. Proxy `dinput8.dll` loads the system DLL by its
absolute system directory path. Its DirectInput8Create forwards the original
call, then initializes a signature-validated hook outside DllMain on the caller's
thread. DllMain does not load other libraries or start threads. This prototype
refuses installation alongside an existing dinput8.dll; it does not chain mods.

The public re4_tweaks reference at revision
`92c0208bd09c29c9640e13a2493b8e6bd6edd700` identifies a main-loop CALL and a nested
CALL at the destination to `PadRead` in KeyboardMouseTweaks.cpp, the Key_btn_on
operand in Game.cpp and forward/back/right/left flags in SDK/pad.h. Our code
independently resolves these patterns only in executable PE32/i386 sections and
requires one match each, in-module operands/targets and title version `1.1.0`.
It replaces only the entry branch's displacement (E8 CALL or E9 tail JMP), preserves its opcode, calls
the original void cdecl function first, then replaces only four movement bits.
It does not include the reference's source code, loader or binary.

All writes to native input occur on the game's input thread after PadRead. The
game retains responsibility for locomotion, collisions, speed and movement
restrictions in its routines. No player coordinates, save files or game EXE are
patched. Runtime hook/DirectInput loading and actual collisions are still pending
validation in the user's Windows installation; compilation is not proof of those.
The user has confirmed DLL loading and IPC. A read-only audit found an E9 entry
at RVA 0x7509 targeting 0x567360; 0.3.4 resolves it dynamically and accepts its
branch type. A portable regression uses those exact observed bytes/RVAs.

IPC: local Windows mapping `Local\RE4Bridge-Control-<pid>`, default process DACL,
40 bytes (ten little-endian uint32 words): magic `0x52453443`, version 1,
seqlock sequence, GetTickCount stamp, armed, buttons, latched stop, hook status,
frame counter, last applied buttons. Only a single host should write it.
Odd/changed sequence, nonmovement bits, opposite directions or expired input
produce zero remote movement. TTL is 500 ms, with uint32 tick wrap handled.
Stop 1 is F8 and stop 2 is expired armed input; either requires a new explicit
arm generation in the authenticated TCP session to clear it. Hook status 1 means
ready, 2 means unsupported signatures/image and 3 means patch/protection error.
Frame counter is needed to verify the hook actually runs. There is no native
network listener. The Python host only maps IPC and reads RE4, not WriteProcessMemory.

Build from the repository:

```bash
bash tools/setup_native_toolchain.sh
bash tools/build_native.sh
```

Cloud toolchain: verified Debian repository packages, GCC MinGW 14.2.0 x86
POSIX, headers 12.0.0, binutils 2.44. They are extracted under
`/workspace/tools/mingw32`; no system installation is required. For another
Linux/Windows MinGW installation, set RE4_MINGW_CC to its i686 gcc executable.
Only Windows system KERNEL32, USER32 and msvcrt imports; no extra MinGW runtime
DLL is distributed. Timestamp is disabled for consistent binary rebuilds.

Portable native tests execute the same TTL/mask policy header on Linux, checking
stale/torn writes, opposing inputs, disarm, F8 state and wrap. They do not load
the Windows DLL. Windows installation is a user-operated script with verified
save backup; removal checks the package's own DLL hash and preserves backups.
