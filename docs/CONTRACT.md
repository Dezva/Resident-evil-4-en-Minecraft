# Puente v1: contrato del prototipo

## Orientacion opcional 0.4.1 (IPC nativo v2)

re4_control puede agregar look {enabled, first_person, total_yaw, pitch,
eye_height}. Flags booleanos; first_person requiere enabled. total_yaw finito
abs<=1e7 grados acumulados, pitch finito +/-85, altura finita abs 40..2500 (0.4.0 solo aceptaba hasta 250).
Ausencia/frame desarmado desactiva look. ACK agrega capabilities 0..3 (raton1,
camara2), camera_active booleano y contadores look_packets/camera_ticks.

IPC v2 conserva palabras 0..9 y agrega floats total_yaw/pitch/eye_height
en 10..12, flags13, arm_id14, capacidades15, yaw aplicado16, ticks camara17,
muestras look18 y camara activa19. 80 bytes; magic anterior, version2 y
seqlock sobre entrada extendida. Host acepta v1 solo para movimiento.
Una secuencia consume una muestra; delta maximo30 grados y diferencia pendiente.
Camara opt-in cambia resultado por frame de QuasiFPS::hitCheck sin teleport,
con FOV/roll originales. El rearme reinicia el acumulador.

## Control nativo (mod 0.3.5; compatible con host anterior)

Modo separado `re4_control`, solicitado con `/re4bridge controller` y host
`--control`. Solo acepta `type: re4_control` con `armed` booleano, `buttons`
entero no negativo con máscara permitida 0x4F (0x0F antes de 0.4.10)
y `arm_id` entero 0..2147483647, ademas de protocol/session/seq.
Armado requiere arm_id >0 y una nueva generacion para rearmar tras parar. Bits
1/2/4/8 = forward/back/right/left nativos; desde 0.4.10, bit64 = KEY_RUN.
No se permiten ataques, apuntado, recarga u otros bits. Correr exige capacidad4
de la DLL; host rechaza su solicitud con DLL anterior y exige nuevo arm.
`ack` incluye re4_sample y control_state {armed,stop,hook_ticks}. Stop 1=F8,
2=TTL expirado. La sesion arranca desarmada y desarma al cerrar o fallar.

El host 0.3.5 agrega reason, requested_buttons y native_buttons como campos
opcionales para observar solicitud y aplicacion. native_buttons es mascara
0x4F (0x0F en versiones anteriores) y reason texto de hasta 160 caracteres. El mod tolera su ausencia en
hosts previos, pero necesita el nuevo host para el detalle del indicador.

MC publica teclas hasta 20 Hz solo al activar `/re4bridge arm`, con mundo local,
ventana enfocada, sin UI/pausa y con muestra RE4 reciente. El worker nunca
repite input de mas de 200 ms: envia desarmado en su lugar. Abrir UI/perder foco
desarma y exige arm explicito. Native IPC usa TTL 500 ms y seqlock: ver
native/README.md. El modulo llama a PadRead original, despues reemplaza solo los
cuatro bits de movimiento y, desde0.4.10, KEY_RUN en el hilo del juego.
W+S y A+D cancelan su eje; sin dirección restante también se retira KEY_RUN.
FP transforma dirección relativa a mirada en avance nativo manteniendo correr.
Capacidades: ratón1, cámara2, correr4. IPC continúa siendo v2/80bytes.
No teletransporta RE4.
Control y follow se excluyen para no crear autoridades circulares. Este modo
aun no iguala las posiciones ni camaras de los dos juegos.

Este contrato cubre el simulador y el lector externo RE4 con Minecraft Java
1.21.1. El canal de lectura no controla el renderizador ni escribe memoria RE4.

## Autoridad y alcance

En modo simulador, Minecraft controla el movimiento, inventario y simulación del jugador. El cliente
envía su pose; el servidor integrado toma las formas de colisión de su mundo.
El receptor de esta versión solo confirma y registra información. No controla
Minecraft, no edita el mundo y no aplica daño en RE4.

Solo se admite un mundo local de un jugador. La conexión se solicita con
`/re4bridge connect` para simulador o `/re4bridge re4` para el lector real,
y se cierra al salir del mundo. El cliente activa primera
persona al conectar y restaura la perspectiva anterior al desconectar o fallar.

## Transporte y ciclo de vida

- TCP, IPv4, `127.0.0.1:28741` por defecto. Sin puertos externos.
- Cada trama es un objeto JSON UTF-8 seguido de LF. Máximo 524288 bytes sin LF.
- `protocol: 1` en todas las tramas. El tipo es `type`.
- El cliente genera un UUID `session` nuevo para cada conexión.
- `hello` lleva `token`, leído de `config/re4bridge.json` de la instancia.
  El archivo se genera localmente al iniciar el mod; no se distribuye ni registra.
- El receptor responde `welcome` con la misma sesion y `mode: "simulator"` o
  `mode: "re4_reader"`. El cliente exige el modo que solicito al conectar.
- Tras el saludo, el cliente asigna `seq` creciente desde 1 a cada trama y espera
  un `ack` con la misma sesión y secuencia antes de enviar la siguiente.
- Timeout de conexión: 1.5 s; respuesta: 2.5 s; sesión ociosa en el simulador: 3 s.
  El cliente manda `heartbeat` cada segundo cuando no tiene muestras, incluso
  mientras el mundo está pausado. No hay reconexión automática: usar `connect`.
- Dos buzones de último valor, pose y colisión, con turnos alternados. Se
  descartan muestras pendientes antiguas si llega otra del mismo tipo. No hay
  cola creciente ni trabajo de red en los hilos de Minecraft.
- `seq` ordena envíos; `sample_ns` viene del reloj monotónico de la JVM. No se
  compara con el reloj del receptor. Pose y colisión se muestrean en hilos y
  momentos distintos; no son una captura atómica ni sincronía por fotograma.

Este canal es de diagnóstico. No transporta imágenes, profundidad, meshes,
sonido ni eventos de combate. El protocolo del puente nativo necesitará estos
canales y sus medidas de latencia antes de afirmar que la cámara está sincronizada.

## Lectura RE4 (mod 0.2.0)

En modo `re4_reader`, el cliente envia solamente `type: "re4_poll"`, con
`protocol`, `session` y `seq`, hasta 10 Hz, incluso durante la pausa. No envia
poses ni colisiones del mundo Minecraft. El host autentica y valida secuencia
antes de consultar memoria; responde `ack` con un objeto `re4_sample`:

```json
{"protocol":1,"type":"ack","session":"00000000-0000-0000-0000-000000000001","seq":1,"re4_sample":{"state":"player","position_raw":[100,-200,300],"angles_raw":[0,1.5,0]}}
```

`state` puede ser `player`, `waiting_player`, `transition` o `unavailable`.
Los tres ultimos invalidan la ultima posicion. En `player`, ambos vectores
deben tener tres numeros finitos con magnitud <= 100000000. La edad se mide
desde la recepcion con el reloj JVM; caduca tras un segundo. No se compara el
reloj Python con el Java. El host no acepta ordenes de escritura ni estado del
mundo Minecraft en este modo.

`/re4bridge calibrate` ancla ambas posiciones y el yaw de Minecraft al angulo
Y del personaje RE4. Conversion relativa:
Xmc = Xmc0 + (Xre4-Xre40)/escala; Z equivalente;
Ymc = Ymc0 + ysign*(Yre4-Yre40)/escala;
yawMC = yawMC0 + turnsign*grados(anguloY-anguloY0), normalizado a [-180,180).
Escala inicial 100 unidades/bloque, Y invertida y giro positivo son ajustes
provisionales, no medidas verificadas. No se lee pitch de la camara RE4.

El seguimiento se activa aparte con `/re4bridge follow`, requiere espectador
y aplica teleports al jugador del servidor integrado en su hilo, hasta 10 Hz.
Comprueba conexion, dimension, calibracion vigente y frescura antes de moverlo.
No edita bloques. No prueba colisiones fisicas. Desconexion/cambio de dimension
invalida calibracion; un salto por eje de mas de 64 bloques detiene seguimiento.
Los ajustes de calibracion son locales a esta sesion de Minecraft.

## Pose

`type: "pose"` contiene `sample_ns`, `dimension`, `feet: [x,y,z]`,
`eye: [x,y,z]`, `yaw_deg`, `pitch_deg`. Se publica hasta una vez por tick del
cliente (aproximadamente 20 Hz sin pausa), limitado por las confirmaciones.

Coordenadas nativas de Minecraft: +Y arriba, +X este, +Z sur. Un bloque es la
unidad espacial; yaw 0 mira hacia +Z y yaw 90 hacia -X; pitch positivo mira abajo.
La pose es del jugador, no incluye modificaciones de cámara de otros mods.
El eye es la posición de los ojos y feet la de los pies.
No se asume una escala, origen o cambio de ejes para RE4. Eso debe medirse en la
versión real 1.1.0, junto con sus matrices y el campo de visión.

## Colisión

### Bloque visual 0.4.19 / cliente y host 0.4.20

El frame `re4_control` incluye opcionalmente `preview: {bounds:[minX,minY,minZ,maxX,maxY,maxZ]}`,
con los mismos límites de caja 40..2500 y coordenadas finitas ±20000000.
`blocks on` captura la última colocación confirmada de BlockItem en el mundo local;
la caja queda delante de Leon, sin mapear todavía la cuadrícula Minecraft.
Romper/sustituir el bloque o cambiar de dimensión elimina la selección. Un intento
sin cambio de estado no crea caja. `blocktest` permite seleccionarla manualmente.

El host anuncia `control_state.preview_available: true` y devuelve `preview_state`
con status (1=canal listo, 2=dibujo detenido, 3=sin dispositivo), contadores uint32,
HRESULT y motivo. Listo no implica un dibujo visible ni una autorización vigente.
Los clientes anteriores pueden ignorar la capacidad adicional; el mod 0.4.19
requiere esa capacidad para activar blocks/blocktest.

En 0.4.20 el tamaño inicial de blocks on/blocktest sin argumento es 1000
unidades, configurable 40..2500. Es provisional, no una calibración de la
cuadrícula compartida. Se capturan intentos antes del uso del ítem mediante
START_CLIENT_TICK y UseBlockCallback, con cola de hasta 16 probes de diez ticks.
Duplicados no reinician el plazo ni cambian el anclaje. La observación puede
ocurrir desarmada, pero tanto cliente como host excluyen la orden de dibujo
hasta arm. La selección sigue siendo una sola caja.

preview_state añade requested (bool, orden publicada), bounds (seis números o
null), skipped_passes, camera_rejections y lease_rejections (uint32). El cliente
valida requested, skipped_passes y reason (cadena de hasta 160 caracteres).
Ninguno acredita visibilidad; el estado de última escena puede ser antiguo.
DLL/build sigue siendo 419 y su ABI no cambia.

El host escribe **CubeCommand v2, 56 bytes**, en `Local\RE4Bridge-CubeCommand-PID`:
8 uint32 `magic=0x52453454, version=2, sequence, stamp, enabled, epoch, mode, reserved`,
seguidos por 6 float bounds. `mode=0` es el cubo diagnóstico anclado a la cámara;
`mode=1` usa bounds explícitos fijos. `reserved=0`, enabled 0/1, epoch no nulo al
activar. Secuencia impar/par, snapshot consistente, TTL 500 ms y F8 tienen prioridad.
El mutex `Local\RE4Bridge-CubeWriter-PID` excluye host/prueba simultáneos. Nueva arm
o cambio de bounds cambia epoch; quitar/rearmar la misma selección no recalcula sus bounds.
Un fallo gráfico desarma; fallo de restauración bloquea hasta reiniciar RE4.

CubeTelemetry conserva version 1, 92 bytes, build 419 y solo lectura desde el host;
CaptureIPC conserva v3, 3296 bytes. El control de teclas mantiene IPC v2/80 bytes.
El dispositivo proviene de CreateDevice y se envuelve sin alterar su objeto o tabla
nativos. Se conserva el dibujo/restauración de estados de 0.4.18. El canal retirado
BlockPreview-PID no se abre ni se utiliza en esta versión.

### Materiales y barrera0.4.24

Preview escena añade style3, collisions bool opcional(defaultfalse), tiles y faces.
Style3 exige1..64tiles: stringsbase64 estrictos1368chars ->1024bytes ARGB little
endian16×16. faces tiene una entrada por caja, seis caras por entrada, cada una
[tile,u0,v0,u1,v1,u2,v2,u3,v3]. Tileuint0..n-1 y UV finitas0..1. Caras en orden
-X,+X,-Y,+Y,-Z,+Z; corners(0,0),(1,0),(1,1),(0,1) sobre ejesu=(axis+1)%3,
v=(axis+2)%3. Style0..2no acepta tiles/faces. Límites/origen/bounds previos.

Host anuncia materials_available para(v6,424); collision_available requiere además
mapping de métricas válido. Petición incompatible desarma antes de escribir input.
Scene lease publica atomizado estilo/paleta/caras/colisión y borra sobrantes. Epoch
cambia también al cambiar materiales/flag físico. En disarm todo se desactiva.

CubeCommandv6 mide96352bytes. Primeros3152conservados. Offset3152:4uint32 para
collisions,arm_generation,tile_count,reserved. Offset3168:128x6BlockFaceMaterial36
bytes(tile,uint32 +8UVfloat). Offset30816:64x256ARGBuint32. Reserved0, flagsbool,
tilecount0..64 y mayor0paraStyle3; coll exige mode2 ygen1..2147483647. Native
valida escena/materiales antes de dibujar/corregir. Host exige canales nuevos
para materiales/colisión. Capturev3/3296,CubeTelemetryv1/92,build424.

Local\RE4Bridge-Collision-PID es telemetría readonly48bytes:magic0x52453456,v1,
sequence,build424,pid,ticks,contacts,corrections,rejections,reason,active,generation.
Reason0activo,1off,2nueva referencia,3dato/memoria inválido,4salto de pose,
5cambio antes de escribir. Secuencia consistente; countersuint32. active es última
observación, no lease. Host falla cerrado ante3/5de generación actual; rechazo
antiguo no impide nuevoarm. ACKpreview_state incluye collision_state y requestflag.

Materiales desde PNG activos/modelos del cliente:16×16/primer frame/tintes/capas,
UV por quad. Recurso ausente/capacidad produce fallbackmagenta ycontador visible.
Atlas256×256 con celdas32y bordes,43setters/alpha, clipping conUV/rhw, un draw y
estado/recursos restaurados. No imágenes en ZIP, ni malla completa/profundidad RE4.

Barrera opt-in: huella cuadrada radio250/altura1600, barrido XZ yslide hasta4contactos;
ignora caja ya ocupada para salir. ConservaY. Pose previa ligada a jugador/gen/origen;
pausa>100ms/delta>500reinicia, sin arrastre. Solo con renderer/control/scene vigentes;
F8/generación/stale/desarmado dominan. SRWtry-lock evita reentrada; pointer/writable/
pose/nuevoF8/TTL comprobados antes de corregir X/Z. No se registran colliders motor,
no se modifican Y/velocidad/otrosactores y no hay apoyo vertical/saltos.

### Textura visual0.4.23

style acepta entero estricto0(wire),1(solid),2(stone). Host anuncia texture_available
bool solo con CubeCommandv5/build423. Un style2 incompatible desarma antes de
publicar input y desactiva el lease; NativePreview/CubeSession rechazan la petición
antes de modificar el mapping. surface_available admitev4/v5, scene_availablev3+.
Cliente valida preview_state.style0..2. No se amplía el payload ni se envían imágenes.

CubeCommandv5 conserva3152bytes/offsets; mode2 admite style0..2; mode0/1 requieren
count0/style0. Headerreserved0, cantidad1..128, secuencia consistente, TTL500ms,
F8/mutex/borra-slots y límites previos. Reader nativo exigev5. Host acepta parejas
(v2,419),(v3,421),(v4,422),(v5,423) y capacidades según canal, con build423 en
CubeTelemetryv1/92 y CaptureIPCv3/3296. Requiere nuevo JAR/host/DLL de0.4.23.

Solid/stone usan vertices28bytes XYZRHW/diffuse/UV; stone añade TEX1. UV0..1 por
cara interpoladas durante clipping, rhw=1/depth para perspectiva. Se mantiene
orden aproximado de caras y sin profundidad RE4. Stone genera tesela propia
16×16 A8R8G8B8, point filtering/clamp/sinmips, sombra multiplicativa. Se aplica
al mismo material provisional en todas las formas y puede estirar losas; no
exporta materiales, resourcepacks ni texturas originales. Mesh463872bytes.

La textura transitoria se crea después de respaldo/captura dentro de la escena
visible, lock DISCARD, pitch64..65536 válido/pBits presente, copia16filas, unlock.
Error libera recurso; Release tras Apply y liberación de scratch antes de retornar.
30setters para wire/solid y38 para stone, SBT_ALL; fallos de restauración latchean
fatal. No recurso GPU propio persistente ni cambios de creación/cámara/control.
Crear una textura por dibujo tiene coste de driver pendiente de comprobación.

### Caras visuales 0.4.22

Preview de escena acepta style opcional, entero estricto0(wire)/1(solid); ausencia
significa0. Los mismos límites y coordenadas de escena0.4.21 se mantienen.
Host anuncia surface_available bool solo para CubeCommandv4/build422; un solid
sin esa capacidad desarma antes de publicar input y desactiva el dibujo. Cliente
valida ACK preview_state.style0..1. scene_available también admite v4/422.

CubeCommandv4 conserva3152bytes y offsets de v3; style(uint32 offset76) reemplaza
scene_reserved. mode2 permite style0/1, mode0/1 requieren count0/style0; header
reserved(offset28) sigue0. Reader nativo exige v4. Host admite(v2,419), (v3,421)
y(v4,422) y rechaza solid en v2/v3 antes de modificar el mapping. Secuencia,
TTL500ms, mutex/F8, snapshot completo y borrado de slots mantienen sus reglas.

Solid genera caras grises opacas: culling según ojo local, descarte de interiores
completamente cubiertas, orden aproximado de lejos a cerca y recorte de quads
contra near1 y cuatro planos de frustum. No usa profundidad del escenario RE4.
Hasta768caras/16128vértices en scratch CPU temporal334848bytes; una llamada
TRIANGLELIST con vértices transformados. Wire conserva LINELIST.24 setters,
SBT_ALL/Capture/Apply/Release y liberación de scratch dentro de cada llamada;
fallos de restauración bloquean nuevas activaciones hasta reinicio. No modifica
factory/creación/device/hook de cámara ni crea recursos GPU persistentes.
CubeTelemetryv1/92 y CaptureIPCv3/3296 conservados, build422. Superficies no
implican materiales/texturas, detección de suelo ni colisión física.

### Escena visual 0.4.21

Preview puede usar exclusivamente `{origin:[x,y,z,yaw],boxes:[[minX,minY,minZ,maxX,maxY,maxZ],...]}`.
Origin xyz finito±20000000, yaw±π, lista1..128, bounds locales finitos±20000000,
extensiones por eje1..7500. No se mezcla con preview.bounds del cubo único.
El origen corresponde al centro horizontal/base del primer bloque delante de
Leon; los demás bounds son coordenadas MC relativas escaladas. Giro rígido
θ=-(yawRE4+yawMC) normalizado, local-to-world X=c*x-s*z,Z=s*x+c*z,Y=y.
La primera referencia permanece tras retirada; origin cambia solo mediante
height, nueva blocks on o cambio/deselección de escena. No se fija de nuevo al
mover cámara o Leon. No mapea el suelo, no altera física ni guarda en disco.

Cliente captura hasta64 bloques/128 cajas,16 formas por bloque, comprueba
estado/formas de celdas cargadas y retira aire/sustitución. Celdas descargadas
no se consideran aire. Losas dobles y vecinos actualizan formas; overflow
posterior retira escena con aviso. Mobs/estructuras/suelo no se exportan.
Host anuncia scene_available bool solo con DLLv3/build421. preview_state añade
box_count uint1..128 o0 desactivado y origin/cantidad al diagnóstico; el host
no devuelve la lista completa en cada ack. requested no acredita dibujo visible.

CubeCommand v3 mide3152 bytes: header8uint32 previo, bounds6float(32..55),
origin4float(56..71), count/scene_reserved(72..79), boxes128x6float(80..3151).
mode0/1 requieren count0; mode2 requiere count1..128; ambos reservados0.
Secuencia consistente/TTL500ms/F8/mutex anteriores. Host publica snapshot
completo y borra slots sobrantes. El renderer valida toda la escena, compone
camera*transform rígido, proyecta hasta3072 vértices y usa un respaldo y una
llamada DrawPrimitiveUP. Estado y referencias se restauran; sin recurso GPU
persistente, Z/occlusion siguen desactivados para diagnóstico de aristas.
Telemetría de92bytes y CaptureIPC de3296 conservan sus versiones, build421.
Host puede leer DLL419/v2 para único; escena en v2 se rechaza sin escribir.

### Prueba visual 0.4.11 — retirada, contrato histórico

El frame `re4_control` puede incluir `preview: {bounds:[minX,minY,minZ,maxX,maxY,maxZ]}`.
Es una única caja en unidades nativas RE4, finita, dentro de±20000000 y con
tamaños por eje40..2500. No es `collision_snapshot` ni una entidad con colisión.
El mod fija esa caja explícitamente delante de Leon mediante blocktest,
mantiene el bloque Minecraft seleccionado y retira la caja si se rompe,
cambia mundo/dimensión o se desconecta. No envía el superplano completo.

El host opcionalmente devuelve `control_state.preview_state` con status0..4
(pendiente/listo/sonda fallida/no hallado/ambiguo), frames, draws, failures y
last_hresult. Los contadores son uint32 y no forman parte del control.
La memoria independiente `Local\RE4Bridge-BlockPreview-PID` mide68bytes:
magic0x52453442,version1,sequence,stamp,enabled,generation,6floats de caja;
después status,frames,draws,failures,last_hresult. Host publica únicamente
los campos de control con secuencia impar/par; DLL escribe solo contadores.
Dibujo exige controles armados/frescos y misma generación, caja completa y
fresca (TTL500ms), sin stop. La ausencia de preview desactiva el dibujo.
F8/stop/desarmado tienen prioridad. No afecta el IPC principal80bytes/v2.

Las líneas se proyectan con la cámara actual y no usan profundidad del terreno.
Los estados gráficos se restauran antes de EndScene original. Esta prueba no
demuestra todavía conversión espacial general, texturas o colisión de bloques.

`type: "collision_snapshot"` contiene `sample_ns`, `dimension`, `center`,
`complete` y `blocks`. El área es de 9 × 5 × 9 bloques alrededor del centro
entero (radios X/Z = 4, Y = 2), aproximadamente 4 Hz. Solo se consultan chunks
cargados, sin forzar generación. Cada instantánea reemplaza la anterior.

Cada bloque no aire contiene:

- `pos: [x,y,z]`: posición entera absoluta en el mundo Minecraft.
- `state`: representación diagnóstica del estado, incluidas propiedades.
- `boxes`: lista de cajas `[minX,minY,minZ,maxX,maxY,maxZ]` relativas al bloque.
  Para obtener coordenadas de mundo, sumar `pos` a cada extremo.
- `collision_complete`: indica si se enviaron todas las cajas del bloque.

Se usa `VoxelShape` del estado con `ShapeContext` del jugador del servidor.
Una losa inferior suele ocupar Y = 0..0.5; una superior, 0.5..1. No se convierte
todo en cubos enteros. Las cajas pueden salir del bloque (por ejemplo, vallas).
Formas con más de 256 cajas se marcan incompletas y no se truncan silenciosamente.
`complete` es falso si falta algún chunk o forma. Una región no recibida o
incompleta nunca demuestra espacio vacío ni suelo seguro.

La instantánea **no** incluye entidades, geometría RE4, fluidos físicos,
navegación de NPC, cambios fuera del radio, pendientes trianguladas ni huecos
excavados en RE4. La colisión RE4 → Minecraft y Minecraft → RE4 sigue pendiente.

## Ejemplo

```json
{"protocol":1,"type":"collision_snapshot","session":"00000000-0000-0000-0000-000000000001","seq":2,"sample_ns":100,"dimension":"minecraft:overworld","center":[0,64,0],"complete":true,"blocks":[{"pos":[0,63,0],"state":"Block{minecraft:stone_slab}[type=bottom,waterlogged=false]","collision_complete":true,"boxes":[[0,0,0,1,0.5,1]]}]}
```

## Criterios de prueba

Las pruebas automatizadas ejercitan el transporte Java ↔ Python real por TCP,
autenticación, sesiones, secuencias, tamaños, números finitos y sustitución de
instantáneas. No prueban la geometría en el motor de Minecraft ni RE4.
La prueba manual debe confirmar que moverse cambia `feet`, colocar/quitar bloques
cambia `blocks`, y que losas superior/inferior producen las cajas esperadas. La
etapa nativa exigirá además evidencia dentro de RE4 de renderizado, colisiones
en ambos sentidos y daño atribuible a eventos únicos.


## Extension0.4.26

Control TCP puede incluir hud={image:Base64} solo armado y con look.first_person.
Imagen exactamente256x32 ARGB little-endian (32768 bytes /43692 caracteres). ACK
hud_available boolean estricto. Campo opcional mantiene compatibilidad de frames.
Local\\RE4Bridge-HudCommand-PID v1/32792: magic0x52453448,version,sequence,stamp,
enabled,generation (24 bytes),8192 uint32 pixels. Escritura odd/body/even, enabled
0/1; gen1..INT_MAX para activo, frescura500ms, mismo gen control80/v2 armado/FP,
F8, facade válida, backbuffer, BeginScene, reset/present/escape. HUD no activa
CubeTelemetry.active. Shutdown/disarm borra enabled,gen,pixels. Un host controla
ambos canales bajo el mutex CubeWriter existente. APIs de captura/render conservadas.
Build426 con escena v6/96352; lectores reconocen par426/v6 y colisión426/v1.

## 0.4.30 — mapped gaze and coherent render view

Capability8 supports optional look.absolute_yaw (strict bool, defaultfalse).
Only enabled FP may set it; total_yaw is then the mapped angle in degrees
[-180,180] whose negative is native yaw, not a relative accumulated delta.
Native flag4 joins mouse1/FP2; Control v2 remains80 bytes. Old relative mode
is retained. New Java reader accepts capabilities0..15; old host/DLL cannot
activate absolute mode without capability8. No additional input buttons.

An internal seqlocked final view (same actor/generation, age<100ms, original
control TTL) supplies blocks in FP. One active D3D scene freezes its view;
current collision corrections remain independent. No orbit-view fallback in FP.
The DPF_COCKPIT0x10000 Global+0x58 attempt was ineffective in user testing
and is withdrawn in0.4.32;
health/ammo/inventory values are never modified.

The temporary Minecraft editor floor is local creative only,121 air cells max,
server-executor owned; it is excluded from native scenes and restored on normal
stop/disconnect. User changes, including replacement glass after breaking a
helper, relinquish ownership. Editor origin is fixed in air above existing
terrain. This is not a terrain collision import or native jump implementation.

EXPORT-RE4-MAP has no write/IPC/native-call capability. Version/unique signatures
and validated SAT/EAT manager/cSat structures gate bounded extraction; errors
mark incomplete data, never assume empty space. JSON/OBJ/HTML are user-local,
not published game meshes. Default local radius20000native units. Shared map,
slab quantization, placement ray/occupancy, jump/support and combat remain
unimplemented until actual geometry is validated.


### 0.4.32 cockpit classes and vanilla hotbar capture

Control capability16 signals uniquely validated module-static IDSystem+0x30
visibility access. Only display-off class bits0x21/0x32/0x34 are borrowed/restored;
no global display flag/HP/ammo changes, ID deletion or HUD vtable patch. Existing
FP/F8/TTL/owner guards apply; client accepts capabilities0..31. If validation
fails, FP can still work and status reports native HUD hiding unavailable.

Minecraft's own hotbar render runs only from HudRenderCallback onto a transparent
256x32 target, with vanilla GUI models/overlays, custom DrawContext dimensions,
private vertex allocator, saved/restored GL/render state and invalidated vertex
binding cache. Readback is requested from ticks and reused when static; animated
items refresh at most10Hz. Selection/items/offhand/resources trigger replacement.
F1 returns no HUD. A capture failure restores state, disables only this layer and
reports an error; F8 still restores RE4. No game image is bundled. This is the
hotbar, not the E inventory screen/health/hunger or Minecraft combat implementation.
Real GL/Windows appearance/performance require manual validation. HUD ABI stays
v1/32792, RGBA bottom-up conversion produces top-down straight ARGB pixels.
