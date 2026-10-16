# 0.4.57-impactfix2 — extensiones compatibles de geometría/telemetría

No cambia ningún tamaño ni versión de IPC existente: ABI DLL457, Combat4,
Runtime5, WorldMesh1. Actualizar host/DLL/JAR del paquete juntos.
TerrainSnapshot admite4096 triángulos completos. Radio deseado32000 unidades,
presupuesto320000 bytes JSON de triángulos/tipos; si no cabe se reduce el radio
declarado conservando todo triángulo cuyo AABB cruce el volumen. No se presenta
un subconjunto arbitrario como terreno completo. Caducidad1s, refresco0.5s y
worker único se conservan; fuera del volumen se pausa/oculta el proyectil.

Combat kind1 usa slot/GUID como candidato conocido del catálogo. El rayo nativo
elige el actor real, que debe seguir presente en ese mismo catálogo/pool, y su
parte validada. Se publican el slot/GUID/posición del impacto real. Kind0 mantiene
selección estricta anterior. SAT/EAT de construcción no veta antes el impacto
contra su propio objeto destructible; RE4 y los bloques colocados siguen
autorizando la colisión. Reason2 sin mensaje nativo permite continuar un paso
con colisión de terreno; no retira el ID de proyectil. Encolar un mensaje retira
ese ID antes de llamar al emisor, con historial/floor acotado, confirmación y
TTL previos. Los demás rechazos/timeouts retiran la flecha en el cliente.

Canal opcional Local\\RE4Bridge-GpuDepth-PID,128 bytes, propiedad nativa,
lectura externa FILE_MAP_READ exclusivamente. Header cuatro u32 magic0x52453447,
version1,build457,PID. Sequence16 cubre el reporte completo: stamp20,reason24
(0recuperado,1shader no reconocido/consulta inválida,2constantes no disponibles,
3matriz no válida),compare28,shader_words32,WVP16floats36,A100,B104,near108,far112,
samples116,recovered120,reserved124. No contiene comandos ni punteros.
Depth.source4 identifica proyección recuperada de WVP; source3 conserva la
proyección aislada. Reconocedor VS3 lineal, shader<=16384 bytes, una consulta
por intervalo de profundidad/frame; sin GPU readback ni cambios de shader.
La relación Z.xyz=A*W.xyz y Z.w=A*W.w+B debe ser válida y coincidir con el sentido
de comparación. Estas comprobaciones no prueban por sí mismas la oclusión real.

# 0.4.57-depthfix1 — mismo IPC y JAR

Mantiene build457 en todos los canales y WorldMeshv1/Runtimev5/Combatv4.
La selección opaca solo acepta LESS/LESSEQUAL/GREATER/GREATEREQUAL, con cámara,
viewport no degenerado y proyección válidos antes de fijar depth_seen. Escaneo
GPU y reintentos de geometría acotados al intervalo/frame. Guardias500ms y F8
sin cambios; ningún nuevo acceso de memoria del juego.

Depth tail240/244 publica MinZ/MaxZ cuando tail252==0x44584631. Sin esa marca,
el lector omite esos campos (compatible con las DLL anteriores). Comando host
208..223 y el resto de offsets permanecen intactos. Son datos de diagnóstico;
no autorizan movimiento ni garantizan oclusión.

# 0.4.57 — geometría del mundo separada del HUD

WorldMesh: Local\\RE4Bridge-WorldMesh-PID, magic0x52453457/version1/build457,
77888bytes. Cabecera u32 magic0/version4/build8/PID12, command seqlock16,
stamp20/enabled24/generation28/quads32, siete reserved36..63=0.
Vertices64: máximo128 quads ×4 vértices ×24bytes (float xyzuv + ARGBu32,
little endian); atlas12352:128×128 ARGBu32. Lease500ms, sesión FP nativa válida,
sin escena/menú, F8 y restauración obligatoria. No admite punteros/comandos.

JSON opcional world_mesh={encoding:"world-quads-v1",vertices:base64,texture:base64}.
Vertices no vacíos, múltiplo96, máximo12288bytes; XYZ finitos ±20M, UV0..1.
Textura zlib máximo32768bytes comprimidos y exactamente65536 descomprimidos,
sin cola ni segundo stream. Se valida antes de cualquier publicación IPC.
Cliente solo envía un snapshot <200ms, de la misma generación y sin suspensión.
El host deshabilita en ausencia del campo, parada, menú o escena. Si añadir
la malla excede524288bytes, el emisor omite solo esa malla durante la trama;
conserva controles/escena/HUD y el límite estricto del protocolo.

Depth conserva256bytes/version1. Build457 admite stage2 (inserción tras draw
opaco) y source3 (matriz D3D validada en constantes GPU). Tail nativo224:
world_reason,world_quads,world_attempts,world_draws; el comando208..223 no cambia.
Los contadores describen envíos, nunca prueban visibilidad ni contenido del Z.
Runtime v5 y Combat v4 conservan ABI de456; lectores aceptan456/457 y el nuevo
canal WorldMesh exige457. Todos los canales de la DLL nueva anuncian457.

No se amplía la cobertura del terreno ni se relajan las guardias de daño.
La flecha pausada por cobertura desconocida se oculta; una flecha incrustada
en geometría conocida conserva su representación. Patada nativa sin cambios.

# Mod0.4.56-arrowfix1 — sin cambios IPC

La primera invocación del tick de una flecha vanilla ocurre con age1 porque
ambos World incrementan el contador antes de Entity.tick. El mod registra edad
0/1 una sola vez, con propietario/dimensión/sesión válidos; una primera
comprobación desarmada no puede registrar esa entidad más adelante.
Impacto y overlay usan la misma marca. Diagnóstico máximo128 eventos por sesión
en el log normal de Minecraft, sin tramas, tokens ni geometría.
Core0.4.56, host0.4.56 y DLL456 conservan contrato y bytes nativos anteriores.

# Cliente/host0.4.56 — flechas y acciones nativas

Combat v4/5332bytes/build456 conserva reportes/catálogo y comandos previos;
añade u32 kind5324 y projectile5328, cubiertos por request_sequence16. kind0
exige projectile0 y conserva origen a<=1 bloque del ojo; kind1 exige entidad
1..INT_MAX y segmento<=6 bloques con origen a<=64 bloques del ojo. No convierte
reach en alcance total del arco. Servidor Minecraft autoriza segmentos solo
en terreno conocido, valida propietario/dimensión/sesión y calcula daño.

JSON combat admite campos anteriores, o esos mismos más kind/projectile juntos;
sin extensión significa melee. scale/reach/damage/identidad/finite conservan sus
límites. El host escribe la extensión dentro del mismo seqlock. Una flecha se
consume por generación incluso si su impacto se rechaza. Historial128 más
marca inferior monotónica evita replay tras expulsión; IDs antiguos pueden
rechazarse conservadoramente. Catálogo sigue limitado a128, pool<=4096; consulta
de actores<=64*scale+2000, sin nuevas clases ni escrituras de HP/rutina.

Runtime v5/160bytes/build456: reserved[0]/offset152 reporta GLOBAL STATUS[2].
STA_PL_EM_ACTION0x40000000 devuelve scene1, restituyendo cuerpo/cámara nativos,
sin convertir el aviso STATUS[0]/0x4000 en escena. Se mantiene el resto del IPC,
incluidos diálogos, recuperación, F8 y generación. Todos los canales de esta
DLL anuncian456; Runtime/Combat exigen la pareja exacta. Host0.4.56 conserva
las firmas ABI corregidas y mensajes de error con cabecera observada.

No se transmiten texturas originales del juego en el paquete: render de flechas
usa modelos del Minecraft instalado. Efectos especiales de proyectiles y
retargeting del jugador no forman parte de este contrato.

# Histórico cliente/host0.4.55 — trampas, altura y renderizado

Combate v3/5324bytes y runtime v5/160bytes exigen build455. cEm2a se publica
solo con los perfiles de ambos manejadores verificados. Usa el mensaje nativo
existente y confirma HP0 + mensaje consumido + transición nativa, sin escrituras
de HP/rutina/sonido ni llamadas directas a manejadores de trampas. Identidad,
variante, alcance, raycast nativo, muros, sesión y TTL siguen siendo necesarios.
La caja aproximada incluye400unidades bajo el origen de trampas; el raycast
nativo valida la parte real. No amplía alcance ni aplica a colisiones de enemigos.
Trampas quedan excluidas de bajas y corazones; el daño a ganados sigue igual.

Minecraft devuelve min(límite vanilla,60) mientras el control activo en primera
persona está vigente, sin cambiar opciones persistidas. Al salir usa vanilla.
La guardia de desplazamiento de colisión solo mide X/Z; no modifica la Y nativa
ni relaja el TTL500ms o el presupuesto de movimiento horizontal.

## Histórico0.4.54 — sin cambios de DLL/IPC nativo

Se conserva build453/Combatv3/Runtimev5. Client input lease: LIVE<200ms,
SUSPENDED[200ms,1s) solo para una sesión previamente armada, EXPIRED>=1s.
SUSPENDED transmite armed=true/buttons0/menu=true, conserva look/escena/Y,
suprime acción/combate y extracción nueva de terreno. Nunca escribe un timestamp
nuevo al snapshot cliente. Un tick auténtico puede recuperar antes de1s; después
la política host exige nueva generación. El menú existente conserva su límite1s.

Opcional client={reason:string<=160 sin CR/LF,focused:bool,paused:bool,
screen:string<=32 sin CR/LF,sample_age_ms:int[-1,60000],tick_age_ms:int[-1,60000]}.
Se valida antes de IPC; es diagnóstico y no autoriza ninguna escritura. El host
lo devuelve junto con generation y registra transiciones inmediatamente. Si el
worker suspende/caduca por falta de ticks, sobrescribe solo su reason diagnóstico.
Consola usa un emisor asíncrono con32 mensajes, descarte de antiguos y shutdown
acotado; nunca bloquea el hilo del socket esperando una selección de consola.

Foco: callback HEAD onWindowFocusChanged publica pausa/0 antes de los cambios
vanilla. ControlArming conserva como máximo200ms una sesión activa ya confirmada;
registra la hora en callback para impedir recuperación tardía si no hubo ticks.
VIEW espera solo foreground nulo o ForegroundStaging de Explorer, máximo200ms;
ventanas ajenas, fallos de ratón, cierre y F8 mantienen la parada.

Contratos anteriores debajo.

# Build453: Combat v3 y Runtime v5

Combat: 5324 bytes, magic0x52453446/version3/build453/PID. Conserva offsets de
comando [16,80), seqlock80, catálogo136 y tail5256. Añade siete u32 en5296:
callback_seen/owned/thread_changed, last_native_flags, last_message,
callback_foreign, boxes_broken. target_kind pertenece al impacto, no al último
actor del catálogo. La identidad GUID/vtable/tipo, parte y origen del mensaje
siguen siendo obligatorios; el hilo emisor no lo es. Se reclama una sola vez
bajo bloqueo y se libera durante la llamada nativa. Solo al mensaje propio se
le quita flags bit0 (no letal); el resto de daño no cambia. Una caja0x53 que
llegue a HP0 espera además rutina (1,1), mensaje consumido y flags&0x603==1.
Se observa la transición intacta→rota; no se escribe HP/rutina/bandera del objeto.

Runtime: 160 bytes, magic0x52453452/version5/build453/PID. Conserva comando48..83,
feet_y112 y reportes previos. Reporte128: ocho u32 dialog_active,dialog_actions,
dialog_flags1,action_ack,action_generation,dialog_source,reserved[2]. El seqlock16
cubre el reporte. GLOBAL routine3/STATUS[1]&2 (STA_ITEM_GET) habilita recogida,
salvo movie/cut/death; lo mantiene como escena pausada1 para movimiento/combate.
action_kind:0 contextual,1 skip;2 arriba,3 abajo,4 izquierda,5 derecha,
6 aceptar(KEY_OK|KEY_A),7 cancelar(KEY_CANCEL|KEY_B). Solo recogida permite2..7.
Cada pulso tiene un trigger, hold120ms y restaura exclusivamente sus bits.
La cola cliente limita16 pendientes, edad1s, espera ack/generación e intervalo
150ms entre envíos y un comando cero durante al menos30ms antes del siguiente; timeout500ms, cierre de diálogo/menú/F8/generación la vacían.

Contratos históricos conservados debajo.

# Combat v2 / build 452

Combat mapping is 5296 bytes, magic 0x52453446/version2/build452/PID. The command
area [16,80) and report seqlock at80 retain their offsets; the 128 actor rows at136
remain 40 bytes. Added tail at5256: six u32 (engine_ready,queued,expired,hit_part,
hook_reason,pending_id), three float hit_position and u32 target_kind.

Requests are consumed once, separately from acknowledged completion. Reason9
means a native message is waiting; ack still names the preceding completed
request. A native ApplyDamage for the exact actor/GUID/part/source/thread may
replace the amount and variance (0), retaining flags/result. HP counters and
ack advance only on that callback. A pending request expires at500ms or loses
its control generation/menu/F8 lease; only its still-owned pending message can
be canceled. Slot reuse/foreign messages must retain ordinary native damage.

Native Bullet_RayTest supplies the actual hit-part pointer; that pointer must
belong to the target's bounded/cycle-checked YARARE list with valid bone, ray
point and reach. World geometry is authoritative in RE4. Placed-block occlusion
is rechecked against the actual point, not just the selection AABB.

Client effects require a confirmed HP reduction, are bounded to64 live vanilla
particles and render at the returned impact in the transparent overlay. Crow
and active Box/Rack/Window/Barrel types with valid HP join the existing whitelist;
this is not a general entity/object editor. Combat no longer requests a second
terrain capture. The host writes rotating error logs without authentication
frames/tokens or geometry dumps.

Historical channel contracts follow.

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
2=TTL expirado. Desde0.4.55 también3=referencia de colisión caducada,
4=desplazamiento horizontal fuera de rango,5=cambio de actor vertical,
6=altura fuera de rango,7=reinicio/fallo gráfico y8=retorno/parada de VIEW.
El campo informativo `stop_cause` y la consola muestran el motivo.
La sesion arranca desarmada y desarma al cerrar o fallar.

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

### 0.4.33 modal inventory / full GUI and hand overlay

Only InventoryScreen/CreativeInventoryScreen in active FP with available enabled
Minecraft interface are allowed to retain arm. Generation is unchanged; movement
buttons are zero while modal. Host rejects menu=true with any movement, unsupported
codec/backend and old HUD image sent to the new overlay backend. Other screens,
pause/focus loss/stale sample/stop still require explicit rearm. UI is rendered
by vanilla; existing E/click/drag/scroll callbacks act on actual MC inventory.
No game-mode switch or health/damage transfer is added for UI/hand rendering.

Overlay image fields: encoding=zlib-argb,width32..1600,height32..900,menu boolean,
base64 image<=262144 characters/196608 compressed bytes; inflation must terminate
at exactly width*height*4 with no trailing/second/truncated stream. Optional
selection[10] comprises rigid origin[xyz,yaw] and6 bounds, finite/range checked.
Menu suppresses selection. Local auth512KiB framing unchanged. Cached decoding
is bounded to2 images; stable image identity retains native revision/upload.

Independent Local/OverlayCommand mapping128+5760000, native header magic5245344f,
version1. Seqlocked writer fields are0..39/64..107; native draw telemetry40..55
reports actual viewport/last managed ID counts. Pixels begin128. CPU staging is
heap-only, D3D texture per facade; alpha/linear filtering and half-pixel alignment.
Native TTL/FP/actor/generation/backbuffer/scene/escape/Reset/F8 gates remain.
Readback GL/render state is restored; CPU encoder runs a latest-only daemon queue.
Captured frames older500ms are withheld. UI/hand capture capped20Hz, not native FPS.
Vanilla screen blur/alternate targets cannot affect Minecraft main frame while
capture active; original rendering remains unchanged outside that scope.

Native cockpit leases are session-long; camera/model leases remain per update.
Static manager, counts/bounded writable array, ID class/slot identity gate writes.
Only visible0x8 on cockpit primitives and their class display-off bits are borrowed;
existing hidden bits/other classes/bits are retained. At stop only matching slots
are restored. No HUD function/vtable hook or rectangle-based suppression.

VIEW also leases Minecraft bounds/style and restores both windows on F8/cleanup.
Cursor motion is forwarded only with fresh enabled menu IPC and original windows/
foreground verified, and is never consumed so Windows cursor can move. Raw game
motion remains unforwarded. Outline is mapped MC shape, not native terrain raycast;
helper floor top is thin outline. Native collision/support/map/combat unchanged.

## Extensiones35: preparación, superficies y profundidad

VIEW→mod: localhost puerto config.port+1 (65535→65534), JSONL máximo4096bytes,
`{protocol:1,type:"prepare",token:...}`. Solo esta acción; no comandos arbitrarios.
Respuesta `{protocol:1,ok:boolean,message:string}` tras confirmar cámara/control.
Cerrar conexión cancela el future; el cliente detiene autorización pendiente.

Control añade `terrain:boolean`, `terrain_revision:int`, `depth:boolean`.
ACK terrain incluye revision/generation/age_ms/complete/position/radius y filas
[ax,ay,az,bx,by,bz,cx,cy,cz], máximo1024. Snapshot incompleto no incluye filas.
Cada extremo valida finitud, región completa y generación; nunca se considera
libre una región desconocida. Importación de bloques del mapa no implementada.

Profundidad separada: mapping256bytes, magic0x52453444 versión1 build435.
Status nativo0..207 bajo seqlock offset8; comando host208seq/212stamp/216enabled/
220generation. Timestamp500ms máximo. No hay parche de proyección ni lectura
de buffers GPU para colisiones. El renderer usa el depth buffer real, estados
D3D respaldados y matrices validadas; omite sólidos en modo depth si no valida.

## Compatibilidad36

Mismo ABI depth256bytes; build435/436 admitidos por el lector. La palabra
reservada offset60 ahora cuenta dibujos sin profundidad. Build436 mantiene
razón2/3 cuando usa compatibilidad por matriz rechazada/buffer ausente;
`depth_draws` solo aumenta por dibujo con profundidad validada. Compatibilidad
se ejecuta al final de la escena y no omite gates F8/TTL/epoch/generación.

## Coherencia37

GX conserva matriz virtual en el diagnóstico; la matriz efectiva normaliza
focalX para el viewport sin cambiar Z. ControllerMapping.gridHeight alinea
la consulta nativa y la selección con la escena. Preparación usa ojos reales
de Steve, limitados por el rango nativo existente. Rayos de construcción
usando pose/orientación del tick, no interpolación de otro renderer. Contacto
de apoyo0.02bloques, máximo20% de altura de una forma; laterales/techo intactos.

## Terreno0.4.38

La respuesta `terrain` admite `kinds` paralelo a `triangles`, con igual cantidad,
valores floor/slope/wall/other. Ausente significa other: no se deduce suelo de
la normal de un raycast bidireccional. Solo suelo/pendiente con componente normal
vertical suficiente permite contacto en la base sin corte superior. Consultas
de flechas conservan todas las superficies. No se desplaza una colocación
rechazada a un voxel vecino; hay una sola referencia para suelo, hit y volumen.

## Preparación y pausa0.4.40

VIEW prepare admite game_mode opcional keep/survival/creative, con keep por
defecto. Valor textual exacto, acción fija autenticada; ACK incluye game_mode
solo después de completar la preparación. Los modos explícitos esperan
confirmación del GameMode local de Minecraft antes de solicitar armado.
ControlArming admite PAUSE solo en armado ya activo; no activa un arm pendiente
con menú abierto. Pausa/inventario emiten controles con buttons0, look y leases
vigentes. Foco, F8, muestras caducadas, timeout o parada nativa conservan sus
checks y no se rearman solos. Supervivencia usa vida/comida de Minecraft;
no se modifica HP ni el reloj de RE4. IPC/DLL build439 sin cambios.

## Vida/menús0.4.41

La restricción histórica de no modificar HP se sustituye para el enlace de vida
autorizado. HP máximos/munición/inventario RE4 no se modifican. Control80bytes
v2 mantiene tamaño; look_flags bit8 significa menú con botones0. validate_look
admite bits0..3 y preserva validación FP/gaze. TTL nativo500ms intacto.
Worker extiende edad únicamente del snapshot menu=true/buttons0 hasta1s;
movimiento normal200ms. setScreen publica ese snapshot antes de efectos GUI.

Wire health opcional tiene mode1(survival)/2(creative), ratio0..1, damage_ack
y healing_ack uint32. ACK health_state incluye capacidad/frescura/generación,
actor/HP/max, contadores normalizados (1e6=100%), muerte/modo/motivo.
IPC Health100bytes v1/build441: header0..15; request seqlock16, stamp20,
enabled24, generation28, mode32, float ratio36, ACKs40/44; report seqlock48,
stamp52, reason56, active60, generation64, player68, hp72, max76, damage80,
healing84, dead88, writes92, mode96. Dos escritores por regiones separadas.
Las escrituras de HP/rutina se realizan en game thread con snapshot/revalidación.

Servidor Minecraft aplica deltas una vez antes de publicar ratio/ACK. Un cambio
creativo/supervivencia descarta daño observado en creativo; F8 anula tasks y
petición. Cero vida fija ambos HP actuales y Die3 solo desde Move/Damage.
HP nativo0 nunca se revive; nueva generación permite un actor recargado vivo.
Muerte ya aplicada no mata al jugador que reaparece fuera del puente.

## Escena/acción/enemigos0.4.42

Runtime64bytes v1/build442: magic0/version4/build8/pid12; native report seqlock16,
stamp20, scene24 (0gameplay/1scripted/2unknown), status_flags28, capabilities32
(bit0 manager enemigo disponible, bit1 key_trigger disponible), contactos36,
correcciones40, acciones44; request seqlock48/stamp52/action_id56/generation60.
Las publicaciones del reporte usan try-lock SRW para impedir escritores nativos
simultáneos sin bloquear el renderer. Cache de clasificación8ms invalidada por
epoch/camera/generación; no se hace VirtualQuery por cada objeto dibujado.

Wire opcional action_id entero0..INT_MAX. Token positivo solo armado y sin menú;
edad máxima500ms y misma generación. Request0 cancela, duplicate token no repite.
R presta KEY_CK/KEY_ACT_A en key_on/key_trg64bits; restaura solo esos bits antes
del próximo callback original. No fire/aim/menuconfirm ni mensajes de escritorio.
RuntimeState fresco permite mantener una sesión ya armada sin actor durante una
escena clasificada; no inventa PlayerSample, no arma inicialmente sin actor, y
stop nativo siempre prevalece. Escena nativa suprime movimiento/HUD/cubos y
mantiene el editor; al salir se renueva solo la referencia temporal del seguidor.
Foco/F8/TCP/lease conservan las paradas explícitas, sin rearme automático.

Enemy manager/header y actor pos94/id100/guidF8/hp324/routineFC se validan contra
SDK y memoria. Barrido horizontal de humanoides vivos Move/Damage, máximo256
slots y64 actores, relectura antes de X/Z. Actor/array/generación/origen cambiado
o salto real rebaselinean el actor, sin cortar Leon. No se escriben Y/AI/HP/
velocidad/rutinas ni se garantiza navegación/bloqueo de ataques/bosses.
Health100bytes mantiene ABI; lector acepta441/442. R requiere host/DLL442.

## Clasificación0.4.43

STA_CINESCO0x01000000 describe formato/letterbox y aparece en gameplay normal;
se excluye de RE4_SCRIPTED_MASK. Estado real observado0x03000110 con MainLoop3
se clasifica0; movie/event/death/cut-change y rutinas globales no MainLoop
mantienen1. pad refresca clasificación/reporte aun desarmado o parado, sin
activar controles ni escritura gameplay. runtime_scripted sigue condicionado
al armado y stop0; no se cambian leases ni recuperación automática.
Runtime64bytes y otros ABI conservan tamaño, build443. Host exige443 en Runtime
para rechazar la clasificación defectuosa442; otros lectores conservan historial.

## Construcción/salto0.4.44

Control80bytes v2 conserva layout; pitch ahora admite[-90,90]. refreshLook cambia
solo mirada de la instantánea, preserva createdNs, armado/generación/botones y
menú. No renueva el TTL de input200ms ni modifica stops.

Runtime v2/build444 ocupa80bytes. Reporte0..47 conserva layout; request seqlock48,
stamp52/action56/generation60 más float lift64/token68/float scale72/enabled76.
Solo host escribe48..79; lift0..128*scale, scale40..2500, token1..INT_MAX, enabled1,
snapshot completo/TTL500ms/F8/arm/generation/FP requeridos. Interpolación50ms por
token nuevo en pad; menú conserva altura, escenas/paradas retiran el offset.
Solo cámara/pies de construcción virtuales: Y/velocidad/rutina nativa no escritos.
El umbral de discontinuidad conserva X/Z y Y cruda del actor, excluyendo offset
solicitado. Radio/altura física virtual siguen escala, acotados por solver.

Scene.collider_count opcional0..boxes.size indica prefijo físico. CubeCommand
v6 mantiene96352bytes; material_reserved3164 vale prefix+1 o0 (histórico: todas).
Lectores aceptan únicamente0 o mode2 con prefix válido. Barridos jugador/enemigo
usan prefijo; dibujo/materiales conservan todas las formas. Un scene solo con
fragmentos tiene collisionsfalse y prefijo0. Límites128formas/64tiles intactos.
La física/vida Minecraft es del executor local; extenuación y daño de caída
acumulados por generación evitan duplicación al coalescer tareas del seguidor.
EditorPhysics grounded distingue penalización aérea de minería, manteniendo
lease exacto y restauración en F8. Ganados/combate/animación nativa pendientes.


## Actualización 0.4.51 — melee

Runtime v4 mantiene128bytes y el host exige build451. Los canales existentes
aceptan451; un canal nuevo Local\\RE4Bridge-Combat-PID usa magic0x52453446,
version1/build451 y5256bytes. Header0..15:magic,version,build,pid. Petición:
seqlock16; stamp20,enabled24,generation28,id32,damage36; scale float40,
reach float44; slot48,guid52; start[3]float56,end[3]float68. Snapshot20..79
bajo la misma secuencia. Reporte:seqlock80,stamp84,generation88,ack92,reason96,
target_guid100,target_slot104,hp_before108,hp_after112,hits116,kills120,
attempts124,active128,count132; hasta128 CombatActor de40bytes desde136.
Actor:slot,guid,kind,hp,max (u32),position[3],radius,height (float).

JSON control añade combat_enabled booleano, combat_scale40..2500 y combat_reach
positivo<=6, combat_terrain_revision entero0..INTMAX y combat opcional. Ataque
requiere sesión armada en primera persona sin menú, con id1..INTMAX, slot0..4095,
guid u32, damage1..50000 HPnativos, scale coherente, reach<=6, start/end finitos
<=20M y longitud positiva<=scale*reach+1. JVM caduca el ataque a200ms; native
control/petición requieren frescura<500ms y la misma generación. Sin ataque,
watch publica id0: un catálogo no escribe HP. Datos de reporte solo los escribe
la DLL, nunca el host. Id consumido no se vuelve a ejecutar, incluso en fallo.

Minecraft usa atributo real GENERIC_ATTACK_DAMAGE y cooldown del servidor
integrado: base*(.2+.8*carga²), redondeado a50HP/punto. Identidad del objeto/ranura
coincide al solicitar; desgaste/exhaustión solo tras ack confirmado del mismo
golpe. Un token de cancelación impide liberar una solicitud posterior desde una
sesión cancelada. Menú/desconexión/F8/escena no admiten ataques nuevos.

Nativo recoge actores vivos Move/Damage del pool SDK con límites4096slots/64MiB;
catálogo próximo máximo128 y rechazo de desbordamiento. Caja corporal:radio250 y altura1800.
Repite raycast contra actores actuales y valida slot/GUID, máscara viva/tipo,
pose/HP/max/rutina, puntero de jugador y pool antes de escribir HP(+0x324).
Un fatal escribe r_no_0..3(+0xFC)=3,0,0,0. No hay llamadas a funciones de firma
no descubierta, despawn forzado ni escrituras a misión/botín/HPmax. Ventana
500ms por identidad: rechaza daño menor/igual y aplica solo diferencia si mayor.
Corrección de posiciones de enemigos y apoyo vertical conservan sus canales.

Geometría de combate usa otra TerrainFeed con el mismo límite1024triángulos y
radio(reach+1)*scale+2500 (6500 con alcance3/scale1000). Mantiene caché/hilo,
generación y revisión propios; la lectura continúa fuera del hilo de entrada.
Cambio de radio durante lectura descarta el resultado. JSON combat_terrain y
combat_terrain_state se parsean aparte y no sustituyen terrain de construcción.
Se requiere cobertura completa del rayo; desconocido no autoriza daño.
Rayos de bloques usan sus formas actuales y la DLL vuelve a comprobar la
escena física vigente. Un bloque anterior al enemigo mantiene la minería.

## Actualización 0.4.50

Runtime v4/build450 conserva 128 bytes y sus offsets. JSON vertical requiere
además native_ground booleano. true exige lift=0 y publica vertical_enabled=2
@76; false mantiene modo1 absoluto. El host exige build450 para distinguir estas
semánticas. Los lectores de otros canales aceptan build450 sin cambiar sus ABI.

Con apoyo nativo, BridgeVertical sigue la Y observada de RE4 y vuelve a referir
su rango al suelo actual. El mapa ausente/incompleto no impide caminar por
escaleras; pequeñas discrepancias del suelo conocido y cobertura ausente no
imponen una altura distinta a la del actor. Salto y apoyo en bloques conservan
el extremo absoluto; el retorno del actor elevado no se adopta como nuevo suelo.

Modo2 libera runtime_vertical_valid/actor_active y los extremos de vista,
conservando radio/altura/skin de colisiones según scale. No escribe Y del actor.
La cámara usa sus pies nativos. Se solicita nueva referencia de rango para el
siguiente salto, después de comprobar identidad del actor. No amplía la cesión
a F8/TTL/datos inválidos: esas guardas siguen invalidando el control. actor_y y
vertical_writes describen aplicaciones absolutas; no son un eco de feet_y ni
un contador de pasos nativos. Una primera petición absoluta en menú no adopta
un extremo anterior. El siguiente salto interpola desde su nuevo suelo.

## Actualización 0.4.49

Runtime v4/build449 tiene 128 bytes. Conserva offsets anteriores: petición
48..83 bajo request_sequence@48; feet_y float absoluto@112 pertenece a esa
misma publicación y se copia entre las mismas dos lecturas de request_sequence.
Reporte bajo sequence@16 añade actor_y float@116, vertical_active u32@120 y
vertical_writes u32@124. El host no escribe esos campos de reporte. Los demás
canales mantienen sus versiones y aceptan build449; Runtime exige v4/build449.

JSON vertical requiere exactamente lift,scale,token,feet_y; token entero positivo,
scale40..2500, lift0..128*scale, feet_y finito y abs<=20000000. La Y absoluta viene
del mismo ControllerMapping.gridHeight que los bloques. TerrainFeed centra la
consulta en feet_y, sin volver a sumarle lift al actor ya elevado.

Runtime conserva extremos absolutos de vista interpolados50ms y el último Y
físico recibido. La nueva generación empieza la vista en feet_y-lift; una
primera petición de menú no puede aplicar el extremo de una generación previa.
Invalidación/F8/TTL limpia la validez. Pausa congela la vista y conserva el apoyo.

El callback vertical escribe únicamente cCoord::pos_94.y (+0x98) del jugador
validado, antes de la colisión X/Z, incluso sin bloques o pase gráfico válido.
Revalida puntero/pose/arm/generación/F8/TTL y rechaza cambio de actor o un destino
más allá de128*scale desde su altura inicial. No altera velocidad, pos_old,
rutina ni actores enemigos. F8 detiene las escrituras sin restaurar posiciones
anteriores. La colisión horizontal excluye de su detector de teleport la Y que
controla esta simulación; mantiene los límites de desplazamiento X/Z.

BridgeVertical conserva suelo independiente de las muestras del actor elevado.
Usa rayos a suelos/pendientes tipados bajo los pies, paso máximo0.6bloques,
apoyo de bloques colocados y límites de cobertura. El retorno de la altura real
no acumula saltos ni crea un suelo flotante. Esta actualización sustituye la
limitación de Y virtual descrita en las versiones históricas siguientes.

## Actualización0.4.48

Runtimev3/112bytes y Scenev6 conservan sus layouts; el host exige DLL448.
El comando lift conserva su significado relativo. El último lift validado
alimenta `runtime_physics_lift`; `runtime_lift` sigue siendo solo la altura
visual interpolada. Menú no sustituye el extremo físico por el valor visual.
Datos inválidos/F8/caducidad/generación eliminan ambas alturas mediante las
guardas de vertical existentes. La telemetría view_lift sigue siendo visual.
La colisión del jugador usa un margen de apoyo superior0.01*scale, sin modificar
el límite de cabeza, la geometría compartida ni la Y nativa. Los enemigos
conservan sus dimensiones y tolerancia anteriores.

## Actualización0.4.47

Runtime v3 conserva offsets112bytes y requiere build447 en el host. La acción
contextual usa la captura real E=`0x80400` (KEY_CK|KEY_A), no la combinación
sintética de0.4.46. Se conserva una arista y una pulsación de duración limitada.

Las coincidencias de EmMgr se deduplican por operando dentro del módulo. La
selección ocurre durante la ejecución, cuando el pool está inicializado: un
solo candidato debe cumplir los límites de cManager y stride>=sizeof(cEm)=0x408,
con array legible. Dos candidatos compatibles o ninguno rechazan la selección.
No se fija el RVA del diagnóstico ni se elige por orden de coincidencias.

Scene v6 mantiene el mismo ABI. La época identifica generación, origen,
colisionadores y modo. Movimiento/expiración de fragmentos, texturas y UV de
minado renuevan el snapshot sin rearmar la época. La DLL distingue stamp de
lease (60Hz) de stamp de geometría (20Hz); duplicados no anulan velocidad.
La separación horizontal usa una salida de la unión conectada de cajas,
incluido un actor inicialmente dentro, conservando Y y las guardas de escritura.

## Actualización0.4.46

Runtime v3 conserva sus112bytes y offsets; el host0.4.46 exige build446 para
no aceptar silenciosamente una DLL445 sin estas correcciones. Una solicitud R
produce una sola arista de Key_btn_trg y un Key_btn_on sostenido hasta120ms;
los paquetes que repiten el mismo token no prolongan ese plazo. Las llamadas
auxiliares de proyección conservan el disparo inicial hasta la siguiente lectura
del pad. Menú/F8/caducidad/generación/muerte cancelan la inyección.

La firma EmMgr del SDK fijado sigue siendo la misma. Se permiten hasta64
coincidencias solo si todos los operandos resuelven una misma dirección
alineada y legible dentro del módulo. Destinos distintos permanecen rechazados.
PROBE añade conteos/destinos y capacidad/stride mediante lecturas.

Scene v6 tampoco cambia. La predicción visual opera sobre el sufijo posterior
al collider_count de una copia local, nunca sobre IPC ni cajas de física.
Predice como máximo50ms entre snapshots compatibles con intervalos10..100ms;
materiales, tamaños, sesión, origen o conjunto incompatibles reinician velocidad.
No extiende leases ni la vida del fragmento, ni cambia posición Y de los actores.

## Altura/acciones/enemigos0.4.45

Runtimev3/build445:112bytes. Reporte0..47 y comando48..79 mantienen offsets.
action_kind80 pertenece al host y comparte request seqlock48. Solo0(contexto)
o1(skip) con token positivo; nativo copia48..83 de forma estable. Reporte nativo
agrega enemy_reason84/scanned88/eligible92/skips96, float view_lift100,
enemy_capacity104/stride108 bajo report seqlock16. Host escribe48..83 solamente.
R presta CK/PICK_UP/ACT_A/OK; F6 EV_CANCEL solo en movie/event, nunca death.
Pad restaura los bits prestados antes del input original. En proyección de escena
nativa, si pad lleva80ms inactivo, se entrega un pulso de50ms con los mismos
checks/token único. Stop/foco/menú/generación caducada nunca se rearman.

Pools enemigos admiten4096slots y64MiB, comprobando hasta64regiones comprometidas.
Flags Alive sin Dead/Destruct son físicos aunque Trans esté apagado.128actores
vivos Move/Damage, whitelist humanoide, lectura/revalidación de identidad y
pose antes de X/Z; proyección reconcilia después del movimiento. Separación
horizontal inicial limitada a8pasadas; no altera Y/IA/HP/rutina/velocidad.

TerrainFeed centra la extracción local en Y nativa+lift publicado, sin ampliar
radio12000 ni límite1024triángulos. Un techo fuera de cobertura aplaza la subida;
no transforma geometría desconocida en espacio libre. La posición cliente del
seguidor se escribe solo END tick, para preservar interpolación vanilla. El
suelo temporal mantiene groundY. El salto nativo conserva sus pies virtuales y
límite128bloques; no escribe la Y real de Leon.
