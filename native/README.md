# Native RE4 movement and optional look, 0.4.3

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
80 bytes, first ten words retaining the old layout: magic `0x52453443`, version 2,
seqlock sequence, GetTickCount stamp, armed, buttons, latched stop, hook status,
frame counter, last applied buttons. Only a single host should write it.
Words 10..19: float cumulative yaw degrees, pitch, signed eye height, flags,
arm ID, capabilities, applied yaw, camera ticks, look samples and camera-active.
The verified player-slot signature locates cCoord ang_A0.y; mouse writes only
its yaw, validates object/vtable/ranges and bounds finite deltas per sequence.
Optional CameraQuasiFPS::hitCheck and CamCtrl signatures come from Trainer.cpp
and Game.cpp of the pinned public reference. A fastcall wrapper forwards the
original, then overrides the argument matrix and eye point for that instance.
0.4.3 uses identity axes with world translation feet + eye height and a normalized
world yaw/pitch sight line, removing a rotating orbit offset. Argument matrices
may be stack-local: never retain them. Only the static player matrix at
CamCtrl+0x278+0x178 is backed up and restored before the next native update.
The validated player's cUnit be_flag_4 drawing bits Trans (0x2) and foot shadow
(0x10) are temporarily cleared; Alive/Move and collision flags remain unchanged.
Restore only those bits, preserving other changes, and only while PlayerPtr and
vtable still identify the same object; never write a departed/freed player.
Restoration runs before PadRead and camera calls, including stop/stale input.
This rendering interpretation needs Windows validation; weapons/child effects
may still draw. Mouse yaw is user-confirmed; first-person anchoring/hiding is not.
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

## 0.4.3 idle smoothing and weapon rendering

Pinned Trainer.cpp disables lr_rate smoothing by changing the ZF result of
CMP byte [EBX+0x1B6],0. New unique E8 ???? 80 BB B6 01 00 00 00 signature
requires that exact seven-byte CMP. A naked i386 hook replays CMP, preserves
GPRs/flags, uses the portable ZF policy only while camera_active/armed/no stop
and EBX equals our QuasiFPS, then resumes after CMP. Helper uses integer reads
only, with no external calls or x87/SSE changes. No smoothing hook means FP
capability remains off. All camera/yaw signatures and protections still apply.
Player Wep_7D8 -> cPlWep slots0x34/0x38 identify weapon cObjWep models. Hide
only validated in-module-vtable objects' drawing bits, deduplicate identical
slots, and restore only the same player/controller/slot/object/vtable chain.
Restore happens before native input/camera logic; never dereference old objects
when ownership changes. Inventory and weapon behavior remain native. The user
reports idle camera drift and floating weapon in0.4.2; these corrections remain
Windows-unverified.
