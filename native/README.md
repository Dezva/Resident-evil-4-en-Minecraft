# Native RE4 bridge

Build449 uses Runtime v4 (128 bytes): absolute feet Y drives the actual
player position and horizontal block collision; the camera interpolates absolute
endpoints separately. Native terrain supports landing without feeding actor Y
back into gravity as a new ground floor. Pad/camera/projection reapply only the
validated player's pos_94.y, with arm/generation/500ms/F8/scene/pointer/pose gates.
F8 leaves the current position to native RE4; it never restores an old ground Y.
Native report fields distinguish requested feet from applied actor height.
[0.4.49 guide](../docs/RE4-BLOCK-0.4.49.md). Windows gameplay remains unverified.
The following notes describe older builds and their earlier virtual-only Y.

Build448 separates validated physical jump lift from interpolated view lift and
uses a scaled support contact margin for player sweep/depenetration. See
[0.4.48](../docs/RE4-BLOCK-0.4.48.md). Older notes follow.

Build447 uses the observed E mask0x80400, selects the unique SDK cEm-compatible
pool among distinct manager operands, resolves connected-block penetration and
keeps debris motion timestamps across repeated lease packets. See
[0.4.47](../docs/RE4-BLOCK-0.4.47.md). Older notes follow.

Build446 adds bounded120ms contextual holds, shared-operand enemy discovery
and render-only debris prediction between20Hz packets. See
[0.4.46](../docs/RE4-BLOCK-0.4.46.md) for tested scope and Windows checks.
The notes below describe build445 and earlier versions.

Current build445 uses Runtimev3 (112bytes) for contextual check/pickup/confirm,
separate native event cancellation and enemy diagnostics. Pad owns input;
scripted projection can deliver a bounded50ms action pulse when pad is idle80ms.
Menu/F8/TTL/generation/death guards remain. Enemy pools up to4096slots/64MiB are
validated across64committed regions; at most128humanoids are corrected. Pad and
post-update projection reconcile X/Z, including already occupied placements.
No enemy Y/AI/HP/routine/animation writes and no combat/pathfinding are added.

Vertical view now interpolates between consecutive20Hz endpoints rather than
restarting an easing curve from each lagging pad sample. Native actor Y remains
unchanged; raw-Y discontinuity checks exclude virtual lift. Minecraft terrain
queries follow virtual height with unchanged local geometry budget. Unknown
ceiling coverage waits instead of disarming. Scene v6 retains its collision
prefix for render-only fragments. Gameplay CINESCO classification remains fixed.
See [0.4.45](../docs/RE4-BLOCK-0.4.45.md) and [IPC contract](../docs/CONTRACT.md).
Native fixtures are synthetic; gameplay/driver validation requires Windows.

## Historical creation facade and explicit cube test, 0.4.18

0.4.18 wraps the successful owned CreateDevice result in an external, typed
119-method device facade. Native object/vtable remain untouched. Initial owned
reference and later AddRef/Release are balanced independently of native resources.
All facade calls serialize through a per-device recursive critical section.
Begin/EndScene/Reset/Present track scene/loss.
Each delegated call also holds a temporary owned reference through bookkeeping,
protecting facade/native lifetime if a reentrant callback releases the caller's
last reference. The reference is released when the call ends.
Unknown interfaces delegate and disable rendering.
Resource GetDevice and GetDirect3D still return native identities; coverage and
identity comparisons with the facade remain a Windows compatibility limitation.

TEST-RE4-CUBE explicitly publishes a32-byte command, guarded by a single-writer
mutex, complete snapshot and500ms lease. Five-second countdown,30-second test,
F8/expiry/close stop; loss/Reset require a new explicit test and restore failure
requires restart. Separate92-byte read-only telemetry records drawing/errors.
The world-fixed100-unit green wire cube uses the validated Global CAMERA and
current player, projects/clips12edges and draws only to the backbuffer before
original EndScene. Temporary D3DSBT_ALL restores every changed state; no GPU
resource survives a frame. DllMain creates no wrappers/IPC/GPU resources and
the withdrawn discovery header remains excluded. [Windows test](../docs/RE4-CUBE-0.4.18.md).

Descriptions below document earlier versions; creation-only behavior applies
to0.4.15–0.4.17, not the explicitly enabled cube test in0.4.18.

0.4.17 compares the loaded D3D9 spelling with both exact paths derived from
GetSystemDirectoryW/GetSystemWow64DirectoryW. A 32-bit DLL loaded through the
System32 spelling can be redirected to SysWOW64 while GetModuleFileNameW retains
the spelling used at load. Local/proxy paths and subdirectories remain refused;
the unique named import must still match the loaded system export. Both attempts
record loaded/WOW64/system paths and the matched form, in IPCv3/3296bytes.
The reader retains v1/v2 compatibility. [Instructions](../docs/RE4-CAPTURE-0.4.17.md).

0.4.16 records the initial installation reason and performs at most one retry
before the first forwarded DirectInput8Create, only if the initial import was
unavailable/rejected. Identical PE32/system-export filters apply; a ready or
protection-failed hook is never retried. IPC v2 is168bytes with initial/latest
attempts; the reader also accepts v1 from0.4.15. No device recreation or search.
[Historical0.4.16 instructions](../docs/RE4-CAPTURE-0.4.16.md).

0.4.15 introduced the creation-only diagnostic.
DllMain now redirects one validated PE32 Direct3DCreate9 import; it does not call
COM, allocate a wrapper/mapping or load modules. The original import must exactly
match the export in the system D3D9 library. Missing/changed/ambiguous imports are
refused. Outside DllMain, the returned factory is wrapped with a typed17-method
IDirect3D9 facade; original device outputs are observed without dereferencing,
retaining, altering or wrapping them. Device addresses are historical snapshots.
No device scan or EndScene hook; block_render.h remains excluded with #error.
The lower historical descriptions document previous versions, not active graphics.

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
and Game.cpp of the pinned public reference. A fastcall wrapper calls the
original when FP is inactive or validation fails. Active FP replaces hitCheck
without executing the original collision/orbit adjustment first, matching the
reference's freecam control flow. It sets roll to zero and takes FOV from the
validated QFPS_OFFSET, defaulting to 50 when nonpositive.
0.4.4 uses identity axes with world translation feet + eye height and a
1000-unit world yaw/pitch sight line to retain float precision after translation.
Argument matrices
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

## 0.4.5 view-relative locomotion and final camera

Mouse maintains view_yaw independently of the animated player's yaw in FP.
WASD chooses a normalized horizontal heading relative to view_yaw; native
forward input drives locomotion/collision. No coordinate writes, collision
disable, velocity addition or diagonal speed boost. Non-FP retains tank input.
Disarm restores the current player's heading to view_yaw only while ownership
still matches; rearm anchors anew. Jump/sprint/crouch are not implemented.

The user's 0.4.4 trace shows Ctrl.Camera_60's final point changing by ~0.209
radians / 212 position units when stopping, despite unchanged desired gaze.
Resolve GlobalPtr slot (Game.cpp) and CameraCurrentProjection CALL site (same
pinned reference). Validate unique executable signatures, slot/module bounds,
target execute protection before patching the CALL displacement. FP capability
requires these additions. At that render call, armed/fresh/valid player and
prepared FP frame fix Ctrl.Camera_60 and Global.Camera_74's camera/view matrices,
point, up/back/right and distance; projection data stays untouched. Snapshot
CAMERA bytes for restoration before native input/camera processing. Restore
Global only while its current slot still identifies that embedded camera.
No writes to freed/replaced global work storage. Original projection is always
called. Mathematical inverse/eye/gaze tests pass; hook timing and visual/native
locomotion behavior still require actual Windows RE4 testing.

## 0.4.6 shared projection destination

User's read-only probe reports seven projection signature matches. 0.4.5
required one and refused FP before installing camera hooks. The old report
did not enumerate targets, so a common destination is not yet established
on Windows. Collect all matches in executable sections (limit32); require
every opcode E8 and every rel32 destination to be the same in-module executable
address, then patch each CALL site to projection_hook. Delegate to that shared
original target. Differing targets, invalid/out-of-module calls, empty/overflow
lists remain unsupported; no arbitrary first/majority selection. All other
patterns retain unique-match policy. C tests cover seven matching callers and
one discrepant/invalid/opcode-changed caller. Probe now lists every call's RVA,
target, bytes and protection. Real activation remains unverified.

## 0.4.7 observed two-target layout

The supplied probe identifies six callers of thunkRVA0xE395 -> E9 ->0x19F3A0
and one caller atRVA0x1F7BAD of thunk0x344F ->0x202F60. Pinned Game.cpp gives
CameraCurrentProjection VA0x59F3A0, corresponding to RVA0x19F3A0 (preferred
base0x400000). A version-gated explicit compatibility profile requires ALL
seven observed callerRVAs, both thunkRVAs and their E9 destinations, with
committed/readable/executable code. Only then select the six projection callers,
leaving the unrelated one unchanged. Unknown/missing/altered layouts reject;
no count-based vote or general fallback address. Module-relative validation
supports relocation. Generic one-shared-target behavior remains unchanged.
The portable C profile test uses the actual supplied RVAs/displacements, checks
six selected sites and rejects modified/missing/differently resolved candidates.
31 Python tests include the read-only profile report. This layout was identified
in user diagnostics; DLL0.4.7 runtime activation/render remains unverified.

## 0.4.8 stable reads and projection entry

User confirms FP activation/improvement0.4.7, but rapid WASD mixing is unstable.
221-sample trace shows stable desired gaze during several key transitions,
while Global camera can differ. The known profile now patches the validated
E9 entry atRVA0xE395, delegating directly to body0x19F3A0. This covers every
caller of that entry rather than only the six short-pattern callers. Generic
shared-target path is unchanged. No new unvalidated addresses/opcode changes.
ControlFrame seqlock reader retries3 times, then can reuse last complete frame
only with original timestamp <500ms, live armed and no stop. Never extends TTL.
This prevents partial publications from resetting mouse anchor/consumed total.
Live disarm/stop still dominate; stale cached armed frame latches stop2.
Camera/render/input share the same reader. C lease tests cover repeated retry
through expiry, disarm, stop and uint32 wrap. Windows visual result unverified.

tools/test_native_frame_read.py extracts the actual C reader, compiles it with
mocked clock/barriers and interrupts publication after copying fields. Verifies
the torn candidate never updates cache, retry obtains the full new frame, odd
publication reuses only unexpired frame, and live disarm/stop reject reuse.
No runtime Windows DLL is executed by this harness.

## 0.4.9 projection telemetry

Control IPC remains80bytes/v2. A separate64-byte v1 mapping named
`Local\RE4Bridge-CameraDiagnostics-PID` reports loaded build409, PID, calls,
writes, rejections, native tick counters and camera yaw before/write/after
original_projection. Its sequence is odd while a projection call is being
observed and even on completion. TRACE opens with FILE_MAP_READ only and
rejects partial samples. If creation fails, controls continue unchanged.
No new hooks, locomotion/camera algorithm or automatic arm are introduced.
The counters establish whether the validated entry actually runs during the
user's test; on-disk DLL hashes alone cannot do that. Hooks are expected to
run on the game's update/render thread, as before.

## 0.4.10 KEY_RUN

Allow mask0x4F: four direction bits plus native KEY_RUN0x40, verified from
SDK/pad.h at the pinned reference revision above. Aim0x10/reload0x20/fire0x80
remain forbidden. The native hook advertises capability4 for run in addition
to mouse1/FP2 and replaces only these five permitted bits after PadRead.
Opposite directions cancel; run with no remaining direction is removed.
FP relative movement translates direction to forward1 while preserving run64.
No speed/position/collision/animation-state edits. Diagnostic build is410.

## 0.4.11 one visual block

Separate BlockPreview mapping68bytes, control48bytes + native counters20bytes.
EndScene requires a coherent complete control/preview, matching generation,
fresh original leases, live armed and no stop. F8 also latches stop in EndScene.
No control protocol or movement/camera algorithm change; diagnostic build411.

An official absolute-system-path Direct3D9 hidden-window probe provides the
exact IDirect3DDevice9 vtable pointer. Search only writable non-executable PE
data sections for readable/writable interface pointers with that same vtable;
GetCreationParameters must identify a visible window of the current process
and current thread, constraining discovery to the game window thread.
Require one unique candidate (aliases allowed); atomically replace only its
interface vtable pointer with a private copy, changing EndScene alone. No
function prologue patching, device-address guesses or shared system-vtable edits.
Discovery tries at most5 times,2s apart, after120 pad ticks. Ex/wrappers may fail
validation; skip renderer and preserve input. No dummy window receives focus.

Draw world-anchored green diagnostic edges in screen coordinates using actual
Global CAMERA.v_mat and FOV. Near-plane/viewport clipping, finite/bounded inputs,
and orthonormal-row checks prevent giant/invalid draw vertices. No terrain depth,
collision, textures or native entity creation yet. Capture D3DSBT_ALL, change only
states needed to draw, apply backup and release even on setter/draw failure;
always return original EndScene result. No persistent default-pool resources.
Device Reset preserves the interface; whole-device recreation needs game restart.

## 0.4.30 coherent scene view / mapped gaze / native HUD

Capability8 accepts absolute mapped gaze via flag4 (FP+mouse required,
[-180,180]degrees); otherwise legacy cumulative mode remains. D3D Begin/EndScene
bound one camera view so input changes midway through rendering cannot alter
blocks relative to the scene. Collider still checks current feet. A seqlocked
internal view lease (actor/generation/controlTTL/age100ms) supplies FP overlays,
not mutable Global camera used by auxiliary passes. Repeated projections retain
16ms validation reuse and F8; dormant guards unchanged.

The borrowed Global.Flags_DISP+0x58 DPF_COCKPIT0x10000 attempt did not hide
already-created cockpit ID primitives in the user's real screenshot; withdrawn in0.4.32.
SDK references at existing pinnedrevision; no HUD function/vtable hook added.
No Y,velocity,speed,animation,inventory,health/ammo change. Build430; runtime
Windows ordering/appearance remains unverified. The separate Python map exporter
reads SAT/EAT layouts/signatures without process writes/native invocation.

## 0.4.32 targeted cockpit class visibility

Pinned public layout/signature reference: nipkownix/re4_tweaks commit
92c0208bd09c29c9640e13a2493b8e6bd6edd700, dllmain/Game.cpp IDSystem pointer
`B9 ? ? ? ? E8 ? ? ? ? 8B ? ? ? ? ? 8B C8 D9` immediate+1; dllmain/SDK/ID.h
IDSystem0x58, display-off[8]+0x2c, ID classes0x21/0x32/0x34. These classes share
word1+0x30 and masks0x40000000/0x2000/0x800. Only these visibility bits are leased;
no ID deletion, function/vtable hook, viewport masking or health/ammo mutation.
Unique executable signature, aligned static pointer inside bio4, committed
readable0x58/writableword, max/count runtime bounds gate writes. Capability16
states signature availability (31 with all prior features), not verified visual
suppression. Existing callbacks restore before native logic, reapply at validated
FP projection/pad and restore on F8/TTL/stop. Same-owner restoration preserves
other bits and previously disabled cockpit classes. Build432; control80/scenev6/
HUDv1 layouts retained. Eleven real C fixtures run with simulated memory/API;
Windows native UI rendering has not been executed in cloud.

## 0.4.33 session cockpit visibility / full overlay channel

User screenshot shows32 did not hide cockpit despite signature support31.
Pinned ID.h ID_UNIT0x144/be_flag+0 visible0x8/class+2, IDSystem.array+0x50:
HUDTweaks.cpp uses exactly this visibility bit to hide scope/binocular units.
Own bounded table traversal selects only0x21/0x32/0x34, saves256 identities and
original visible bits, reapplies while FP, restores only same array/capacity/slot.
HUD visibility lease lasts through pad/camera updates; camera/model restoration
is separate. All-stop/invalid camera/TTL/F8 restores native HUD. No HP/ammo writes.

OverlayCommand128-byte header +1600*900 ARGB pixels, separate mapped channel.
Heap staging/texture per facade, stable revision skips large copy/upload; bounded
seqlock and dimensions/selection/actor/generation/TTL/device/F8 guards. One native
2px black selected-shape projection precedes alpha compositing of full GUI/hand
image; world geometry remains RE4. Uses coherent native camera, not GUI coordinates.
No scene occlusion or shared terrain-raycast claim. Last overlay draw reports
viewport/leased-ID counts; telemetry is not proof of visual suppression. D3D9
state backup/restoration/Reset/escape lifetime follow existing facade policy.
Native build433; ABI80/v6 scene/legacy HUD1 preserved, new channel required for33.
Validation includes actual extracted callbacks and renderer with simulated APIs;
no real Windows driver or RE4 draw has been executed in cloud.

## 0.4.34 busy producer / cached overlay texture

Build434 retains the cockpit visibility implementation confirmed in433.
An incomplete overlay header or pixel-copy seqlock is a dropped update, not
a texture/device failure: retain and draw the last successfully uploaded image
only for the same arm generation, with its original timestamp younger than500ms.
Independent control freshness, native stop, F8, actor, Reset/escape and graphics
gates still apply before cached drawing. Expiry/generation change release cache.
IPC sizes/versions unchanged. Simulated renderer tests cover1,000 busy reads,
torn writes, image expiry with fresh controls and stop/F8/disarm/new generation.

## Build435: profundidad

Canal `Local\RE4Bridge-Depth-{pid}` de256bytes: status nativo0..207 y
comando host208..223 separados por seqlock, TTL500ms/generación. La fachada
creada por Direct3D mantiene el flujo de COM original. Registra proyección GX
en un pase opaco y dibuja bloques antes de Clear(Z) sin Clear(TARGET), o en
EndScene si no se detectó ese punto. Se valida la proyección D3D o la CAMERA
ProjMat_64 GX con FOV/aspect/planos antes de usar Z/ZWRITE/ZFUNC. Fuente SDK:
`nipkownix/re4_tweaks@92c0208bd09c29c9640e13a2493b8e6bd6edd700`,
`camera.h`, `GX.h`, `GXStruct.h`, `D3D9hook.cpp`. Solo lectura del SDK/cámara;
no se ejecuta código externo. F8/TTL/generación/Reset y restauración de estado
siguen siendo condiciones del renderer. La validación de drivers Windows
queda pendiente; CAPTURE registra las matrices si se rechaza la proyección.

## Build436: recuperación visual

Si no se valida proyección o falta depth buffer, dibuja caras con textura
al final de la escena usando estados sin profundidad. No cambia la cámara,
las colisiones ni las validaciones de matrices. Mantiene reason2/3 y aumenta
solo el contador de compatibilidad (offset60), nunca depth_draws. Rechazos
de control/generación, F8, TTL y restauración siguen obligatorios.

La matriz observada GX4:3 se admite en viewport widescreen. Bloques y contorno
usan focalX de esa matriz para XY, y los planos reales para profundidad. FOV,
estructura perspectiva, finitud y límites de aspecto/planos siguen validados.

## Build439

Una escena válida recortada enteramente fuera del viewport publica CUBE_ACTIVE
sin CUBE_DRAW. Recupera la métrica después de un pase omitido/rechazado y mantiene
la referencia física sin necesitar un dibujo. Antes, active podía quedar0 hasta
volver a mirar un bloque, provocando stop1/motivo4 por referencia de más de500ms.
Se mantienen los checks de lease, geometría, cámara, dispositivo y errores; no
se relaja el TTL. Fixture integrado de renderer/collision_tick:200 frames,
rechazos transitorios, pared en sprint sin penetración, vuelta a la vista y
cámara inválida.16 fixtures ASan/UBSan; GPU Windows pendiente.

## Build438

Colisiones: presupuesto de desplazamiento500+5×elapsed_ms, con lease500ms
conservada. El sprint medido~4158unidades/s puede superar el antiguo límite fijo
tras150ms de pases gráficos omitidos. Sigue el barrido desde la última posición
segura; saltos instantáneos/teletransportes se rechazan sin arrastrar al jugador.
El motivo4 de parada permanece visible en callbacks inactivos de la generación.
No cambia layout IPC. Fixtures de recuperación tras gap y salto real, ASan/UBSan.

## Build437

Corrige la expansión horizontal de36: GX virtual4:3 se lleva a focalX efectivo
igual a focalY en píxeles del viewport. Se conservan coeficientes/planos de
profundidad y se mantiene separado el GX original del resultado efectivo en
telemetría. Fuente de la corrección: regresión visual reportada y proporción
matemática33%; no se llama ni se modifica el shader del juego.

## Build441: vida compartida y menú

Nueva DLL. Health IPC v1,100bytes, seqlocks de petición y respuesta separados;
normalización de daño/curación en contadores acumulados con ACK de Minecraft.
Usa el callback pad existente y GLOBAL_WK.playerHpCur_4FB4/max4FB6 con el global
slot ya validado por firma única. Copia HP actual a cEm.hp324; no cambia máximo.
Para cero HP solicitado por Minecraft inicia Routine0::Die3 desde Move/Damage
usando cModel.r_noFC..FF, conserva muertes ya iniciadas. Layout y enum se leen
del SDK público fijado arriba. Leon/LeonAshley y MainLoop solamente; guardas
completitud/generación/lease/F8/puntero/memoria y revalidación antes de mutar.
Sin debug flags, hooks de daño, invulnerabilidad permanente ni resurrección.

look_flags bit8 representa menú, requiere botones0 en host; validación de mirada
admite ese bit manteniendo límites. Colisiones conservan callback/referencia
sin pase gráfico mientras el menú está abierto y no confunden su pausa de
inicialización con teleport. Guardia de movimiento normal/TTL/F8 intacta.
Fixture real health_tick valida escrituras/retención de muerte y fallos;
animación/ejecutable Windows necesita comprobación.
