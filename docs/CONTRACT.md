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

### Prueba visual0.4.11

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
