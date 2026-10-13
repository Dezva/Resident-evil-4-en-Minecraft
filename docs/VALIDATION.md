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

## 0.4.15 — creación gráfica, sin renderer

La captura utiliza funciones nuevas independientes del renderer retirado.
Import lookup acotado sobre cabeceras/imports PE32; no escritura durante búsqueda.
Fixture del instalador ejecuta la función real extraída: API oficial SysWOW64 y
fallback Windows32, CAS/objetivo original, import cambiado, biblioteca externa,
límites, permisos de página ejecutable preservados, query/protect fallidos y
rollback si falla restauración. APIs COM ausentes en este fixture de instalación:
no se ejecutan métodos durante DllMain. No se crea mapping/heap en ese recorrido.

La fachada real de fábrica se ejecuta contra17 métodos tipados simulados.
Verifica forwarding de argumentos/resultados, identidad IUnknown/IDirect3D9,
referencias propias balanceadas incluso si un dispositivo retiene fábrica nativa,
asignación fallida, fábrica NULL y salida/parámetros inválidos cuando CreateDevice
falla. El dispositivo permanece byte a byte intacto; no se llama su vtable.
Tres fixtures pasan también con ASan/UBSan. No prueban interoperabilidad real del
driver/juego ni identidad comparada contra GetDirect3D de un dispositivo nativo.

Lector IPC96bytes abre solo FILE_MAP_READ;4 pruebas comprueban campos del informe,
snapshot cambiante/ocupado, cabeceras/build/PID inválidos, canal ausente, bytes sin
modificar y limpieza de handles/proceso incluso al fallar.55 pruebas Python y4
suites C pasan. DLLPE32 compila con -Werror, solo imports KERNEL32/msvcrt/USER32.
Símbolos capture_* presentes; block_*/EndScene y mapping BlockPreview ausentes.
Mod/API se reutilizan sin cambios. Instalación/desinstalación respaldos/hash propio
probadas; no se ejecuta RE4 ni Direct3D9 real en Linux. Windows requiere abrir,
cargar y ejecutar CAPTURE-RE4-GRAPHICS; todavía no hay dibujo ni colisión de bloques.

## Captura0.4.15 rechazada — diagnóstico externo

Salida del usuario: hook_status3, creaciones0, dibujo desactivado. No demuestra
causa concreta ni captura de dispositivo funcional. Se mantienen los artefactos
0.4.15 y0.4.14 sin modificación; no se publica otra DLL con filtros relajados.
Nuevo auditor de imports de solo lectura verifica condiciones actuales, no las
que existían durante DllMain. Datos de destino se ubican en módulos conocidos,
no se dereferencian objetos ni se ejecutan COM. Resultados incompletos dejan
estado de simulación desconocido, en lugar de presentar rechazo reproducido.
Cinco pruebas sintéticas comprueban bytes conservados, OFT ausente/IAT resuelta,
valor cambiado, duplicados/ordinales, forwarder, rutas/bounds/guardas/límites y
reportes/limpieza al fallar. ZIP autónomo6archivos extraído: --help funciona sin
checkout/config; no contiene DLL/JAR/EXE/instalador. UM publish check6archivos,
0fallos,0avisos. Auditoría real pendiente de imports-audit del Windows del usuario.

## 0.4.16 — evidencia del usuario y reintento registrado

graphics-capture-20261011-135432.json confirma el rechazo de0.4.15, todas las
llamadas/capturas a cero. imports-audit-20261011-135454.json completa604lecturas y
2361bytes: una importación por nombre Direct3DCreate9, slotRVA0x1310CE0,
valor0x68EB4B20 igual a la exportación oficial del D3D9 cargado/esperado en
SysWOW64. current_native_lookup_status1 con hook_status3 previo no establece la
causa del rechazo durante DllMain. Sin evidencia de dibujo/captura real funcional.

El instalador nativo real extraído registra motivo inicial y último intento.
Fixture comprueba IAT no resuelta inicialmente y luego resuelta, reintento único,
registro inicial conservado, biblioteca ausente y luego disponible, conflicto
persistente y ausencia de reintento si listo/fallo de protección. Conserva pruebas
de ruta/bounds/CAS/rollback/protección y LastError. No ejecuta COM al instalar.
Tres fixtures de captura pasan ASan/UBSan; no prueba cargador/driver real.

IPCv2/build416/168bytes conserva los24campos iniciales y añade fase/contador/dos
intentos. Lector FILE_MAP_READ acepta también v1/build415/96bytes; prueba rechazos
de build/PID/motivos, snapshots completos, dos vistas de lectura y cierre ante
fallo del remapeo extendido.61 pruebas Python y4 suites C pasan; compilación
PE32 con -Werror e instalador/desinstalador respaldos/hash propio verificados.
Mod/API se reutilizan sin cambios. Arranque y captura0.4.16 pendientes de Windows.

## 0.4.17 — rechazo de ruta identificado, corrección por probar

graphics-capture-20261011-141909.json contiene build416, fase DirectInput8Create,
dos intentos, motivo7 en inicial/último y contadores gráficos cero. No demuestra
problema de carpeta ni fallo de CreateDevice: no se llegó a observar esa llamada.
Se identifica la comparación de ruta como bloqueo; no se conocen las cadenas.

Fixture ejecuta el instalador real con ambas APIs disponibles y ruta cargada
System32; caso antes rechazado y ahora aceptado. Verifica SysWOW64, Windows32,
case-insensitivity, buffers truncados, rutas/API ausentes y longitud excesiva.
DLL en carpeta del juego y subdirectorio de sistema se rechazan sin escritura.
Conserva pruebas de import/CAS/protección/rollback/LastError/reintento único.
La fachada no cambia;3 fixtures de captura pasan ASan/UBSan.

LectorIPC3296/v3 acepta también96/v1 y168/v2, reconoce exclusivamente los builds
correspondientes y valida rutasUTF-16 con terminador/matched. Fixture conserva
bytes, devuelve las rutas de los dos intentos y rechaza UTF-16 inválido, falta de
terminador o identidad desconocida; cleanup y permisos de lectura conservados.
62 pruebas Python/4 suites C pasan, compilaciónPE32 -Werror e instalación/hash
propio comprobados. Mods/API no cambian. Falta Windows para comprobar rutas
reales, arranque y creación observada; ningún dibujo de bloques está habilitado.

## Captura inicial0.4.17 confirmada en el equipo del usuario

Informe graphics-capture-20261011-142924.json, UTC17:29:24: build417,
hook_status1, motivo1, DLL_PROCESS_ATTACH, attempts1. Import slotRVA0x1310CE0,
observed/expected0x68EB4B20. loaded C:\\WINDOWS\\SYSTEM32\\d3d9.dll coincide con
system C:\\WINDOWS\\system32\\d3d9.dll; matched SystemDirectory. El formato de ruta
causante del rechazo queda confirmado; no hizo falta el reintento.

factory_calls2/success2, wrappers_live1, wrap_failures0, device_calls1/success1,
device_failures0, HRESULT0x00000000, escaped_interfaces0, creation_observedtrue.
CreateDevice devolvió dispositivo tipoHAL, adapter0, flags0x44, windowedtrue y
1366×768. El informe acredita instalación y creación observada con RE4 real,
no métodos de render, vida actual del puntero, estabilidad prolongada, pérdida,
Reset ni recreación. rendering_enabledfalse. No se modifica DLL ni se repiten
pruebas cloud por este registro documental; los ZIP publicados se conservan.

## 0.4.18 — dispositivo recibido de CreateDevice y dibujo explícito

La fábrica real invoca cube_wrap_device solo con salida correcta y owned, y
devuelve la fachada manteniendo HRESULT/LastError; fixture comprueba también
fallback, ausencia de llamada en fallo con punteros inválidos y bytes nativos
intactos. Façade119métodos compilada contra SDKMinGW real -Werror; fixture portátil
envía argumentos/resultados distintos en112slots delegados y comprueba los7
métodos especiales: identidad/refcounts, objeto/vtable intactos, interfaz nativa
viva al liberar fachada, escenas correctas/fallidas, Reset/Present, bloqueo y
fallos de mapping/asignación. La generación usa solo declaraciones estándar SDK.
Callback reentrante simulado durante Reset vuelve a consultar la fachada sin
bloqueo; dibujo suspendido durante Reset y cleanup al fallar el bloqueo comprobados.
La referencia temporal por llamada mantiene vivo el wrapper si ese callback
libera la última referencia del llamador; se libera al terminar el método.

Renderer real probado con estado/COM simulados:23setters fallidos y DrawPrimitiveUP
fallido restauran el estado completo capturado, incluyendo efecto de stream0.
CreateStateBlock/Capture fallidos no cambian estados; Apply fallido libera backup
y bloquea futuras órdenes. Superficies/stateblocks equilibrados. Targets ajenos,
cámara ausente, pérdida, F8, caducidad con wrap y anclaje fijo/nueva generación
comprobados. Reader real de CommandIPC rechaza odd/torn/malformed y no cachea
activaciones. Seis fixtures nativos pasan ASan/UBSan; no ejecutan driver/RE4.

CLI probado con reloj virtual: countdown, duración, F8, fallo nativo, activación
con mutex, rechazo de segundo escritor, limpieza incluso en error, permisos6
solo en comando y4solo en telemetría, snapshots/version/build/PID, cierre y JSON.
70 pruebas Python/4 suites C pasan; instalador/desinstalador backups/hash propio
verificados. Sin tests/rebuildJava: mods/API idénticos. No se habilita blocktest
ni se restaura detector retirado. Falta pruebaWindows para arranque, fachada
compatible, cubo visible/proyección, cambios de dispositivo y estabilidad real.
Conteo de DrawPrimitiveUP correcto no acredita por sí solo visibilidad.

### Confirmación visual del usuario en Windows — 0.4.18

El usuario confirmó que el cubo apareció correctamente y adjuntó una captura de
RE4 con las aristas verdes visibles. `cube-test-20261011-151401.json` identifica
build 418, un dispositivo vivo y parada por duración tras 30,04 segundos.
Los dibujos aumentan de 1.734 a 3.500 (1.766 adicionales); los fallos de dibujo
y restauración permanecen en cero, con HRESULT final 0x00000000 para ambos.
Sin interfaces escapadas ni fallos de wrapping/cámara. Reset permanece en 3,
fallos de Reset en cero y fallos de Present en 138: estos últimos ya estaban
presentes antes de la prueba, no surgieron durante ella. `last_scene_active`
describe el último frame antes de desactivar, no una autorización vigente.

Esto acredita arranque y dibujo visible durante esta prueba. No acredita todavía
anclaje/proyección precisa al moverse, recuperación de pérdida/Reset durante
el dibujo, estabilidad prolongada, colisiones ni colocación desde Minecraft.
Registro documental sin reconstruir la DLL ni modificar los ZIP publicados.

### Recorrido con cubo — informe 153629

El usuario confirmó que el cubo se mantuvo en su sitio en el nuevo recorrido.
Se inspeccionaron fotogramas de `testeadocubo.mp4`: el cubo cambia de tamaño y
posición en pantalla al acercarse/alejarse y reaparece al volver a mirar hacia
su ubicación. `cube-test-20261011-153629.json` registra 1.164 dibujos adicionales
(6.583 a 7.747) durante 30,02 segundos, parada por duración y cero fallos de
dibujo/restauración. Reset permanece en 8 y fallos de Present en 426, sin nuevos
eventos de esos tipos durante la prueba. Los pases omitidos aumentan en 628;
ese contador agrupa pases de render ajenos y geometría fuera de vista, por lo
que no identifica por sí solo la causa de cada omisión.

Evidencia de anclaje durante el recorrido, sin resolver la causa del desplazamiento
extraño referido anteriormente ni acreditar precisión de proyección, estabilidad
prolongada o recuperación de dispositivo. No cambia el código ni el paquete.

## 0.4.19 — colocación y retirada del último bloque

72 pruebas Python pasan, incluyendo TCP autenticado real -> ControlPolicy ->
NativePreview/CubeSession con mappings Windows simulados. Se comprueba publicación
de bounds/mode, retirada al omitir caja, parada F8 sin rearm automático, nueva arm
y desactivación al cerrar conexión. Apertura v2 y permisos separados comando6 /
telemetría4, cabeceras/build antiguos rechazados sin activación, mutex excluyente,
epoch fijo/renovación y telemetría conservada. Fallo de dibujo desarma ambas
órdenes y necesita nueva arm; contador de restauración impide nuevas activaciones.
Una razón antigua de la última escena no rechaza de inmediato el nuevo epoch.

17 pruebas bridge-core y 5 de Minecraft pasan con JDK21/Fabric Loader JUnit:
capacidad de preview recibida por el cliente, protocolo de control existente,
BlockPlacementProbe confirma una sola colocación por cambio del bloque esperado,
conserva caja nativa fija, rechaza intento sin cambios/bloque incorrecto/expirado
y cancela al cambiar dimensión. Las pruebas de formas de losas/fence se conservan.
La interacción gráfica del callback en un cliente real sigue pendiente de Windows.

4 suites C portátiles y compilación MinGW PE32 -Werror pasan. Seis fixtures nativos
ASan/UBSan pasan; el renderer real recibe bounds explícitos, los mantiene al
cambiar cámara/personaje, actualiza ante otra caja y retira al desactivar.
Cajas invertidas/NaN no dibujan; snapshot v2 real rechaza reservado, modo,
versiones y publicaciones incompletas. Restauración/refs/Reset/fallback mantienen
su cobertura anterior. Instalador/desinstalador verifica backups/hash y no pisa
DLL ajena. APIs simuladas no acreditan driver o visibilidad de esta integración.

## 0.4.20 — captura duplicada, tamaño y telemetría de solicitud

73 pruebas Python, 17 bridge-core y 10 Minecraft pasan; mod0.4.20 compilado
con JDK21/Fabric. Tracker verificado contra registro real: callback/tick
duplicados retienen el primer anclaje, segunda mano/intento no borra una
confirmación retrasada, tecla mantenida no extiende plazo, material equivocado
no confirma, último confirmado prevalece, dimensión/clear cancela, cola≤16.
TCP autenticado transmite requested/skipped/reason; caja detectada desarmada
no habilita canal, nueva arm lo habilita, cese de muestras/parada lo retira.
Telemetría nativa permanece intacta y epoch no cambia con renovaciones.

Fabric lifecycle0.116.6+1.21.1 confirma START_CLIENT_TICK en HEAD de tick y END
en RETURN. Esto valida el punto de observación previo a la interacción, no
prueba la GUI real. Manual100 visible pero diminuto en captura del usuario;
default1000 provisional, pendiente de calibración y colocación automática real.
DLL conserva hash/build419; no se modificó ni recompiló código nativo.

## 0.4.21 — escena rígida de varias cajas

79 pruebas Python,19 bridge-core,15 Minecraft,4 suites C portátiles pasan;
compilaciones JDK21/Fabric y MinGW PE32 -Werror completas. Seis fixtures nativos
con ASan/UBSan pasan, incluidos reader real, DrawPrimitiveUP batch128, rollback
gráfico ante fallos y restauración fatal, COM/lifetime/fábrica/hook anteriores.
Instaladores/diagnóstico PowerShell verifican backup, hash, no-overwrite,
retirada propia y lectura IPC. Native imports limitadas KERNEL32/msvcrt/USER32.

Escena MC: distancia y altura de una pared, borrar el bloque origen sin mover
los demás, vaciar/rellenar con referencia conservada, losa inferior/superior/doble,
giro alineado MC/RE4, height común, límite64 y rechazo sin pérdida, cambio de
dimensión y chunks sin cargar comprobados con registro real Minecraft sin GUI.
Bloques sin formas se rechazan; no se copia superplano ni entidades.

TCP autenticado real -> ControlPolicy -> NativePreview/CubeSession v3 con APIs
Windows simuladas verifica origen, count, formas, retirada individual,
F8/nueva arm, desconexión; v2 conservado para único. Valida128 cajas y formas
finas; overflow/NaN/boolean/origen incorrecto rechaza antes de escritura. Orden
v3 de3152 bytes con publicación impar/par y body completo; entradas sobrantes
se borran al reducir/retirar. Header/build incompatibles no activan dibujo.

Geometría C verifica composición cámara*origen/rotación, proyección fija de
varias cajas y losa, traslado de vista sin movimiento de la escena, retirada,
rotación,128 cajas en un batch, reservados/count y invalidación de snapshot.
Todas las23 fallas de setter anteriores conservan restauración. Dispositivo
nativo/vtable siguen sin alterarse. Las APIs simuladas no prueban driver,
arranque real ni visibilidad de0.4.21. Usuario confirma0.4.20 con vídeo, pero
0.4.21 todavía necesita prueba real. El suelo RE4 no se ha leído: height es un
ajuste visual, y formas de colisión MC no son colisiones instaladas en RE4.

## 0.4.22 — caras grises y evidencia de la cuadrícula anterior

El usuario confirma 0.4.21 en Windows: colocó dos filas de tres piedras y una
losa, rompió parte del conjunto y las restantes conservaron su sitio. La captura
muestra cinco cajas. Esto verifica disposición y retirada visual, no física.

En la nube pasan84 pruebas Python,20 bridge-core,16 Minecraft y4 suitesC.
Siete fixtures de los helpers nativos reales pasan ASan/UBSan con APIs simuladas:
import, instalación/rollback, factory, device, command reader, draw y superficies.
La última valida recorte contra cinco planos, máximos, caras cubiertas, sombreado,
orden por distancia, media losa, retirada y2000 escenas con cámara rotada.
Draw cubre los24 fallos de setters en wire/solid, memoria temporal y referencias,
Capture/Apply/CreateStateBlock/Draw/alloc fallidos, restauración, F8 y caducidad.
TCP lleva estilo1/escena, retirada y cierre al mapping v4; versión anterior
rechaza solid sin activar input. Se mantiene cobertura de rollback y pérdidas.

JAR0.4.22 y DLL PE32 compilados con warnings como errores en C. DLL importa
KERNEL32, msvcrt y USER32; no incluye detector de dispositivo retirado. Se
verifican copia de partidas, hashes, no sobrescritura, retirada solo de DLL
propia e interop de diagnóstico mediante fixtures PowerShell. El paquete debe
incluir ambos mods, host, DLL y scripts completos. Apariencia y rendimiento de
caras, arranque de RE4 y driver D3D9 reales requieren la prueba del usuario.

## 0.4.23 — textura de prueba y confirmación manual de caras

Usuario confirma0.4.22: dos capturas muestran una construcción con caras grises
rellenas y la misma en aristas. No acreditan profundidad compartida ni física.

88 pruebas Python,20 bridge-core,16 Minecraft y4 suitesC pasan; siete fixtures
nativos ASan/UBSan. La geometría valida UV de intersección near interpoladas antes
de proyectar, rhw positivo1/depth, recorte finito y extremos/2000 escenas rotadas.
Draw ejecuta código real con COM simulado: tile opaca/variada16×16, pitch80bytes
para filas64bytes,128cajas en una llamada, fallos de38 setters(textura)/30(otros),
CreateTexture/Lock/Unlock/Capture/Apply/Draw/heap, recursos liberados, F8/TTL y
restauración completa. TCP real transmite escena/style2, retirada individual y
cierre al mappingv5; estilo2 incompatible no publica input ni activa dibujo.

JAR0.4.23 remapeado y DLL PE32 compilada con warnings C como errores; imports solo
KERNEL32/msvcrt/USER32. Instaladores y diagnósticos validados con archivos/APIs
simuladas. Se conservan la creación/interceptación D3D9 y hooks de entrada/cámara.
Apariencia/rendimiento/arranque real de textura requieren Windows/RE4/driver.
No se distribuyen imágenes originales de Minecraft; patrón generado por código.

## 0.4.24 — materiales y primera barrera

El usuario confirma la adherencia de textura0.4.23 al moverse, con captura.
Se solicitan materiales por bloque y colisiones. Se añade extracción local de
recursos/modelos, paleta/UV/tintes/alpha y barrera horizontal experimental de Leon.

97Python,21bridge-core,20Minecraft y4suitesC pasan.9fixturesnativosASan/UBSan.
TestsMinecraft verifican PNG/ARGB/tinte/composición, imágenes inválidas/dimensiones,
UV afines y media losa; no arrancan MinecraftClient ni su renderer de modelos real.
TCP autenticado transporta dos tiles de colores/alpha, caras y flag físico hasta
mappingv6, retira individualmente y desactiva al parar. Schema y versiones,
límites/torn/cleanup y ACK validados. Las imágenes reales no se distribuyen.

GPU simulado prueba atlas/pitch/bordes/alpha y43fallos de setters; conserva cobertura
de recursos/estado/lifetime. CPU física prueba paredes/barrido/slide/esquinas,
altura, rotación, salida y10000casos. Callback real simulado verifica F8/TTL/gen,
render habilitado, pausa/teleport, no Y/otros campos, puntero/pose cambiantes,
reentrada, y no escritura bajo rechazo. No verifica el comportamiento de la
física RE4 ni la llamada/orden reales de callbacks en Windows.

DLL/JAR compilados y PE32/imports inspeccionados; instalación/respaldo verificado/
no-overwrite/own-onlyremoval/diagnóstico pasan. Extracción de modelos activos,
arranque, apariencia, rendimiento y estabilidad física requieren prueba real.
Apoyo encima/gravedad/saltos, enemigos/Ashley/proyectiles, oclusión/terreno siguen
pendientes; no se considera completada la integración física del motor.


## 0.4.25

Regresión comprobada con sprites reales: SpriteContents sin animación devuelve
un flujo que contiene 1; la versión anterior lo usaba como índice y rechazaba la
imagen estática. Pruebas usan NativeImage y BakedQuadFactory reales con el access
widener Fabric, incluyendo seis caras, imagen generada sin PNG, ABGR/ARGB, capas,
primer frame declarado y conservación de la imagen prestada. Se verifican caché
estable durante 200 actualizaciones, recarga, losa doble, eliminación e Input
restaurado tras detener, desconectar, cambiar jugador y reemplazo ajeno.

No se ha ejecutado RE4/driver D3D9 real ni medido FPS en el equipo del usuario.
DLL/instaladores/API permanecen idénticos a 0.4.24. Se mantienen las pruebas TCP
de caducidad (un worker vivo no mantiene inputs antiguos) y el protocolo v6.

Resultado 0.4.25: 97 Python + 21 bridge-core + 24 Minecraft, cero fallos/omisiones;
build Fabric/remap/access-widener terminado correctamente.


## 0.4.26

Regresiones de presión: callbacks reales de colisión conservan posición segura
ante escena tornada/gráficos temporales;1999 actualizaciones con avance60unidades
por frame no penetran después de cada callback válido. No se confunde esto con
colisionadores del motor: se preservan Y y física nativa y se detiene ante saltos.
Cámara/projection/pad reales extraídos: auxiliares, idle, pad tardío,270 cambios de
dirección, nuevo pie, yaw120deg completo, F8/TTL/generación y restauración de modelo.

HUD lector real y renderer compartido:48 fallos de setters, creación/lock/pitch/
unlock/textura/nulo/dibujo/restore y gates. No marca activos los bloques por dibujar
HUD. Mappings/estado D3D APIs simuladas. Bootstrap PNG/sprite/quad Minecraft reales;
GUI sprite prestado/crop/ARGB, transformación rígida y ojo/altura, modo temporal
con cada combinación de flags y cambio de dueño. TCP real envía HUD43692 caracteres
con escena y sigue desarmando por200ms sin muestras. Instaladores sin cambios.

La nube no ejecuta RE4, driver D3D9 ni MinecraftClient/servidor local en juego:
arranque, raycasts/edición sincronizada, rendimiento, cámara y presión física
necesitan la prueba Windows descrita en RE4-BLOCK-0.4.26.md. El mapa auxiliar no
se genera ni importa por sí solo; HUD es un primer conjunto de elementos.

Resultado:103 Python,24 bridge-core y27 Minecraft, sin fallos ni omisiones.
Once fixtures nativos con ASan/UBSan pasan; cuatro suites C portátiles y los
instaladores/diagnósticos PowerShell simulados también. DLL PE32 compilada;
access widener/JAR Fabric remapeados y build completo terminados.

## 0.4.27

Revisión específica de la regresión de rendimiento comunicada por el usuario.
El renderizador real bajo D3D simulado muestra una sola creación/carga de HUD
y de atlas para 200 frames estables de cada uno, incluso al renovar secuencia
o época. Cambiar píxeles/cantidad actualiza la textura; cambiar tamaño/material
la recrea. Se prueban fallos durante la actualización de una caché existente,
liberación antes de Reset/último Release (incluida referencia nativa del recurso),
restauración de estado, F8/caducidad y generaciones. Las capturas de estado son
temporales; la caché solo retiene recursos propios de tamaño acotado.

Colisión usa la lectura real de prefijo v6/3168, mantiene seqlock/permisos y deja
intacta toda la región de UV/píxeles. No repite la validación del barrido ni hace
barrido para un pie sin movimiento. Se mantienen las pruebas reales de callbacks
y 1999 pasos con presión, fallos temporales y referencia segura. Build427 se
reconoce en host y lectores. Control/escena/HUD no cambian sus ABI.

Resultado:104 Python,11 fixtures C bajo ASan/UBSan, cuatro suites C portátiles
y los fixtures PowerShell. JAR/API/instaladores/VIEW idénticos a 0.4.26 y comprobados
al empaquetar. No se recompila ni cambia Minecraft. **No se mide aquí FPS real,
tiempo GPU ni arranque de RE4; la mejora de rendimiento en Windows está pendiente.**

## 0.4.28

El usuario confirma lentitud de RE4 solo con DLL0.4.27, sin Minecraft/host/VIEW.
Su captura identifica build427 y dibujo inactivo, sin fallos de dibujo/restore;
174 fallos acumulados de Present no incluyen tiempos ni el HRESULT correspondiente.

Se comparan cuerpos reales de callbacks publicados en0.4.27 y corregidos con
la misma API simulada:10000 proyecciones desarmadas hacen40000 consultas de
memoria,20000 al jugador y10000 lecturas de control/tecla/barrera/diagnóstico
en0.4.27; cero de esas operaciones en0.4.28. Sigue llamándose la función original.
Restauración ocurre una vez;1000 cámaras/pads inactivos no vuelven a consultar
memoria del actor. Pasan todas las comprobaciones activas previas de mirada,
generación/F8 y restauración. Dibujo/HUD inactivos10000 veces no leen buffers
grandes ni asignan GPU/heap. Cabecera32byte real rechaza estado impar/desactivado.

Passthrough119slots, incluyendo112 normales, verifica argumentos/resultados/
LastError sin pares COM nativos extra por llamada. Liberaciones reentrantes
deferidas sobreviven métodos anidados y una/dos referencias finales durante Reset;
recursos propios se liberan antes del Reset/última liberación nativa. La DLL
compilada mantiene separados los frames grandes de los renderizadores activos.

Resultado:104 Python,11 fixtures C con ASan/UBSan, cuatro suites C portátiles y
fixtures PowerShell. JAR/API/instaladores/VIEW byte-idénticos a0.4.26. No se ejecuta
RE4 ni un driver real; no se atribuye aún una recuperación de FPS al cambio.

## 0.4.29

Confirmación del usuario:0.4.28 va bien con RE4 solo y con Minecraft sin puente;
el coste aparece tras arm/primera persona. Sprint se transmite y aumenta algo la
velocidad, pero el movimiento sigue percibiéndose más lento que en RE4 normal.

Callbacks reales de cámara/modelos/armas y collider integrados en el fixture:
10000 reaplicaciones de una vista en la misma actualización no consultan memoria,
control/actor/barrera ni reconstruyen la cámara; reaplican matriz y bits si la
función original los sobrescribe. Nueva posición antes/después de la comprobación
de F8, FOV, entrada, propietario, frame,16ms,500ms y parada invalidan la caché.
2000 pasos tardíos contra una pared con collider real corrigen pies/vista sin
penetrar. Se conservan los1999 pasos con fallos temporales del fixture de barrera.

Medidor de movimiento: reloj monotónico, distanciaXZ/unidades y bloques por
segundo, KEY_RUN efectivo, contador PadRead (no FPS) y exclusión de contactos/
transición/desarmado. CLI ejecuta ambas fases con lector real y proceso simulado
sin atributoPID, señal/wait simulados y reporte temporal. Patrones de teclas
únicos/acotados; únicamente lecturas de proceso/IPC. No modifica velocidades.

Resultado:109 Python,11 fixtures C bajo ASan/UBSan, cuatro suites C portátiles
y PowerShell. DLL Win32 compilada; JAR/API/instaladores/VIEW sin cambios. No se
mide aquí GPU/RE4 ni se iguala todavía su locomoción a Minecraft: dos capturas
Windows comparables son el siguiente dato requerido para calibrarla.

## 0.4.30 — scene camera, air editor and read-only collision map

User reports0.4.29 greatly improves active FP performance. Both supplied4s
movement tests valid: running total native3.944/bridge3.805 blocks/s; moving-only
native4.158/bridge4.116, PadRead59.75/59.24 per second. Walking moving-only
1.717/1.574 and cadence59.74/52.25: short comparable observations, not GPU FPS.
No speed multiplier or native Y/velocity change added.

114 Python,25 core and30 Minecraft tests pass, no skips/errors.11 native fixtures
pass ASan/UBSan, four portable C suites and PowerShell installer fixtures pass.
Actual extracted C camera callbacks/overlay reader use simulated memory/Windows.
Tests cover absolute gaze after lost first-relative sample/idle body turn,
new control during an active scene without shifting its view, new scene uses
new gaze, coherent overlay view despite global-camera overwrite/temporary
restore, age100ms/generation/torn guards, prior16ms cache and2000 late wall steps,
HUD bit restoration without undoing foreign bits. Temporary editor floor uses
real block states with simulated world,400 cell movements keep121 cells,
unchanged point produces no further writes, unloaded/height/foreign builds/
claimed replacement glass/world switch/clear preserve user content.

Read-only map parser uses synthetic managers/vertices/polygons with actual
SDK layouts and scanner. Tests include world transforms, attributes, disabled
objects, ambiguous patterns, manager/vtable/count/index/NaN/object-race rejection,
real-size7.4MB executable section, JSON/OBJ/offline HTML emission and CLI.
No RE4/game driver/map executed in cloud. User needs to validate camera/HUD
appearance and provide real geometry export before shared map, slope slabs,
placement occupancy or native jump/weapon damage can be claimed.

## 0.4.31 — presentation click routing / SAT reference audit

User confirms blocks/placement/editor floor0.4.30; clarifies hotbar icons correct,
only window clipping.115 Python tests pass, no skips. PowerShell compiles actual
WindowsDesktop/Session/MouseRouter against fake desktop:400 real-message-format
build/break transitions retain focus, held left/right/middle/X release, capture
moves, wheel delta, modifiers, non-native target/foreign foreground/injected/
closed-reused window rejection, send/setup/cleanup failure, independent stop,
resize before arm and raise without resizing, pending-restoration retry. Actual
Return-ToNormal script executes against simulated desktop; WinForms script parses.
No real Windows hooks/GLFW/D3D presentation run in cloud. Native, Java, API,
control-host and installer bytes preserved; no recompilation of unchanged targets.

User JSON/OBJ/HTML consistency verified220 EAT triangles (59floor/85slope/76wall);
SAT18 matches rejected in0.4.30. New parser accepts1..64 identical verified manager
references and rejects differing destinations, logs each source/destination.
Five map tests include18 aliases plus one discrepant operand. Actual SAT operands
remain unknown: no completed map/import/placement occupancy or jump claim.

## 0.4.32 — cockpit class visibility / vanilla item hotbar

The supplied first-person screenshot disproves effective cockpit hiding in30/31;
DPF_COCKPIT lease removed. IDSystem display-off masks from pinned SDK replace it.
Actual extracted C callbacks cover three targeted classes, prior hidden bits,
foreign-bit preservation, invalid capacity, cached projection reapplication and
existing F8/TTL/model/camera restoration.11 native fixtures pass ASan/UBSan;
four portable C suites and PE32 native build complete.115 Python tests pass with
build432 reader compatibility; TCP accepts/returns capability31.25 core and32
Minecraft tests pass; Fabric remap and access widener validation complete.
Readback tests exercise nonzero buffer offset, vertical flip, RGBA/ARGB order,
premultiplied alpha/transparent pixels, input immutability and malformed dimensions.
The separate glint/base buffer regression submits real CPU-side vertex builders
concurrently, catching premature fallback flush without a GPU. No real OpenGL,
Windows or RE4 rendering executes here; these tests do not prove
native cockpit disappearance, hotbar appearance or GPU performance. User must
check FP native HUD disappearance, faithful items/overlays and F8 restoration.

New JSON/OBJ/HTML parse coherently: SAT manager0x7f9a7c and EAT0x7f9aa0 use separate
arrays/vtables; SAT18 references all agree.205+225=430 triangles:98 floor,109 slope,
223 wall. No extraction errors; complete refers to this bounded local collision
snapshot, not all visible geometry/all rooms. No map import/terrain voxelization/
placement occupancy/native jump claim. Geometry files remain user-local.

## 0.4.33 — modal vanilla UI / hand image / native selection / HUD session lease

User reports native cockpit visible despite32 signature support31. Own ID_UNIT
visibility leases use pinned SDK and documented HUDTweaks scope visibility bit;
leases persist through pad/camera updates and restore for stop/invalid FP/F8.
Actual C callback fixtures cover visible/already-hidden/prompt classes, native
callback order, foreign-bit restoration and previous10k active/dormant reuse guards.

119 Python,26 core/34 Minecraft tests pass without skipped tests.12 native
fixtures pass ASan/UBSan,4 portable C suites and Win32 DLL compile pass. New C
fixture executes real overlay renderer against simulated device:200 unchanged
frames reuse image/upload, black native-view selection, native telemetry,45
setter failures and state restoration, torn header/age/F8/gen/escape/reset cleanup.
Compressed IPC tests reject zip bombs, extra/truncated streams and wrong geometry
without writes; menu movement/old-mod rejection occurs before native arm.
CPU1366×768 codec exercises channels/alpha/size, latest-only encoder and queue
close. Gate retains inventory generation while focus/stop cancel permanently.

PowerShell compiles real WindowsDesktop/Session/MouseRouter with fake desktop:
both window leases/restoration,400 build clicks, menu movement sent without
consuming OS movement, no game raw duplication, modifiers/held release/failures
and legacy native-stop-only mapping contract. Installer/diagnostic fixtures pass.
Java remap/AW/strict versioned mixin compilation completes, but neither real GL
framebuffer rendering/mixin-driven inventory interaction nor Win32/GLFW/RE4 visuals
execute in this cloud. Do not infer actual cockpit suppression, faithful skin/UI
appearance, pointer alignment, FPS or latency from these CPU/API fixtures. User
must validate the full workflow and F8 restoration on the actual games.

New overlay mapping128+5760000 is independent; old80 control/96352 scene/32792 HUD
remain. JPEG/game resources/map exports are not bundled. Native view/FOV projects
MC-selected outline and helper-floor top; no native terrain raycast, map import,
vertical/jump physics or Minecraft damage to RE4 actors is added. Native DLL433
SHA256 f707036409bad6f7044baf49fdc29f790af8f654ff11617730aa350055968a11.

## 0.4.34 — resultados12 de octubre de2026

-27 pruebas bridge-core y39 Minecraft:0 fallos/errores/omitidas; JAR remapeado34.
-124 pruebas Python, incluyendo las12 fixtures nativas existentes y5 consultas
 de geometría nuevas. DLL434 compilada y4 suites C pasan (SHA en guía34).
-VIEW:10.000 movimientos de menú coalescidos a un envío; orden posición/clic,
 botones simultáneos, cursor OS, captura/liberación, rechazo de inputs ajenos,
 pérdida de foco, errores de envío y restauración pasan con desktop simulado.
-GPU simulada: saturación de tres buffers no espera/copia/reutiliza un pendiente;
 solo el más reciente completado se copia; resultados tardíos no retroceden.
-Encoder descarta ruido sobredimensionado y procesa el siguiente frame; TCP
 envía imagen nueva entre ticks, no mezcla menú/movimiento ni prolonga armado.
-La exportación real completa contiene430 triángulos SAT/EAT. Query vertical
 en la posición exportada da Y=-454.6412477 frente a Y=-454.6429443 de Leon.
-Estas pruebas no ejecutan una GPU, OpenGL/GLFW, Win32 o RE4 reales. La frecuencia
60Hz es un máximo de captura/transporte, no una medida del equipo del usuario.
 No hay importación física del mapa ni oclusión visual activadas en34.

-DLL434:1.000 lecturas con productor ocupado conservan textura/dibujo sin
 nuevas creaciones; un snapshot roto reutiliza solo el último validado. TTL de
 la imagen, cambio de generación, F8, stop y desarmado impiden dibujar caché
 y liberan recursos. Se mantienen las45 pruebas de fallo de setters/restauración.

## 0.4.35 (cloud, 2026-10-13 UTC)

- JDK21/Fabric1.21.1: core32 y Minecraft40 pruebas, compilación/remap completos.
- Python134 pruebas: incluye snapshots seqlock ocupados1000 veces, feed inactivo
  sin lecturas, presupuesto/incompletitud/edad y caché de firmas con validación
  de objetos. IPC profundidad: host escribe solo208..223, lector readonly sin
  escritura, ABI y finitud estrictos.
-14 fixtures C del capture/render, también ASan+UBSan: profundidad LH/RH,
  GX→D3D normal/inversa, FOV inválido, depth buffer/generación/F8/clipping
  y restauración. Build DLL PE32 con mingw y4 políticas nativas.
- VIEW sintético y loopback real: preparación asíncrona autenticada, respuesta
  acotada y desconexión al cancelar F8. Java prueba cancelación del future.
- Instalación/desinstalación/backups y diagnóstico PowerShell comprobados.
- No se ejecutó RE4, el driver D3D9 ni WinForms real en Windows. La oclusión,
  rendimiento y preparación en los juegos reales siguen pendientes del usuario.

## 0.4.36: regresión de visibilidad

Build Fabric/JDK21: core32 y Minecraft43 pruebas. Python138 pruebas.
15 fixtures C de renderer/captura con ASan+UBSan, incluyendo rechazo de
proyección con atlas Minecraft, Z/ZWRITE desactivados, restauración y
referencias liberadas, espera hasta pase final, F8/TTL/generación, escena
vacía y fallo de setter. DLL PE32 build436 compilada con políticas nativas.
Installer/diagnóstico verificados con DLL nueva. CAPTURE comprueba matrices
435/436 sin OpenProcess y sin escrituras en el mapping. Hit de construcción: reproducción del límite de distancia vanilla, punto
corregido y roundtrip real PacketByteBuf, caras y celdas negativas. Sin
ejecución del juego ni
prueba de GPU Windows; proyección observada del usuario validada en fixture, oclusión real pendiente.
Matriz GX4:3/viewport1366×768: superficies y contorno con focalX real,
near100/far≈1e6 y clipping de pantalla comprobados.

## 0.4.37

Build/tests Fabric: core33 y Minecraft47. Python138.15 fixtures C con ASan/
UBSan. Verificación de cubo frontal cuadrado en1366×768 desde GX4:3, Z y
clipping sin alterar planos; rayo desde ojos a cara superior para apilar;
poses/escalas/altura de cuadrícula y todos los vértices de dos bloques con
rotaciones frente al contorno. Contacto pequeño de suelo admitido y muro
rechazado. Instalación/desinstalación y checksum del paquete comprobados.
No hay driver D3D9/RE4 ejecutado en cloud; apariencia final Windows pendiente.

## 0.4.38

Core36/Minecraft49; Fabric build JDK21. Python138;15 fixtures C ASan/UBSan.
Suelo fraccionario hasta0.99bloques, candidato original en contacto clasificado,
pared rechazada sin cambio a voxel superior, pendientes recortadas a huella y
corte superior rechazado; calibración evita adoptar techo sobre el jugador;
helper por suelo nativo conserva construcciones. Sprint4.15unidades/ms tras
150ms sin graphics, barrido desde referencia segura, teletransporte rechazado,
parada persistente. Backend VIEW compilado, routing/focus/restauración y
preparación autenticada comprobados; Win32 hook/thread y GPU real no ejecutados.
DLL438 PE32 y paquete completo mod/host/VIEW/DLL con hashes verificados.

## 0.4.39

Core36/Minecraft51/Python139 y16 fixtures ASan/UBSan. Integración de cube_draw y
collision_tick:200frames sin vértices visibles con rechazos transitorios previos,
sprint contra pared y vuelta al viewport sin stop/penetración ni escrituras Y;
cámara inválida no habilita física. VIEW ejecuta funciones reales de preparación
sobre desktop fake: foco denegado no envía, espera no redimensiona/reintenta,
un clic activa una única petición y F8 restaura/cancela. Se comprueba el reset
noClip de PlayerEntity1.21.1 antes de LivingEntity.tick, lease por jugador exacto,
límites de edad/parada/cambio de dueño y separación cliente/servidor. Build Fabric
JDK21 y DLL439 PE32. Interacción/Win32/GPU real no ejecutados en cloud.
