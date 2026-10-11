# Evidencia de validacion — prototipo 0.3.0

Validado en Linux cloud, 2026-10-10, con Temurin JDK 21.0.8+9 y Gradle 8.11.1.
Descargas de JDK y distribución Gradle verificadas con SHA-256 oficial. El wrapper
tiene fijado el checksum de Gradle. Se mantuvo la validación TLS usando el proxy y
el almacén de certificados de confianza de la máquina.

| Comprobación | Resultado |
|---|---|
| `:bridge-core:test` | 9 pruebas pasadas, 0 fallos, 0 omitidas |
| `python3 -m unittest discover -s tools -p 'test_*.py' -v` | 15 pruebas pasadas |
| `-Pminecraft=true :minecraft:build` | Compilación y remapeo del JAR correctos |
| Pruebas Minecraft con `fabric-loader-junit` | 2 pruebas pasadas, 0 omitidas |
| Contenido del JAR | Metadatos 0.3.0, entrypoint cliente y nucleo anidado presentes |

La prueba Java ↔ Python abre TCP real en loopback y comprueba autenticación,
confirmaciones, pose, caja de losa, heartbeat y cierre. Las pruebas de Minecraft
usan su registro de bloques real con el cargador de pruebas de Fabric; comprueban
losas inferior/superior/doble, valla de más de un bloque de alto y ausencia de
colisión del aire. No se reemplazaron por cajas simuladas.

La primera ejecución de esas pruebas con JUnit sin el cargador Fabric falló por
acceso a métodos internos remapeados. Se corrigió la configuración con la
dependencia oficial `fabric-loader-junit`; la ejecución posterior completó ambas
pruebas. No se deshabilitaron aserciones ni pruebas.

## Prueba manual del usuario en Windows

El usuario cargo el mod con SKLauncher en un mundo superplano. Su captura muestra
conexion al simulador y 256 mensajes confirmados. Posteriormente confirmo que el
numero `blocks` aumenta al colocar bloques y disminuye al romperlos. Esto valida
el envio de cambios de bloques en su instancia; no prueba la integracion con RE4.

## Pendiente de comprobar en Windows

- Restauracion de perspectiva, pausa y reconexion desde la interfaz del juego.

## Componente lector de RE4

`tools/re4_reader.py` implementa deteccion y lectura externa de posicion en
Windows. Seis pruebas de memoria sintetica comprueban resolucion de firmas,
lectura de pose, cabeceras PE malformadas, version incompatible, firmas ambiguas,
punteros invalidos, muestras no finitas y transiciones. Junto con las cinco del
simulador, `python3 -m unittest discover -s tools -p 'test_*.py' -v` pasa 11 pruebas.
Estas pruebas no ejecutan las llamadas Win32 ni el juego. El usuario confirmo
posteriormente que el lector reconoce 1.1.0 y que posicion/angulos cambian al
mover/girar el personaje. Sus capturas muestran muestras `state: player` con
valores diferentes. OpenProcess inicialmente fallo con Windows 5; pudo acceder
tras ajustar los permisos de ejecucion en su PC.

## Conexion 0.2.0 y limites

Cuatro pruebas nuevas del nucleo Java comprueban muestras invalidas/no finitas,
caducidad local, conversion relativa/escala/signos/wrap de yaw, rechazo del modo
simulador al pedir RE4 y comunicacion Java -> Python por TCP real con
`re4_poll`/`re4_sample`, incluyendo desaparicion del personaje. El proceso Python
de esta prueba es una fixture sintetica; no se distribuye en el ZIP Windows.
Una prueba Python nueva valida el modo lector y su rechazo a poses salientes.

La compilacion/remapeo del mod y las pruebas de geometria Minecraft pasan.
El usuario confirmo que el seguimiento funciona en su PC. Su captura muestra
`follow: true`, 4555 ACK y posiciones variables en la consola del host.
La lectura del personaje confirmada por el usuario no verifica su camara,
escala a bloques, colisiones ni control desde Minecraft.

## Modulo de control 0.3.0

DLL C compilada como PE32/i386 con MinGW GCC 14.2.0, headers 12.0.0 y binutils
2.44, descargados con apt desde repositorios Debian verificados y extraidos en
workspace. Exports DirectInput inspeccionados; imports solo KERNEL32, USER32 y
msvcrt. No dependencia a DLL de runtime MinGW. La politica C de TTL de 500 ms,
wrap, parada, seqlock y teclas opuestas se ejecuto en Linux con nueve aserciones.

Tres nuevas pruebas Python verifican armado explicito, enclavamiento, rechazo
de comandos invalidos, perdida de personaje y falta de ticks. Una prueba Java
nueva intercambia controles con host Python por TCP real y IPC sintetico:
arranque desarmado, input correcto, corte tras 200 ms sin muestras y parada
explicita. No se presenta el IPC sintetico como una prueba del cargador real.

Instaladores ejecutados con PowerShell 7.4.7 Linux, descargado desde el release
oficial y verificado con hashes.sha256. Se comprobo copia de saves por hash,
rechazo de DLL existente, retirada propia/conservacion de DLL ajena y rechazo
de saves vacios. Windows PowerShell 5.1 y permisos reales siguen pendientes.
PowerShell inicialmente no pudo crear cache en /home; se corrigio mediante
rutas XDG dentro de workspace sin cambiar HOME.

La DLL no fue cargada en Windows/RE4 en cloud: sus firmas adicionales, hook,
movimiento, colisiones nativas, parada F8 y frames en segundo plano deben probarse
en el PC del usuario. No se valida todavia camara libre ni sincronizacion completa.

## Todavia no implementado

- Composicion de imagenes y sincronia de camara por frame.
- Colisión y navegación de RE4; geometría RE4 → Minecraft.
- Entidades proxy y daño de TNT, espadas o flechas entre juegos.
- Movimiento nativo verificado en juego; Ashley, puertas, cinematicas y guardados
  coordinados entre ambos juegos.

Una conexión al simulador no es evidencia de que RE4 funcione. No se ejecutó ni
modificó RE4 desde este entorno. Ningún ejecutable o recurso del juego se incluye
en el paquete de prueba.

## 0.4.0: orientacion y camara opcionales

W/S y A/D de 0.3.5 fueron confirmados por el usuario, sin retraso notable.
Alt-tab desarma como previsto. 0.4.0 agrega raton/camara experimentales;
todavia no hay prueba real de esas dos funciones. TCP Java/Python prueba los
campos look/capacidades y limites; publicacion IPC v2 usa memoria sintetica.
Pruebas C ejecutan el consumo acumulado y la matematica del ojo. La DLL
fastcall se compila x86 con limpieza de 12 bytes de argumentos de pila, y
solo imports KERNEL32/msvcrt/USER32. Ese chequeo no valida el hook en RE4.

## 0.4.1: ajuste de altura de primera persona

El usuario confirma giro por raton y Leon/camara acoplados en UHD. La primera
persona de 0.4.0 muestra las piernas pese a FP activo. Se aumenta altura
inicial de 160 a 1600 y rango absoluto de 250 a 2500 en cliente, host y DLL.
Referencia: Trainer.cpp de re4_tweaks 92c0208bd09c29c9640e13a2493b8e6bd6edd700
eleva la matriz de su camara libre 2000 unidades para quedar sobre el personaje.
Esto apoya probar otra escala, pero no confirma la altura de los ojos ni que
nuestro punto local use exactamente el mismo espacio. Se conserva ajuste
manual y signo. No cambia el giro, el hook, el protocolo ni los cortes de entrada.
Regresiones Java/Python/C cubren rango, transporte IPC y direccion a altura
alta. La posicion final de primera persona necesita prueba en Windows.

## 0.4.2: ojo anclado y cuerpo oculto

El usuario confirma que 0.4.1 situa la camara dentro del modelo de Leon y
solicita primera persona sin orbitas ni cuerpo visible. Se reemplaza la matriz
de camara por ejes identidad y traslacion pies + altura; mirada mundial yaw/pitch
normalizada. Se conserva giro nativo W/S/A/D y raton. cUnit be_flag_4 documenta
Trans 0x2 y DrawFootShadow 0x10: se limpian temporalmente solo esos bits del
jugador validado, con restauracion antes de logica nativa y sin escribir objetos
anteriores tras cambio de PlayerPtr. El argumento de matriz puede ser local;
solo se retiene copia si es la matriz estatica CamCtrl/QuasiFPS documentada.
Regresiones C comprueban que girar no desplaza el ojo, pitch vertical y mirada
unitaria, valores no finitos y conservacion de bits Alive/Move/ajenos al restaurar.
La efectividad del dibujo y la transformacion final siguen pendientes de Windows.

## 0.4.3 + vista 0.1.1

Usuario confirma vista y primera persona al moverse, pero deriva lateral en
reposo y arma flotante. Se agrega bypass condicionado del suavizado lr_rate
(Trainer.cpp de re4_tweaks, revision92c0208bd09c29c9640e13a2493b8e6bd6edd700):
CMP EBX+0x1B6 exacto y unico, hook preserva GPR/flags excepto ZF mientras FP
activo. Capacidad FP exige esa firma/hook. Ocultacion de modelos de arma
cPlWep0x34/0x38 con deduplicacion y restauracion con propiedad/vtable comprobados;
no se cambia inventario. Vista oculta panel totalmente al F9; Application.Run
conserva timer al ocultar. F8 ejecuta retorno directamente, sin PerformClick
sobre un boton oculto; F9 vuelve al panel. No se presentan las regresiones y
compilacion como prueba Windows de la nueva camara/render/WinForms.

## Cámara nativa 0.4.4 (mod/host 0.4.3)

El registro recibido de Windows contiene 223 muestras, DLL 0.4.3 en disco,
157 muestras armed/camera_active y 17 armed fuera del instante camera_active.
Con ratón inmóvil, yaw del jugador y los puntos Ctrl/QuasiFPS permanecen
estables al soltar W; no prueba cuál transformación de render provoca el
desvío visual. No se incluye el registro privado completo en el repositorio.
FP ahora reemplaza hitCheck en lugar de llamarlo primero, siguiendo el flujo
de cámara libre de la referencia. Roll cero, FOV desde QFPS_OFFSET validado;
cualquier fallo de validación conserva hitCheck original. La mirada usa
1000 unidades para reducir pérdida de precisión al sumar posiciones grandes.
Regresión C reproduce coordenadas/yaw/pitch del registro y verifica error
angular menor que 0.00002 radianes tras traducción float. Tres suites nativas
y 27 pruebas Python pasan. ZIP nativo de 14 archivos validado con Universal
Modder. Diagnóstico ampliado registra también CameraControl.Camera_60.
Compilación y pruebas sintéticas no demuestran que el desvío visible esté
resuelto: queda pendiente la prueba Windows del usuario.

## 0.4.5: cámara final y locomoción relativa

Se inspeccionó el vídeo del usuario y el nuevo registro (219 muestras, DLL
0.4.4). En 37.828s, yaw deseado/actor=1.841018, pero Ctrl.Camera_60 apunta a
2.049860; distancia del ojo pedido al punto final ~212 unidades. Look_D0 es
el eje trasero, no la dirección frontal: la comparación correcta usa su negativo.
La cámara final difiere al parar incluso con el punto QuasiFPS estable.

Nueva llamada validada a CameraCurrentProjection fija CAMERA matrices/punto/ejes
de Ctrl y Global justo antes de delegar en proyección; no modifica ProjMat.
Restauración de CAMERA antes de logica nativa, validación del GlobalPtr actual,
seqlock/TTL/arm/player y ramas obligatorias para capacidad FP. Pose independiente
para mirada; WASD elige rumbo de locomoción y envía un único avance nativo.
No se escriben posiciones ni se cambian flags de colisión; el motor mantiene
sus restricciones. En FP aplicado_yaw es mirada, mientras yaw del personaje
puede diferir por el rumbo solicitado. Fuera de FP continúa el modo tanque.

Tres suites C pasan: cardinales/diagonales, inversa de cámara, ojo proyectado
al origen y dirección en coordenadas del registro, además de controles/TTL.
29 pruebas Python pasan incluyendo lectura opcional/no mutante de Global.Camera
y validación de firma/objetivo de proyección.
Instalación/desinstalación y diagnóstico PowerShell pasan en fixtures. ZIP
completo de 31 archivos: Universal Modder 0 fallos/avisos. Incluye el JAR
0.4.3 sin recompilar (sin cambios Java), host/START/view y DLL nueva.
ABI camera fastcall y projection cdecl inspeccionadas en objdump. No se
ejecutó la DLL con RE4 Windows; movimiento lateral, render y colisiones reales
siguen pendientes. No implementa salto/sprint/agachado/física de Minecraft.

## 0.4.6 projection_matches=7

El PROBE del usuario confirma siete coincidencias de proyección y una de cada
otra firma requerida, con Global slot y cámara originales válidos. 0.4.5 sale
antes de instalar cámara por su requisito de coincidencia única. El informe
no incluye los siete destinos, así que no se afirma que sean la misma función.
0.4.6 exige un único destino común en todas las coincidencias E8 rel32, dentro
del módulo/ejecutable y hasta32 llamadas; solo entonces instala todas. Otras
firmas conservan unicidad. Destinos distintos/incorrectos siguen rechazados.
PROBE enumera llamadas/destinos/bytes/protección y resumen de destino común.
Tres suites C y30 pruebas Python pasan: siete llamadas comunes, rechazo de
una discrepante, opcode distinto, dirección fuera del módulo y lista vacía.
Fixtures PowerShell de instalación y diagnóstico pasan. No se ha verificado
la activación FP con DLL0.4.6 en el ejecutable Windows real del usuario.

## 0.4.7 profile de dos destinos observado

PROBE0.4.6 del usuario muestra projection_unique_targets=2: seis CALLs al
thunk0xE395 con E9 06 10 19 00, que resuelve0x19F3A0, y una al thunk0x344F
con E9 0C FB 1F 00, que resuelve0x202F60. Se verificó el Game.cpp de la
revisión pública92c0208bd09c29c9640e13a2493b8e6bd6edd700: identifica
CameraCurrentProjection VA0x59F3A0 (RVA0x19F3A0). No se eligió por mayoría.
Perfil explícito exige los siete sitios RVA exactos, sus thunks/destinos E9
y protecciones ejecutables; selecciona seis sitios y deja la otra función intacta.
RVA relativos al módulo; no dirección absoluta ni perfil para otros ejecutables.
Destino común genérico sigue disponible. Desconocidos/alterados/incompletos
permanecen rechazados. Tres suites C y31 pruebas Python pasan; fixtures de
instalación/diagnóstico también. Activación/render Windows0.4.7 pendiente.

## 0.4.8 cambios rápidos de teclas

Usuario confirma mejora/activación FP0.4.7, pero combinaciones rápidas alteran
la cámara. Registro0.4.7 de221 muestras muestra aplicado_yaw estable en varios
cambios sin ratón y divergencia de Global.Camera; lecturas no atómicas no
prueban la causa exacta de cada sacudida visual. Se encontraron dos problemas:
solo seis llamadores de proyección se enganchaban y muestras incompletas de
seqlock reiniciaban estado de mirada. Perfil observado ahora modifica la entrada
E9 validada0xE395, delegando directamente al cuerpo0x19F3A0, para cubrir otros
llamadores sin recursión. ControlFrame reintenta3 veces y puede usar última
muestra íntegra, solo con timestamp original vigente (<500ms), live armed,
no stop. No extiende TTL; caducidad latches stop2 y exige rearme. Cámara,
proyección e input usan el mismo lector. W+S/A+D siguen cancelando eje.
Tres suites C y32 pruebas Python pasan. Harness compila el lector C real con
barreras/reloj simulados e interrumpe publicación después de copiar campos;
verifica reintento, exclusión de candidato roto de caché, live stop/disarm,
caducidad y wrap. Fixtures instaladores/diagnóstico pasan. No se ejecuta DLL
Windows en esas pruebas; estabilidad visual/locomoción real pendiente0.4.8.

## 0.4.9 projection diagnosis

Reviewed user's gpt.mp4 and216 observations from trace020114. Stable requested
view yaw with~0.209rad idle Global deviation is present after initial mouse
movement. 0.4.8 has not solved the visual behavior. Added independent native
projection telemetry rather than another unverified camera algorithm.
Three portable C suites and36 Python tests pass, including rejecting short,
unknown, odd-sequence and nonfinite telemetry. Installer fixtures preserve
backup/hash/own-only removal. PE32 compiled, Windows gameplay not run here.

## 0.4.10 run input

Pinned SDK/pad.h at92c0208bd09c29c9640e13a2493b8e6bd6edd700 identifies
KEY_RUN0x40. Capability4 and exact permitted mask0x4F added across native,
host and mod. TCP fixture verifies sprint69 and release5 without disconnect;
unknown actions16/32 rejected. Native tests cover relative conversion69→65,
cancelled axes/no-direction stripping run, stop/disarm/TTL. Gradle executed
15 core and2 Minecraft tests with0 failures/skips; Python37 and three C
suites pass. Native PE32 compiled; gameplay run speed not tested on Windows.
Trace022618 confirms loaded build409 and zero projection writes despite
increasing entries. Camera algorithm preserved; no claim of visual fix.

## 0.4.11 visual block

User confirmed0.4.10 sprint in Windows. Added selected-block diagnostic cube,
fixed native bounds, live TCP preview/renderer status and independent68-byte IPC.
17 core tests and2 Minecraft tests pass (no skips); control fixture authenticates
and transmits preview, status and sprint; immutable native box anchoring tested.
41 Python tests pass, including bounds, preview removal/disarm/F8/generation/TTL
and a fixture compiling the actual native EndScene body with synthetic COM calls.
For each setter/draw error, graphics backup is applied before original EndScene;
capture failure does not mutate graphics; resources released. Four portable C
suites pass, including near-plane/viewport clipping and stationary-world anchor
under viewer movement. PE32 DLL compiled with warnings treated as errors.
Actual D3D9 device discovery/rendering not executed on Linux; public reference
was consulted for SDK, not copied as a wrapper. No textures/occlusion/collision,
no save/entity/position edits. Windows visual test remains required.

## 0.4.12 device discovery and focus fallback

User screenshot: discovery status3, zero frames/draws on0.4.11; no real block
render has been confirmed. Native detector's actual helpers are now exercised
with synthetic memory: different validated vtables, window thread independent
of pad thread, split regions, one-level roots, duplicate roots, ambiguous devices,
foreign-process/hidden windows, guarded memory and foreign executable methods.
Native PE32 build and4 portable C suites pass.43 Python tests pass; mappingv1/v2
and counter preservation checked. VIEW backend and real PowerShell return action
fixtures verify native stop/window restoration despite denied foreground request.
No GPU, driver, real Windows GUI or gameplay executed; Linux success is not
evidence that a block renders on the user's machine. Minecraft0.4.11 retained.

## Read-only graphics audit after0.4.12

Uploaded console log:status3, zero tables/devices/frames/draws after5 attempts,
33,360 readable object pointers and1,050 roots. Also fails with armed=True and
camera_active=True; no evidence mouse/FP prevents rendering. Exact discovery
rejection remains unknown. Added standalone external audit; no native or mod
change. Five synthetic tests exercise indirect reference beyond old256-byte
prefix, factory slot, private table/cross-module methods, guarded memory, unknown
pointers, executable filter, resource bounds and local report/handle cleanup on
success/failure. Process bytes preserved. Real Win32/driver inspection pending.

## 0.4.13 private D3D9 table regression

User audit081059 shows a private table whose119 method addresses belong to the
system D3D9 module and whose pointer is at0xC30 in a referenced structure. Previous
detector excluded both storage and offset. This is diagnostic evidence of a
candidate, not proof of an active device. Detector now allows readable private
tables but still validates introspection method identity and executable module
ownership for all119 entries. Requires own visible window and exact game factory
returned by GetDirect3D; releases the returned reference on acceptance/rejection.
Rechecks factory before hook; scans bounded4096-byte prefixes. Native helper
fixture covers private-table-plus0xC30 regression, foreign executable methods,
different introspection methods, factory mismatch/refcount balance and existing
region/ambiguity protections.48 Python tests and4 C suites pass, PE32 builds with
warnings as errors. Preview IPCv1/v2/v3 preserves native counters. Minecraft
mod unchanged. No real driver/game execution; render remains pending Windows.
## Recuperación 0.4.14 tras regresión de arranque

El usuario probó 0.4.13 tres veces y RE4 se cerró antes del menú principal.
Las pruebas sintéticas de descubrimiento no detectan objetos COM muertos ni
validan una tabla real del controlador: no eran evidencia suficiente para ese
hook. El detector se excluye por completo del binario de recuperación; inspección
con MinGW nm confirma ausencia de símbolos block_* y conserva pad_hook,
install_hook y DirectInput8Create. El header retirado tiene #error para impedir
reactivación accidental. No se modifica el algoritmo de controles/cámara.

PE32 compila con -Werror y pasan4 suites C/48 pruebas Python. Las fixtures del
renderer retirado son históricas y no acreditan gráficos habilitados. Cuatro
fixtures PowerShell verifican extracción de Application Error/WER y exclusión
de eventos ajenos/identidad ausente. Get-WinEvent y RE4 no se ejecutan en Linux.
Instalador verifica copias y DLL; desinstalador elimina únicamente hash propio.
Minecraft/API no cambian, por lo que se reutilizan los JAR ya validados.
El usuario confirmó después que 0.4.14 inicia y carga una partida de RE4 sin
problemas. Su informe re4-crash-20261011-054530.json, generado a08:45:30 UTC,
contiene events: [] para el intervalo consultado de siete días. Esto no demuestra
ausencia de cierres anteriores ni identifica módulo/excepción/dirección. Retirar
el detector elimina el fallo observado en esta prueba; la causa exacta anterior
sigue sin confirmar. No se probó dibujo (desactivado) ni se repitió una prueba
completa de controles/cámara en esta confirmación de arranque.
