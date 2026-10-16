## 2026-10-16 — compatibilidad y entregas de prueba (sin nueva versión)

Detectado RUNASADMIN global32 para bio4.exe, con manifiesto asInvoker y acceso
directo sin flag elevado. Usuario autorizó retirar exclusivamente ese valor;
copia restaurable verificada y operación UAC completada, sin cambiar archivos
del juego ni UAC global. Al persistir UAC, se detectó otro RUNASADMIN en HKCU
del usuario real: la consulta previa usó el usuario aislado de comandos. Copia
restaurable y retirada de esa misma ruta sin UAC; verificación real de ambos
hives/vistas32/64 y hash del EXE intacto. Falta validar inicio/acceso del host.
Registrada preferencia: cada actualización para probar se entrega con enlace
de descarga de paquete versionado en GitHub. Detalles en docs/LOCAL-WINDOWS.md.

## 2026-10-16 — auditoría de continuidad local (sin nueva versión)

Recuperadas las ramas publicadas hasta0.4.55 desde el checkout main0.4.45,
sin fusionar main ni cambiar mecánicas. Usuario confirma0.4.54 como última
versión jugada; durante la auditoría actualizó DLL/JAR a0.4.55, verificados por
lectura/hash. El agente no modificó instalación, ejecutables ni partidas.
Windows10, Python3.12/JDK21/Gradle8.11.1 y suites PowerShell comprobados.
Python156/157 pasan; core53/57 inicialmente: simulador pasa con entorno UTF-8,
tres limpiezas de fixtures fallan por logs abiertos. Presupuesto temporal cero
Python falla con resolución de reloj Windows. GCC y captura gráfica pendientes.
Informe, límites y plan en [docs/LOCAL-WINDOWS.md](docs/LOCAL-WINDOWS.md);
evidencia privada bajo D:\logs. Sin gameplay real ejecutado por el agente.

## 0.4.55 — trampas nativas, altura y presupuesto de renderizado

Los golpes alcanzan cEm2a (trampas de osos/bombas) mediante el mensaje de impacto
existente. Sus propios manejadores consumen el golpe, cierran la trampa o activan
la explosión con efectos/sonido nativos; no se escriben HP ni rutinas. Se confirma
la transición observada y se excluyen de muertes/corazones. El perfil de ambos
manejadores se comprueba antes de habilitar estas dianas.

La protección de desplazamiento horizontal usa X/Z: caídas y cambios de suelo
ya no cuentan como teletransportes. Conserva el límite horizontal y los leases.
Distingue códigos de F8, expiración, colisión, altura, gráficos y VIEW, con el
primer motivo y estado gráfico/colisión en la consola. Minecraft conserva un
máximo de60FPS solo durante el control, respetando límites inferiores del usuario.
No se implementa aún combate de arco/ballesta. Ver [guía0.4.55](docs/RE4-BLOCK-0.4.55.md).

## 0.4.54 — estabilidad, pausas del cliente y diagnóstico

Conserva la DLL453 publicada y el combate confirmado por el usuario. El socket
corta movimiento a200ms sin tick, conserva una sesión congelada hasta1s y exige
nuevo armado después. Acciones/combate se suprimen, manteniendo soporte/cámara;
imágenes y red no renuevan el plazo. Foco transitorio tolera hasta200ms con
entradas suspendidas; una pérdida persistente o F8 no se rearman solos.
Consola asíncrona limitada para evitar bloqueo al seleccionar texto. Registra
cambios de control inmediatamente con motivo del cliente, foco, pausa y edades.
VIEW restaura estado iconic/zoom sin activación, detecta modo exclusivo conocido
y conserva brevemente ForegroundStaging sin reenfocar otra aplicación. El HUD
comprueba cadencia antes del lease OpenGL. No se afirma mejora medida de FPS.
Pruebas Linux de protocolo/TCP, callbacks, PowerShell y compilación pasan; foco
real Windows y causa única de todos los cortes anteriores aún sin confirmar.
Ver [guía0.4.54](docs/RE4-BLOCK-0.4.54.md).

## 0.4.53 — daño letal, estado de cajas y menús de recogida

El callback de daño reclama el mensaje propio bajo bloqueo, sin exigir el hilo
que lo envió. La llamada nativa ocurre fuera del bloqueo. Un golpe propio puede
matar aunque llegue la bandera no letal; disparos ajenos mantienen sus argumentos.
Las cajas esperan el estado roto nativo, incluso cuando no pasan por ApplyDamage;
no se fuerzan HP, rutinas, audio ni drops. Corrige el tipo de impacto del host.
STA_ITEM_GET habilita flechas, Enter/R para aceptar y Retroceso para cancelar.
Cola limitada con confirmación, intervalo entre pulsos y descarte al cerrar menú.
Combat v3/5324 bytes y Runtime v5/160 bytes; build453 en todos los canales.
51 pruebas core, 87 Minecraft, 181 Python y cinco fixtures C pasan en Linux.
Actualización desde DLL452 comprobada con el artefacto publicado. Prueba en el
juego Windows pendiente. Ver [guía 0.4.53](docs/RE4-BLOCK-0.4.53.md).

## 0.4.52 — impactos nativos, nuevos objetivos y host

Sustituye HP/Die directos por Bullet_RayTest, EmDmgMsg_Set y un ajuste limitado
al mensaje del puente en Em_ApplyDamage. RE4 elige partes, reacciones y muerte;
la confirmación asíncrona precede a durabilidad y efectos. El diagnóstico del
usuario confirma timestamp 0x5a6192e5, prefijos, RVAs y el único thunk de daño con
91 llamadas. Otros perfiles se rechazan sin parchear instrucciones desconocidas.
Añade cuervos y Box/Rack/Window/Barrel con HP válido; partículas originales de
Minecraft se capturan en el overlay, sin corazones para objetos. Mantiene
bloques oclusores hasta el impacto real. No añade daño de barrido.
El host elimina el segundo extractor de mapa, resume la consola, limita el
frame de salida y guarda errores con rotación; una sesión fallida puede reconectar.
Combat v2/5296 bytes, Runtime v4/build452; el desinstalador reconoce DLL451.
Pruebas simuladas pasan; animaciones/destrucciones/partículas en juego pendientes.
Ver [guía 0.4.52](docs/RE4-BLOCK-0.4.52.md).

## 0.4.51 — cuerpo a cuerpo Minecraft contra ganados

Implementa clic izquierdo, atributo de daño de la mano principal, recarga vanilla
y alcance de entidades. Selección del primer ganado por rayo3D, oclusión de
bloques/paredes y consulta local de geometría independiente de construcción.
Catálogo nativo de128actores próximos, identidad slot/GUID y confirmaciones de
impacto. Servidor Minecraft calcula daño/desgaste y solo cobra durabilidad tras
confirmación. Módulo nativo aplica HP y entrega bajas a Routine0::Die; paquetes
repetidos y ventana de daño no duplican golpes. No envía disparos de RE4.
Canal Combatv1/5256bytes, Runtimev4 exige451 y desinstalador reconoce450.
47pruebas core,85Minecraft y173Python pasan; DLL/JAR e instaladores simulados
verificados. Reacción/muerte/eventos reales pendientes en Windows.
Ver [guía0.4.51](docs/RE4-BLOCK-0.4.51.md).

## 0.4.50 — apoyo nativo para escaleras

En el video nuevo, la escalera de RE4 devuelve al jugador mientras el mapa local
está incompleto: 0.4.49 sigue escribiendo una Y fija aunque no haya bloques ni
correcciones X/Z. Se separa el apoyo nativo del apoyo en bloques: RE4 conserva
su altura al caminar; salto/columna mantienen Y absoluta. Suelo con redondeo o
fuera de cobertura no adelanta una altura distinta en Minecraft. El siguiente
salto se referencia al nuevo suelo sin perder las guardas de actor/generación.
Runtime v4/build450 añade modo2 de altura nativa, sin cambiar offsets ni escala
de colisionadores. Instaladores reconocen DLL449. Pasan pruebas de escalera,
columna con mapa ausente, aterrizaje, salto y caducidad; juego Windows pendiente.
Ver [guía 0.4.50](docs/RE4-BLOCK-0.4.50.md).

## 0.4.49 — altura absoluta y elevación real de León

El video y el log de 0.4.48 muestran expulsiones sobre una pendiente: sumar el
salto relativo al suelo nativo vuelve a introducir los pies en el apoyo entre
paquetes. Se transmite la Y absoluta de la cuadrícula para cámara y colisión.
A petición del usuario, la DLL también aplica esa Y a la posición real de León.
La gravedad consulta suelos/plataformas nativas y no reutiliza el actor elevado
como un suelo nuevo. Runtime v4/128 bytes añade solicitud feet_y y telemetría
nativa de aplicación. F8/TTL/generación/actor/escenas acotan la escritura.
Regresiones de pendiente, retorno del salto, columna a plataforma y salida del
borde pasan. Instalación reconoce DLL448. Requiere actualizar DLL/host/JAR;
prueba de juego y ataques desde abajo pendiente en Windows.
Ver [guía 0.4.49](docs/RE4-BLOCK-0.4.49.md).

## 0.4.48 — apoyo físico independiente de la altura visual

La cámara interpolada podía quedar dentro del bloque de apoyo al aterrizar;
la separación horizontal lo trataba como pared y expulsaba al jugador. Física
usa ahora el último extremo vertical validado, independiente de la cámara,
con margen superior0.01bloques para variaciones nativas entre paquetes.
Regresión reproduce el fallo y valida movimiento, pared elevada, borde,
menú e invalidación. Requiere host/JAR y DLL448; confirmación Windows pendiente.
Ver [guía0.4.48](docs/RE4-BLOCK-0.4.48.md).

## 0.4.47 — señal E real, pool cEm, colisiones y relojes de partículas

El diagnóstico Windows mostró E=0x80400 (CK|A) y19 firmas para tres gestores;
solo el pool60/3592 cabe en cEm, frente a100/468 y380/988. Se usa la señal
observada y selección única por estructura/legibilidad de los operandos del SDK.
Leon ahora se separa al iniciar dentro de un bloque, y la separación resuelve
el conjunto conectado sin oscilar entre vecinos. Movimiento de partículas no
rearma la escena y los paquetes repetidos60Hz no anulan su velocidad20Hz.
Fixtures reproducen los fallos antes de las correcciones. Requiere DLL447 y
host/JAR nuevos; confirmación visual/funcional en RE4 pendiente.
Ver [guía0.4.47](docs/RE4-BLOCK-0.4.47.md).

## 0.4.46 — interacción sostenida, referencias compartidas y partículas entre paquetes

Corrección reconstruida desde 0.4.45 tras los registros del usuario: R solicita
acciones pero RE4 no interactúa; detector de ganados ausente (motivo6, cero
actores revisados). Pulso nativo de120ms con una sola arista y respaldo también
en gameplay con pad inactivo. Se aceptan sitios de la firma conocida que
resuelvan un mismo cEmMgr, conservando rechazo de ambigüedades. Fragmentos
visuales predichos por fotograma hasta50ms, sin tocar colisionadores.
Host y DLL446 completos requeridos. 43 pruebas core,72 Minecraft,160 Python e
instaladores simulados pasan; interacción/ganados/fluidez reales Windows
pendientes. Golpes desde abajo con elevación virtual no corregidos.
Ver [guía0.4.46](docs/RE4-BLOCK-0.4.46.md).

## 0.4.45 — altura, acciones y detector de ganados

- Corrige el error de cobertura de techos que se presentaba como «salto de zona» al subir columnas. Mapa local centrado a la altura virtual; espera segura durante actualización.
- Posición Minecraft al final del tick e interpolación nativa entre extremos sucesivos; conserva suelo, extenuación y caída.
- R añade recogida/confirmación a la acción contextual. F6 envía cancelación nativa de movie/event; Esc conserva pausa Minecraft. Canal Runtimev3 y entrega acotada en proyección cuando el pad está inactivo.
- Pools enemigos de hasta4096slots/64MiB y varias regiones; física independiente del bit de dibujo, revisión después del movimiento y separación horizontal de colocaciones ocupadas.
- Diagnósticos de gestor, slots, elegibles y acciones en host/status; excepciones de seguimiento conservan causa/altura en latest.log.
- Nueva DLL445 y paquete completo. Combate y transición continua/separación de construcciones por zona pendientes. Pruebas automatizadas; sin ejecución de RE4/GPU reales en cloud.

# Registro de desarrollo

## 0.4.44 — construcción por caras, salto del controlador y efectos de rotura

Usuario confirma mejoras43 pero falla al acertar caras; pide preparar creativo/
survival, salto/colocar debajo y rotura antes de combate. Ray usa ojos interpolados
y mirada actual en render y antes de input; se conserva selección de bloques
conocidos ante demora del mesh. Mirada90° en core/host/DLL, refresh entre frames
preserva timestamp de input. Volumen de colocación rechaza cuerpo aun con noclip.

Física20Hz: impulso.42, gravedad.08/drag.98, apoyo/techo de bloques y techo nativo,
columna/pillar, gravedad al perder apoyo, pausa, extenuación y daño de caída MC.
Suelo auxiliar anclado a base nativa y pose/minería distingue aire. Runtime IPCv2
80bytes agrega lift/token/scale/enabled bajo seqlock; cámara y pies virtuales
reciben altura suavizada50ms. No se escribe Y/velocidad/rutina de Leon: su salto
animado y saltar obstáculos nativos siguen pendientes. Desplazamiento virtual
no se cuenta como teleport del actor; movimiento nativo conserva sus checks.

Grietas de diez estados leídas del resource pack y multiplicadas sobre material.
Hook de rotura exitosa del manager más observación de mundo con dedup200ms;
fragmentos texturados,24máximo/0.8s, pase nativo de profundidad y sin física.
Scene.collider_count especifica prefijo físico; v6.material_reserved almacena
count+1 (0 conserva todas), validación y barridos excluyen los fragmentos.
World limits/tiles/leases/F8 se conservan. Reporte real de enemigos no resuelto;
no se añade combate en esta entrega. DLL444/mod/host44 requieren actualización
completa. Pruebas sintéticas/builds; juego/GPU Windows pendiente.

## 0.4.43 — Preparar y movimiento bloqueados por falso estado de cinemática

Registro del usuario: runtime.scene1/flags50331920 (0x03000110), armado verdadero,
look_packets0, camera_activefalse, sin movimiento/dibujos; luego stop1. La máscara
42 incluía STA_CINESCO, presente en partida normal widescreen. Se excluye ese bit,
manteniendo movie/event/death/cut-change y rutina global. Runtime se refresca en
pad también desarmado/parado, evitando publicar una escena antigua antes de
nueva preparación. No se alteran F8/foco/TTL/device-loss ni rearme explícito.

Fixture de callbacks reales reproduce el valor exacto, permite primera persona
y W/sprint, cede a MOVIE y retoma. IPC/host comprueba cien mensajes, pausa y retorno;
lector Runtime exige DLL443 para evitar volver a usar la build defectuosa.
ABI sin cambios; otros lectores conservan versiones previas y agregan443.
DLL443/mod43/host43; instalador reconoce hash442 publicado. Prueba Windows pendiente.

## 0.4.42 — escenas nativas, R, altura de pie y colisiones de enemigos

Usuario confirmó hambre/regeneración normales y muerte de ambos juegos al cambiar
Pacífico. Preparar/F5 supervivencia convierte solo Pacífico a Normal, conservando
otros niveles e inventario/vida/comida. Daño transferido añade extenuación vanilla.
updatePose mantiene STANDING solo bajo lease exacto de seguidor; ojos de pie en
sincronización y mapeado. No cambia la geometría ni elimina techos.

IPC Runtime64bytes build442 con reporte seqlock/capacidades y petición separada
de acción por token/generación. Flags de película/evento/muerte y rutina global
ceden cámara/modelo/HUD/cubos a RE4 sin desarmar. Cliente mantiene Steve quieto,
suprime construcción y retoma sin discontinuidad de seguidor. R presta solamente
KEY_CK/KEY_ACT_A a key_on/key_trg y los restaura antes del próximo pad nativo;
press único, TTL/F8/menu/generación y snapshot validado. E inventario intacta.

Colisión horizontal en pad para enemigos humanoides vivos Move/Damage. Manager
descubierto con firma única, memoria/identidad/relecturas validadas, máximo256
slots y64 actores por tick. Spawn/teleport renuevan referencia sin parar Leon.
No se escriben Y, AI, HP, velocidad, rutinas ni animaciones. No navegación,
bloqueo de ataques, colisión vertical, bosses ni armas Minecraft implementados.
Pruebas sintéticas y builds pasan; juego/GPU Windows de estos cambios pendiente.

## 0.4.41 — vida compartida, pausa en movimiento y F5

Esc publica botones0 en setScreen antes de inicializar GUI, gracia máxima1s
solo para instantánea de menú inmóvil, desfase de isPaused al cerrar tolerado.
Colisión/cámara mantienen referencia con bit de menú; F8/foco/lease normales.
F5 consume press/repeat/release solo en puente enfocado sin menú, cambia modo
local y confirma al cliente; conserva referencias y vida/inventario.
Vida vinculada por porcentaje con IPC100bytes separado. Daño y curación nativos
son contadores acumulados normalizados con ACK después del servidor Minecraft,
sin doble aplicación ni curación que borre golpes pendientes. Regeneración
Minecraft actualiza Leon; cero corazones escribe HP0 y rutina Die desde Move/Damage.
Muerte nativa retenida aunque desaparezca actor; F8 termina enlace y nueva sesión
requiere actor recargado. Identidad/generación/rutina/lease/F8/races validados.
DLL441 PE32 nueva; juegos/animación/hardware Windows pendientes de prueba.

## 0.4.40 — menús sin rearme y supervivencia Minecraft

Esc/opciones/contenedores y pantalla de muerte se capturan en overlay conservando
armado con buttons0 y generación; cierre retoma control, foco/F8/TTL permanecen.
Selector VIEW keep/survival/creative autenticado y confirmado, cambio local de
GameMode antes de armar sin objetos/HP/comida gratuitos. Seguidor/noClip también
en supervivencia, vuelo prestado y restaurado según modo; no invulnerabilidad.
HUD llama renderMainHud/XP/efectos vanilla para corazón/comida/armadura/aire,
mano/items/recetas reales. Sprint consume extenuación por distancia efectiva;
minería omite solo penalización aérea del seguidor. Vida/comida Minecraft
independientes de Leon, conforme a la elección. No pausa reloj ni mapea daño RE4.
Core38/Minecraft57/Python139, VIEW autenticación/modo/foco/F8. DLL439 conservada
sin recompilar; interacción real Windows pendiente.

## 0.4.39 — referencia física sin dibujo y preparación con foco manual

Log38:36 pases omitidos, sin nuevos dibujos/fallos, seguido por stop1 y
collision_state.reason4. La escena validada fuera de pantalla publica active
sin contar dibujo, recuperando referencia de colisión después de un rechazo
transitorio. No se relaja lease500ms. Fixture integrado renderer+colisiones
reproduce200frames sin vértices, sprint bloqueado y regreso sin parada.
VIEW espera un clic manual si Windows rechaza foco: conserva layout/stop y
comienza petición una sola vez tras activación verificada; F8 cancela.
Minecraft1.21.1 restablecía noClip después de tomar el seguidor: hook tras el
reset y antes de movimiento, solo jugador exacto con lease vivo cliente/servidor.
No se borran ni reimportan construcciones antiguas. Core36/Minecraft51/Python139;
16 fixtures ASan/UBSan. DLL439/mod39; juego y Win32 real pendientes de prueba.

## 0.4.38 — suelo de apoyo y paradas con inputs simultáneos

Fotos37 muestran bloques elevados también en Minecraft. Se elimina el salto de
celda al rechazar contacto nativo; SAT/EAT transmite suelo/pendiente/pared/otro,
con contacto de base solo en apoyos clasificados y recorte a la huella.
Referencia vertical calibrada una vez antes de construir; cristal auxiliar por
columna usa suelo nativo cuantizado y conserva construcciones. El log muestra
un rechazo adicional junto a stop1, sin fallo de dibujo. Colisión usa presupuesto
proporcional al tiempo (sprint medido4.16unidades/ms), conserva lease500ms y
barrido, y mantiene motivo4 tras parada para diagnóstico. VIEW separa hook Win32
del timer UI y registra proceso/clase del foco perdido; no hay rearme automático.
Core36/Minecraft49/Python138 y15 fixtures ASan/UBSan. DLL438/mod38.
Hilo Win32, interacción y estabilidad en RE4 real pendientes de prueba del usuario.

## 0.4.37 — proporción, altura y apilado

La captura real36 muestra5068 dibujos con profundidad, pero el usuario reporta
bloques anchos, contorno/colocación incómodos y sensación de altura incorrecta.
Se corrige el uso directo de focalX GX4:3 en widescreen: focales efectivos
iguales en píxeles, Z nativo conservado. Preparar usa ojos/pose/escala de Steve;
el rayo toma cabeza mapeada y orientación del tick de control, y se recalcula
antes del HUD. Desplazamiento de cuadrícula común en consultas y contorno.
Tolerancia de apoyo0.02bloques (cap20% formas pequeñas) evita elevar un voxel
por diferencias pequeñas de suelo sin retirar muros ni checks nativos.
Pruebas de apilado, esquinas/contorno con rotaciones, ojos y contacto/muro.
DLL437 y mod37; presentación Windows pendiente.

## 0.4.36 — visibilidad con proyección rechazada

El log real de35 muestra231 intentos y0 dibujos de profundidad con razón2.
La validación de proyección retornaba antes de dibujar las caras: regresión de
visibilidad. DLL436 usa compatibilidad al final de la escena para proyección
no validada o buffer ausente, con texturas y sin Z/ZWRITE. La ruta de profundidad
validada conserva su pase previo al clear y los rechazos de control siguen
siendo estrictos. Contador separado de compatibilidad en la palabra reservada
60 del IPC; no se presenta como oclusión real. La captura posterior aporta matrices GX4:3 para ajustar la proyección. El segundo informe contiene OpenProcess Windows5:
CAPTURE lee directamente IPC readonly sin abrir memoria del proceso. El usuario
reporta colocaciones que desaparecen y celda elevada: se reproduce el rechazo
vanilla por punto original fuera de la celda desplazada, se corrige el hit y se
comprueba su serialización real. Máximo una celda adyacente, solo reemplazables
y dentro del alcance. Preparación y colisiones conservadas.
El nuevo informe confirma frustum GX4:3 con viewport1366x768 y transform D3D
identity. Se valida el aspecto propio de la matriz, conservando FOV/estructura/
planos, y se proyectan bloques/selección con su focalX real. Near100/far≈1e6,
profundidad normal LEQUAL. Fixture con los16valores observados y pruebas de
alineación de superficies/contorno. Resultado del juego por comprobar.

## 0.4.35 — preparación automática, superficies y profundidad

El log del usuario termina con `Estado de colision cambiando`: se distingue la
lectura seqlock ocupada de los fallos reales y se conserva telemetría previa,
sin prolongar controles. VIEW pide preparación autenticada por localhost al mod;
se conecta/arma sin comandos, permite F8 durante la espera y conserva las
construcciones en una sesión. Worker SAT/EAT acotado a2Hz, sin rescan de código,
rayos y SAT de volumen en Minecraft, flechas con impacto nativo estático.
DLL435 añade lease separado de profundidad, proyección D3D/GX validada y dibujo
previo al clear para HUD cuando procede; restaura estados y omite bloques si
la proyección no se valida. Sin render 3D de flechas ni combate RE4 todavía.
Pruebas de lógica/protocolo/renderer en cloud; resultado visual Windows pendiente.

## 0.4.34 — capa visual hasta60Hz y diagnóstico de interrupciones

La prueba del usuario confirma HUD oculto y mano/inventario correctos en33;
reporta tirones y retornos a RE4 normal. Captura PBO/fence triple sin esperas,
recursos de dibujo reutilizados y transporte hasta60Hz con fuente visual
independiente de los ticks. El límite fresco de controles200ms no se prolonga.
Se descartan frames sobredimensionados sin matar el encoder. ACK con buffer.
VIEW coalesce cursor de menú y mantiene orden posición/clic; registro local de
causa de interrupción. Status incluye tiempos/edad de encoder y GPU ocupada.
Base offline BVH/raycast/intersección exacta sobre430 triángulos de exportación
real; integración de colocación/flechas y profundidad visual pendientes.
DLL434 reutiliza textura/header durante escritura IPC, con TTL/generación/F8
independientes.27 core,39 Minecraft,124 Python y fixtures VIEW pasan en cloud.
No se ejecutaron OpenGL/GLFW/RE4 reales; mejora visual por comprobar en Windows.

## 0.4.33 — interfaz con E/mano/selección y lease de visibilidad de sesión

La firma HUD32 se detectaba pero la captura real seguía mostrando cockpit. Se
separan leases de cámara/modelos y visibilidad: sesión FP mantiene los bits de
clase y hasta256 ID_UNIT visibles0x8 con validación/identidad/restauración.
Capa transparente hasta1600×900 con renderers vanilla de hotbar/mano/Screen E,
cola de último frame y compresión fuera de render. Modal inventario conserva
arm/generación con motor detenido. VIEW ajusta también Minecraft y reenvía solo
movimiento de cursor de menú sin consumirlo; ambos tamaños se restauran en F8.
Contorno negro2px usa view nativo y outline MC transformado; suelo auxiliar solo
cara superior. Mapa importado/raycast RE4, salto y daño de combate siguen pendientes.
Canal Overlay128+5760000 separado; control80/escena96352/HUD legado32792 intactos.
119 Python,26 core/34 Minecraft,12 fixtures ASan/UBSan,4 suites C, Win32 y
PowerShell pasan. No hay prueba visual/GPU/GLFW real en la nube; guía33.

## 0.4.32 — visibilidad de cockpit y hotbar vanilla

La captura real contradice la ocultación DPF_COCKPIT0.4.30: se retira ese intento.
Firma/layout IDSystem validados y bits m_disp_off de LIFE_METER/BLLT_ICON/
LASER_GAUGE prestados y restaurados; capability16 registra disponibilidad.
InGameHud.renderHotbar en FBO transparente32KiB reemplaza partículas/dígitos,
con modelos GUI, overlays y cache sin readback repetido para bloques estáticos.
Estado GL/cachés restaurados incluso ante fallo; animaciones hasta10Hz.
Mapa enviado: SAT205 + EAT225 triángulos, managers/tablas distintos,430 en total,
98 suelos/109 pendientes/223 paredes, sin errores. Aún no es un mapa importado.
DLL/build432, mod0.4.32 y host nuevos; ABI control80/escena96352/HUD32792 intactos.
115 Python/25 core/32 Minecraft,11 fixtures ASan/UBSan y4 suites C pasan.
Compilación no certifica apariencia ni coste GPU en Windows. VIEW/cámara/
velocidades/colisiones conservados; guía docs/RE4-BLOCK-0.4.32.md.

## 0.4.31 — VIEW para clics de construcción y referencias múltiples de SAT

Usuario confirma bloques/plataforma0.4.30; HUD correcto pero fuera de pantalla
en ventana. VIEW prepara el tamaño antes de arm y F9 solo eleva sin activación.
Ratón: reenvía botones/rueda reales sobre RE4 a Minecraft con HWND/PID/foco
verificados, sin duplicar movimiento/teclado ni recuperar foco automáticamente;
retirada y liberación al detener/fallar/cerrar. WS_EX_NOACTIVATE restaurable.
Mapa recibido:220 triángulos EAT válidos; SAT18 firmas aún sin destinos conocidos.
Nuevo exportador exige todas las referencias al mismo manager validado y registra
RVA/operandos.115 Python y fixtures PowerShell del escritorio pasan; Win32/GLFW
real no ejecutado. DLL430/JAR0.4.30/API/host/instaladores sin cambios de bytes;
no se requiere reinstalación. Mapa compartido/salto/combate siguen pendientes.
Guía: docs/RE4-BLOCK-0.4.31.md.

## 0.4.30 — cámara de escena, editor sobre suelo temporal y exportación SAT/EAT

El usuario confirma mejor rendimiento0.4.29; carrera medida durante movimiento
4,116 frente a4,158 bloques/s nativos, por lo que se conserva la velocidad.
Mirada absoluta en la transformación fija (capability8/flag4), una vista por
escena D3D, copia coherente para dibujar bloques y bit DPF_COCKPIT restaurable.
El editor utiliza aire por encima del terreno y un parche temporal11×11 de
cristal que sigue a Steve; no sobrescribe construcciones ni se exporta a RE4.
EXPORT-RE4-MAP obtiene solo lectura de triángulos SAT/EAT locales y genera JSON,
OBJ y visor HTML offline. El mapa compartido, obstáculos de construcción, losas,
salto y combate requieren validar esa geometría real y siguen pendientes.
Cambian DLL/build430, host y mod0.4.30; control80/escena96352/HUD32792 iguales.
114 Python,25 core,30 Minecraft;11 fixtures ASan/UBSan y4 suites C; instaladores
pasan. APIs Windows y geometría simuladas; visual/arranque/export reales no
probados en la nube. Detalles: docs/RE4-BLOCK-0.4.30.md.

## 0.4.29 — reutilización de vista activa y medición de movimiento

Usuario confirma mejora28 desarmado, caída tras arm/FP y menor velocidad incluso
con sprint que sí tiene efecto. Projection reusa vista dentro del mismo update
y<=16ms, conservando F8/TTL y comprobando entrada/frame/pose/propietarios/FOV.
Cambios obligan al camino completo; matrices/flags se reaplican tras sobrescritura
nativa. Movimiento tardío sigue pasando por la barrera. Modelo oculta solo
draw/shadow0x12; motion0x4000 no se modifica, como confirma SDKcUnit de referencia.

Fixture integra funciones reales de modelos/cámara y collision_tick:10000 vistas
idénticas sin trabajo completo repetido,2000 avances tardíos corregidos por pared,
propietarios/FOV/input/límite16ms/500ms/F8. Medidor standalone de solo lectura,
F10+cuenta atrás/tonos, caminar/correr4s, posición/KEY_ON/cadenciaPadRead/contactos,
JSON y rechazo de datos no comparables. No se impone multiplicador de velocidad.
Se solicitarán casos normal/armado para separar coste de actualización y ajuste.

Build429/JAR26 sin cambios, ABIs iguales.109Python,11fixturesASan/UBSan,4C y
PowerShell. Rendimiento, audio y velocidad reales pendientes de Windows.
DLLc065b2d143d8d92fc2b71f1d12b94f20086160ad568e3de91f5c9a8edf1dca5a.

## 0.4.28 — camino inactivo y llamadas gráficas nativas

RE4 sigue lento incluso solo, según nueva confirmación del usuario. Captura
build427:escena inactiva,0 fallos de dibujo/restauración,174 Present fallidos
acumulados sin HRESULT específico. Se guardará el HRESULT de futuros fallos.
La corrección27 de atlas/HUD no cubría proyecciones ni pasoD3D inactivos.

Shared E9 projection retorna al original cuando el puente está desarmado/parado,
restaurando préstamos solo una vez. Restauración sin dueño evita consultas al
jugador; cámara/mando inactivos evitan consultas al actor. Comparación de código
real enAPI simulada10000proyecciones:40000 memoryquery+20000actor+10000control/
F8/collision/diagnostics pasan a0. No se mide FPS ni driver real.

Facada sigue retenida durante callbacks mediante referencia temporal local;
liberaciones nativas del cliente se difieren hasta retornar llamada exterior.
112 forwards dejan de hacer AddRef/Release nativos adicionales. Tests cubren
119slots, LastError, métodos anidados,1/2 últimas referencias duranteReset y
recursos propios. Dibujo inactivo sololee32byte coherentes; funcionesnoinline
separan los buffers grandes. HUDinactivo omite lectura completa y framegrande.

Build428/JAR0.4.26 idéntico;ABI control/escena/HUD igual.104Python,11fixturesC
ASan/UBSan, cuatroCportátiles y fixturesPowerShell. Primera pruebaWindows será
RE4solo, luego armado si mejora. Se conserva recuperación27; no se amplía mapa.
DLL374e92fc07b52abc1ed894897374869f4d69955b9d50ab0f80b1e9ae0ff93187.

## 0.4.27 — trabajo gráfico repetido y colisión compacta

El usuario confirma Minecraft estable y comunica caída fuerte de rendimiento
en RE4 con 0.4.26; se prioriza esa revisión sobre nuevas funciones de mapa.
HUD y atlas tenían CreateTexture/LockRect/copia/Release en cada frame. Ahora
conservan una textura propia por dispositivo y solo suben imágenes cambiadas.
Recursos DEFAULT se liberan antes de Reset, escape/última referencia, fallo o
desactivación. Capturas de estado siguen temporales para no retener recursos
del juego. Pruebas reales con API simulada:200 frames/una creación/una carga,
cambios de contenido/tamaño y fallos de actualización de recursos ya conservados.

La colisión adicional de proyección leía96352bytes/validaba UV dos veces. Ahora
lee3168bytes coherentes de permiso/transformación/cajas, valida una vez y omite
el barrido en reposo. Mantiene la corrección final y posición segura ante pérdidas
temporales, presión W, caducidad, F8 y discontinuidades. No se añaden hooks ni
escáneres. Se explica que map es edición manual y el bloqueo compartido actual
es seguimiento de la posición de Leon, no extracción de geometría.

Paquete0.4.27/DLLbuild427; mod Minecraft0.4.26 idéntico. Lectores/host aceptan427
y mantienen versiones anteriores.104 pruebas Python,11 fixtures ASan/UBSan,
cuatro suites C y fixtures PowerShell. La nube no mide FPS ni GPU reales; mejora
Windows pendiente de comprobar. DLL44e9f3fbb0531acc6a5460beaaa8960951b5dca9f2bf770b97e25293da39a141.

## 2026-10-10 — Inicio del puente de diagnóstico

- Solicitud: ambos juegos reales en Windows; Minecraft/Steve en primera persona
  dentro de RE4, bloques con colisión, TNT y armas con daño cruzado. Losas para
  aproximación inicial de terreno irregular; formas más precisas cuando haga falta.
- RE4: Ultimate HD Edition, 1.1.0 según pantalla principal del usuario;
  `D:\Games\Resident Evil 4 - Ultimate HD Edition\Bin32\bio4.exe`.
- Minecraft: el usuario instaló Fabric con Minecraft Java 1.21.1 y usa SKLauncher.
  Su carpeta de instancia es `%APPDATA%\.minecraft`.
- Ruta: dos procesos con puente de estados y posteriormente renderizado integrado.
  Se empieza con el lado Minecraft y un receptor sintético, sin direcciones de
  memoria inventadas ni loaders copiados en RE4.
- Referencias leídas: Universal Modder `mod-any-game`, `mashup-mods`, nota de
  colisión/combate y ejemplos GTA V/RE2. RE2 usa RE Engine; no se traslada su
  REFramework a RE4 clásico. La base `re4_tweaks` usa Win32 y Direct3D9.
- Autoridad inicial: Minecraft mueve al jugador; el simulador solo recibe datos.
  El contrato está en `docs/CONTRACT.md`. No se transmite todavía renderizado ni
  se conoce el cambio de ejes/escala de RE4.
- Versiones Fabric/Yarn/Loom verificadas en metadatos oficiales. Se fijaron en
  Gradle; no se requiere cambiar la versión de Minecraft del usuario.
- La máquina cloud tenía Java 21 runtime pero no `javac`. Se preparó Temurin
  JDK 21 con descarga oficial y SHA-256 verificado, fuera del checkout.
- Se añadieron requisitos de red oficiales Fabric/Mojang al borrador. La primera
  consulta dio 403; una consulta posterior a Fabric Maven respondió 200.
- No se lanzaron juegos en el PC del usuario, no se tocaron sus perfiles ni
  archivos de RE4 y no se publicó nada en GitHub.

Resultados actuales y límites de evidencia: `docs/VALIDATION.md`.

## 2026-10-10 — Lanzador Python en Windows

- El usuario ya puso los JAR y abrió el mundo superplano en SKLauncher, pero el
  lanzador batch falló porque Windows no encontró el comando `py`.
- Se añadió detección de `python` como alternativa y comprobación de Python 3.10+,
  con instrucciones de instalación si ninguno de los dos comandos funciona.
- No se cambiaron los JAR ni el protocolo. La ejecución batch en Windows sigue
  pendiente de confirmar en el equipo del usuario.

## 2026-10-10 — Lector externo de RE4

- El usuario confirmo conexion Minecraft/simulador y cambios de `blocks` al
  colocar/romper; se actualizo la evidencia de validacion manual.
- Solicito crear el primer componente de RE4. Se implemento lector externo
  Python/Win32, permisos de consulta/lectura exclusivamente, sin instalacion en
  el juego. Comprueba ruta, PE32/i386, version 1.1.0, unicidad de firmas y punteros.
- Referencia: nipkownix/re4_tweaks revision
  `92c0208bd09c29c9640e13a2493b8e6bd6edd700`, Game.cpp para version/pPL y
  SDK/cCoord.h para posicion/angulos. No se copio el loader ni se uso una
  direccion absoluta de una instalacion particular.
- Seis pruebas sinteticas del lector y cinco del simulador pasaron. El lector
  no esta conectado al protocolo Minecraft, no controla RE4 y su API Win32 aun
  debe probarse en el PC del usuario. No se afirma integracion nativa completa.
- Paquete independiente `re4bridge-re4-reader-0.1.0-windows.zip`, con lanzador,
  lector e instrucciones; generado por `tools/package_re4_reader.py`.

## 2026-10-10 — Conexion RE4 -> Minecraft 0.2.0

- El usuario resolvio acceso denegado (OpenProcess Windows 5) y confirmo que
  posicion/angulos reales cambian al caminar/girar. Capturas con `state: player`.
- Se implemento host Win32 externo de solo lectura y modo autenticado
  `re4_reader`, diferenciado de `simulator`. Minecraft solicita muestras a 10 Hz,
  valida contenido y descarta estados sin personaje o muestras de mas de 1 s.
- Comandos re4/calibrate/scale/ysign/turnsign/follow. Seguimiento optativo RE4 ->
  Minecraft en espectador por teleports del servidor integrado; referencia local,
  escala provisional 100, sin asumir orientacion absoluta. No escribe memoria
  RE4 ni importa render/geometria/dano. El objetivo final Minecraft -> RE4 sigue
  pendiente; esta etapa valida transporte y coordenadas en el sentido de lectura.
- Se detiene por cambios de dimension/modo, perdida de conexion y saltos mayores
  de 64 bloques por eje. Pausa suspende movimiento y conserva conexion.
- Evidencia cloud: 8 pruebas Java core, 12 Python y 2 geometria Minecraft;
  incluye Java/Python con TCP real y fixture de personaje que desaparece.
- Paquete Windows 0.2.0 conserva Fabric 1.21.1 y API. Se sustituye el JAR anterior;
  no se instala ningun archivo dentro de RE4. Prueba grafica conjunta pendiente.

## 2026-10-10 — Control nativo 0.3.0 (en desarrollo)

- El usuario confirmo seguimiento 0.2.0, con captura `follow: true` y 4555 ACK.
- Ruta elegida: proxy dinput8 x86 que reenvia a la DLL original del sistema y
  modifica un CALL identificado de PadRead en memoria, despues del input nativo.
  Agrega solo bits avanzar/retroceder/girar; no teletransporta RE4 ni escribe su
  posicion. La logica de movimiento/colision sigue perteneciendo a RE4.
- Referencia publica re4_tweaks revision 92c0208bd09c29c9640e13a2493b8e6bd6edd700:
  KeyboardMouseTweaks.cpp (llamada PadRead), Game.cpp (Key_btn_on), SDK/pad.h (bits).
  No se asume API de plugins estable. Se rechaza instalar sobre un dinput8 existente.
- IPC local por memoria compartida con contador/heartbeat, corte de 500 ms y
  parada F8 enclavada. Red sigue loopback autenticada. Instalacion con juego cerrado,
  copia previa de saves/config suministrados y desinstalacion por hash propio.
  Validacion real del modulo en Windows pendiente; no se ejecuta RE4 en cloud.

- Ruta de saves confirmada por el usuario:
  `D:\Games\Resident Evil 4 - Ultimate HD Edition\Bin32\Profile\Player\Saves`.
- Se resolvio el CALL interior exacto de la referencia publica, verificando
  ReadCall/InjectHook en CookiePLMonster/ModUtils; no se envuelve el CALL exterior.
- DLL compilada PE32 x86, cinco exports DirectInput, sin runtime adicional.
  La DLL carga la original por ruta absoluta SystemDirectory para evitar recursion.
- 9 pruebas Java, 15 Python y 2 Minecraft pasaron; suite C con 9 aserciones.
  Pruebas PowerShell con archivos sinteticos verifican backup y no sobreescritura,
  retirada exclusiva del DLL propio y saves vacios. Host autentica antes de input.
- Native hook y movimiento real pendientes de prueba en Windows; no se afirma
  colision comprobada, camara primera persona ni input de raton. W/S/A/D mapean
  movimiento nativo clasico. No se fuerza ejecucion RE4 en segundo plano.

## 0.3.1: diagnostico del modulo nativo

- Se conserva el handle de la memoria compartida durante la vida de RE4.
- El host informa el codigo Windows y PID al fallar OpenFileMapping.
- Diagnostico PowerShell de solo lectura: hashes, ruta de DLL cargada y estado IPC.
- Paquete mantiene el mod 0.3.0. Actualizacion retira primero la DLL con el
  desinstalador del paquete anterior; nunca reemplaza una DLL desconocida.
- Validacion cloud: compilacion PE32, politica C, 15 pruebas Python y fixtures
  de instalador PowerShell. La causa en Windows y movimiento real siguen pendientes.

## 0.3.2: compatibilidad del diagnostico PowerShell

- Constructor UIntPtr explicito y enumeracion Toolhelp de modulos WOW64.
- Valida magic/version de IPC antes de mostrar estado/ticks.
- Pruebas PowerShell compilan el interop real y ejecutan el script con una API
  sintetica: acceso solo lectura, puntero de 40 bytes, DLL cargada y canal ausente.
- La captura del usuario fallo despues de abrir IPC: no prueba DLL ausente.
  Conserva mod 0.3.0 y DLL 0.3.1; se publica ZIP pequeno solo con el diagnostico.

## Auditoria de prerequisitos nativos

- Usuario confirma DLL local y del sistema cargadas, IPC presente estado 2/ticks 0.
- Se agrega lector de solo lectura de firmas, punteros, llamadas y regiones
  VirtualQuery para distinguir la comprobacion rechazada sin reemplazar DLL.
- Tres pruebas sinteticas comprueban lectura sin modificaciones, regiones
  divididas, firmas ausentes/ambiguas y version/llamada no compatibles.
- Aun pendiente la salida del lector en el ejecutable real; no se relajan
  comprobaciones ni se activan hooks basandose solo en pruebas sinteticas.

- El lector del usuario encuentra todas las firmas y valida version/memoria;
  falla exclusivamente wrapper_starts_call. Se amplian los datos de la rama
  con bytes/RVA para distinguir CALL, JMP relativo o prologo de funcion.
  ReadCall de ModUtils lee rel32 sin distinguir E8/E9; no se cambia aun el
  hook sin conocer los bytes de la entrada real. Cuarta prueba del lector
  comprueba salto relativo y que se conserva la memoria sintetica.

## 0.3.4: entrada PadRead con salto E9

- El usuario confirma entrada E9 52 FE 55 00, RVA 0x7509, destino 0x567360.
- Se acepta E8/E9 manteniendo opcode y validacion de destino ejecutable;
  no se usan direcciones fijas ni se omiten controles de firmas/version.
- Regresion C usa bytes/RVA observados y verifica CALL, JMP, desplazamiento
  negativo y rechazo de opcode/destino/rango invalidos.
- Se conserva el mod Minecraft 0.3.0. DLL nueva requiere retirar la anterior
  con su desinstalador y reinstalar/reiniciar RE4. Ejecucion real pendiente.

## 0.3.5: armado al cerrar chat e indicador de control

- Usuario confirma estado nativo 1/ticks crecientes pero no movimiento;
  chat confirma comando arm y consola muestra desarmado, sin causa precisa.
- El cliente espera cerrar el chat antes de activar/contar confirmacion.
- Indicador en actionbar muestra motivos de desarmado, teclas solicitadas,
  armado del host y mascara aplicada realmente por la DLL. No requiere alt-tab.
- Nuevo host devuelve motivo, mascara solicitada y mascara nativa en ACK.
- Se mantiene DLL 0.3.4; actualizacion requiere solo mod 0.3.5 y host nuevo.
- Se mantienen cortes por foco/menu/pausa/muestra antigua y rearme explicito.
  Pruebas sinteticas de la secuencia no sustituyen movimiento real en Windows.

## 0.4.0: raton y camara experimental

- Usuario confirma W/S y A/D sin retraso notable, Minecraft enfocado y RE4
  en ventana. Perder foco desarma como previsto; no es fallo del host.
- Giro relativo acumulado y consumo nativo por secuencia; delta limitado y
  PlayerPtr/ang_A0 resueltos con firmas, sin teleport.
- Camara QuasiFPS opt-in, wrapper fastcall con original conservado. Ojo
  configurable y pitch limitado, sin tocar colision/modelo/guardados.
- IPC v2 de80 bytes, host acepta movimiento v1 y anuncia capacidades.
  Comandos mouse/firstperson/eyeheight y HUD compacto con muestras/FP.
- Actualiza DLL/mod/host, retirando DLL con paquete anterior. Conserva Fabric API.
- Pruebas cloud no confirman giro/camara real. Primero giro sin FP, despues FP.

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

## Complemento de vista 0.1.0

VIEW-RE4.bat/ps1 y backend Win32: F9 presenta RE4 borderless/topmost sin activar
la ventana; Minecraft conserva el foco. F8/boton Normal escriben solo parada
IPC validada y restauran estilos/bordes/tamano/posicion/topmost/foco de RE4.
Preparar Minecraft y F9 de vuelta desarman; perder foco o cerrar panel tambien.
No cambia mod/host/DLL ni evita cortes de seguridad. Desktop sintetico y memoria
IPC prueban foco, orden parada/retorno, fallo parcial, proteccion de HWND/pid,
restauracion reintentable y parada v1/v2 sin modificar el frame del host.
Presentacion real WinForms/RE4 pendiente de prueba Windows.

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

## Cámara nativa 0.4.4

El usuario confirma que 0.4.3 sigue desviándose en reposo. Su registro muestra
yaw/puntos de mirada estables durante la transición caminar/parar. Se evita
ejecutar hitCheck antes del reemplazo FP, se bloquea roll, se conserva FOV
desde QFPS_OFFSET y se alarga la mirada a 1000 unidades para preservar
precisión float en coordenadas grandes. Sin nuevos hooks ni cambios de IPC,
mod Minecraft o host. ZIP de actualización solo nativa incluye diagnóstico
ampliado de CameraControl.Camera_60 y reversión mediante instaladores propios.
Tres suites C y 27 pruebas Python pasan; eficacia visual pendiente de Windows.

## 0.4.5

Vídeo y registro 0.4.4 muestran mejora pero saltos y controles tanque. El punto
final Ctrl.Camera_60 conserva un desvío ~12° al detenerse. Se fija la cámara
final de Ctrl/Global en una llamada de proyección validada, con matrices/ejes
coherentes y restauración del estado original. La mirada del ratón pasa a ser
independiente del rumbo de locomoción: WASD lateral/diagonal relativo a la vista
solo en FP, mediante avance nativo orientado, sin modificar coordenadas ni
desactivar colisiones. Paquete completo con JAR/host 0.4.3, DLL 0.4.5 y todos
los START/view/instaladores/trace; conserva compatibilidad IPC. Tres suites C,
29 pruebas Python y fixtures PowerShell pasan. Windows queda por verificar.

## 0.4.6

Usuario bloqueado en FP por projection_matches=7; demás firmas requeridas
únicas. Sustituye la unicidad de sitio de llamada por unicidad de destino en
todas las llamadas E8 encontradas (máximo32), manteniendo rechazo si difieren
o alguna es inválida. Instala todas las llamadas solo tras validarlas. Nuevo
PROBE muestra destinos/bytes de las siete coincidencias para investigar si
todavía no se admite FP. Mod/host conservan0.4.3; paquete completo. Tres suites
C,30 pruebas Python y fixtures PowerShell pasan; activación Windows pendiente.

## 0.4.7

Diagnóstico confirma dos destinos de proyección: seis llamadas resuelven la
función de proyección documentada y una es otra función. Perfil estricto para
el layout UHD1.1.0 observado valida siete sitios, thunks E9 y destinos antes
de seleccionar las seis llamadas correctas. No selecciona por mayoría ni
habilita layouts desconocidos. Mod/host0.4.3 sin cambios. Tres suites C y31
pruebas Python pasan; activación FP0.4.7 en Windows pendiente.

## 0.4.8

FP0.4.7 se activa y mejora según usuario; cambios rápidos A/D/W/S y mezclas
todavía alteran vista. Engancha entrada E9 de proyección del perfil validado
en lugar de solo seis CALLs, conservando cuerpo original separado. ControlFrame
reintenta lectura seqlock y reutiliza únicamente última muestra completa aún
vigente, sin reiniciar ancla/giro por un frame roto ni extender timestamp.
Live stop/desarmado y TTL500ms conservan prioridad. Mod/host0.4.3 sin cambios.
Tres suites C,32 pruebas Python y fixtures PowerShell pasan. Estabilidad real
0.4.8 pendiente de Windows; no se modificaron instaladores ni movimiento.

## 0.4.9 — diagnóstico de proyección

Usuario no nota mejora con 0.4.8; vídeo gpt.mp4 revisado y216 muestras del
trace020114. En reposo sin ratón, Global aún se desvía hasta~12°; al caminar
se alinea. No se atribuye todo al movimiento inicial del ratón. Añade mapping
separado64bytes con build/PID realmente cargados, contadores de entrada,
escritura/rechazo y yaw antes/escrito/después de original_projection dentro
de la misma llamada. TRACE solo lectura y secuencia completa. Sin cambio de
algoritmo de cámara/locomoción; no se promete solución visual. Mod/host0.4.3.
Tres suites C y36 pruebas Python pasan; requiere captura Windows para decidir
si el hook no cubre la vista o si otra fase sobrescribe el ajuste.

## 0.4.10 — correr, cámara pendiente

Trace022618:223 muestras, build409 cargado, ratón acumulado0 en pruebas;
projection.calls2853492→3203928 y writes0. El ajuste final no llegó a aplicarse;
condición de rechazo pendiente. Usuario permite avanzar: cámara jugable.
Añade tecla de sprint Minecraft configurada (Ctrl por defecto, mantener) como
KEY_RUN0x40 del SDK/pad.h público fijado. Máscara0x4F rechaza otros botones;
capacidad4 evita solicitar correr en DLL anterior. FP conserva correr al
traducir direcciones relativas, cancelación de opuestos elimina correr sin
dirección. Mod/host/DLL0.4.10 requieren actualización conjunta. Sin cambios
al algoritmo de cámara, velocidad/posiciones, partidas o inventario.
TRACE muestra transcurrido/restante en consola, sin overlay nuevo.
15 pruebas core,2 Minecraft,37 Python y tres suites C pasan en Linux.

## 0.4.11 — primer bloque visual

Usuario confirma sprint0.4.10 y autoriza avanzar a bloque visible fijo. Selecciona
bloque colocado mediante /re4bridge blocktest: caja fija inicialmente a3 tamaños
delante de Leon, tamaño1000 ajustable, sin escala Minecraft↔RE4 aún validada.
Romper el bloque seleccionado/cambiar mundo/dimensión/desconectar retira preview.
No copia terreno superplano ni mobs. Dibuja aristas verdes sobre imagen RE4,
sin profundidad/texturas/colisión; no se presenta como objeto físico nativo.
IPC separado68bytes y preview opcional sobre TCP autenticado. TTL/generación,
stop/F8 y armado completo requeridos. Detector D3D9 compara tabla exacta de sonda
oficial y exige dispositivo único de ventana del proceso; vtable privada cambia
solo EndScene. Contadores visibles en status. Cámara/controles0.4.10 conservados.
17 pruebas core,2 Minecraft,41 Python y4 suites C pasan; callback EndScene real
se ejercita con COM simulado para verificar restauración incluso al fallar.
Detección del dispositivo y visualización efectiva requieren Windows del usuario.

## 0.4.12 — dispositivo gráfico y retorno de foco

Captura del usuario0.4.11: estado3, frames0, sin dibujo; parada nativa1 y
error al activar ventana. Detector deja de exigir identidad de tabla completa
y mismo hilo de ventana: valida métodos de consulta de sonda y pertenencia
ejecutable de los119 métodos a Direct3D9 oficial. Recorre regiones separadas
y prefijos de estructuras referenciadas por datos RE4, con límite y deduplicación.
Solo dispositivo único de ventana visible del proceso; no wrappers externos.
IPC92bytesv2 con contadores de descubrimiento impresos por host, compatible
con mod0.4.11. VIEW restaura/paraliza aunque Windows rechace dar foco, y pide
activación manual sin excepción.43 pruebas Python,4 suites C y fixtures VIEW
pasan; detección/visualización real permanece pendiente de Windows.

## Diagnóstico gráfico tras0.4.12

Registro del usuario:33.360 slots,1.050 raíces,5 intentos, cero tablas/devices
y frames/dibujos0; permanece así también con armado/cámara activos.
El fallo está antes del dibujo; no es una selección desarmada ni prueba de
incompatibilidad de primera persona. No se conoce aún filtro/ubicación exactos.
Se publica herramienta externa de solo lectura sin DLL/mod/instalador nuevos:
módulos gráficos, firma de fábrica D3D9 y posibles tablas/métodos con módulos
y RVAs, búsqueda indirecta hasta4096bytes limitada en tiempo/bytes/raíces.
No llama métodos COM ni escribe en el proceso; candidatos no autorizan hooks.
Cinco pruebas sintéticas pasan, incluidos límites y limpieza/reporte ante fallo.
Se requiere graphics-probe JSON del equipo para continuar la reparación.

## 0.4.13 — tabla privada y referencia a0xC30

Informe081059: bibliotecas oficiales D3D9/NVIDIA; candidata con tabla privada y
119 métodos D3D9 referenciada a0xC30. Es pista, no confirmación de interfaz activa.
Detector anterior la excluía por ubicación de tabla/prefijo256bytes. Se permite
tabla legible privada conservando coincidencia de tres métodos de consulta y
pertenencia ejecutable de los119 métodos a D3D9 oficial. Ventana visible propia,
fábrica devuelta por GetDirect3D igual a la localizada por firma única de RE4;
referencias devueltas liberadas incluso con fábrica distinta. Prefijo4096bytes,
raíces únicas limitadas, ningún offset absoluto del informe. Fábrica debe seguir
vigente al enganchar. IPC visual100bytesv3 añade probe_valid/factory_rejected,
host admitev1/v2/v3; mod0.4.11 conservado.48 Python y4 suites C pasan; render
Windows pendiente. No se sustituye cámara ni se añade colisión.
## 0.4.14 — recuperación, detector gráfico retirado

El usuario informa tres cierres antes del menú principal después de instalar
0.4.13 correctamente. No se atribuye el fallo a Minecraft ni a la primera persona.
La búsqueda ejecutaba métodos sobre objetos COM candidatos y sustituía su tabla;
validar direcciones no demuestra vida del objeto ni tamaño completo de su tabla.
Causa exacta pendiente del evento de Windows. Se excluye block_render.h de la
compilación y se impide su inclusión accidental con #error: sin sondeo, IPC visual,
llamadas COM ni hooks gráficos. Host avisa que blocktest queda desactivado.
Se preservan controles, cámara, correr y F8; mod0.4.11/API sin cambios.
Colector de eventos1000/1001 anteriores de bio4.exe, sin acceder al proceso ni
provocar otro cierre. Compilación PE32, 48 pruebas Python,4 suites C y4 fixtures
de eventos pasan. Instalador/desinstalador probados con hash propio y copias.
0.4.13 retirada, sin sobrescribir artefactos antiguos.

## Confirmación del usuario de la recuperación 0.4.14

RE4 abre y carga una partida sin problemas tras instalar0.4.14. El colector
re4-crash-20261011-054530.json devuelve events: [] para los últimos siete días;
sin módulo, excepción ni offset para atribuir la causa exacta de0.4.13. Se
documenta éxito de arranque/carga y se conserva0.4.14 como base, con renderer
desactivado. No se republica ZIP ni se requiere reinstalación para esta evidencia.

## 0.4.15 — captura de la creación real, dibujo desactivado

Se parte de0.4.14, cuyo arranque/carga confirma el usuario. Se intercepta solo
la importación PE32 única Direct3DCreate9 si apunta a la exportación oficial de
D3D9 en el directorio de sistema (SysWOW64/Windows32). Cambio CAS con protección
restaurada/rollback; DllMain no llama COM, carga DLL, reserva heap ni crea IPC.
Fábrica recién devuelta envuelta en fachada externa tipada17métodos, identidad
IUnknown/IDirect3D9 y referencias balanceadas; allocationfail devuelve original.
CreateDevice delegado conserva parámetros/HRESULT/salida y registra números,
sin retener/tocar dispositivo ni cambiar su vtable. Interfaces desconocidas
se delegan y se cuentan; GetDirect3D del dispositivo devuelve fábrica original,
por lo que igualdad con fachada/cobertura necesita Windows. Sin EndScene, sonda
auxiliar, barrido de heap ni blocktest. Control/cámara/correr no se modifican.
IPC independiente96bytesv1, lector read-only y CAPTURE-RE4-GRAPHICS para probar
solo con RE4, sin Minecraft/host. Mod0.4.11/API conservados.55 Python y4 suites C
pasan;3 fixtures de captura reales con APIs simuladas pasan ASan/UBSan. Instalador
verifica copias/hash y desinstalador solo retira DLL propia. Arranque/captura real
pendientes de Windows;0.4.14 se conserva como recuperación sin sobrescribir ZIP.

## Diagnóstico tras rechazo de captura0.4.15

El usuario informa incompatible/modificada (estado3), creaciones correctas0.
El mensaje no discrimina tabla de nombres, exportación, ruta, destino, ambigüedad
ni condiciones tempranas del cargador. No se relajan filtros ni se cambian DLL.
Se publica AUDIT-RE4-IMPORTS como ZIP autónomo solo lectura: cabeceras/imports de
bio4 y exportación Direct3DCreate9 del D3D9 cargado, rutas e IPC existente. Acotado
2MiB/20.000 lecturas/10s, sin heaps/COM/escrituras. Identifica OFT ausente incluso
con IAT resuelta, destinos diferentes, ordinales/duplicados y lecturas incompletas.
Estado actual válido con rechazo temprano no se atribuye sin prueba a tiempo de
carga.5 pruebas sintéticas pasan; archivo autónomo6ficheros sin DLL/mod/instalador,
extracción/importación comprobadas y Universal Modder sin fallos/avisos.
Se requiere imports-audit JSON del equipo para resolver el rechazo concreto.

## 0.4.16 — resultado inicial y reintento único de captura

graphics-capture-20261011-135432.json confirma estado3 y cero llamadas en0.4.15.
imports-audit-20261011-135454.json encuentra importación única por nombre en
RVA0x1310CE0 con valor0x68EB4B20, idéntico a la exportación de D3D9 oficial en
SysWOW64; auditoría completa604lecturas/2361bytes. El estado actual es válido,
pero no reconstruye el rechazo inicial. No se atribuye a un motivo sin prueba.

Se conservan motivo/estado/WinError/RVA/valores/cabecera de cada intento y se
permite un único reintento antes del primer DirectInput8Create delegado. Solo
si el rechazo fue importación ausente/incompatible; nunca listo/fallo de protección.
Mismos filtros PE32/exportación/ruta, CAS y rollback. Sin búsquedas de dispositivos,
recreación ni renderer. IPC168bytes/v2/build416, lector read-only compatible con
v1/0.4.15. Paquete completo incluye auditor y los mismos JAR0.4.11/API. Algoritmos
de control/cámara/correr permanecen iguales.61 Python/4 C y3 fixtures ASan/UBSan
pasan; instalación/respaldo/hash propio verificados. Arranque y creación real de
0.4.16 pendientes de Windows;0.4.14 conserva su papel de recuperación confirmada.

## 0.4.17 — rutas oficiales bajo WOW64

El informe graphics-capture-20261011-141909.json identifica motivo7 tanto en
DllMain como en DirectInput8Create, build416, dos intentos y cero llamadas.
La comparación previa exigía SysWOW64 siempre que su API respondiese y excluía
System32, aunque GetModuleFileName conserva la ruta usada al cargar, incluso
con redirección WOW64. Se aceptan únicamente ambas rutas exactas derivadas de
las APIs de Windows; se mantienen exportación/importación únicas, CAS/protección
y rechazo de proxies/locales/subdirectorios. No se confirma que esa diferencia
sea la ruta concreta del usuario hasta obtener el nuevo registro.

IPCv3/build417/3296bytes registra rutas loaded/wow64/system y matched de cada
intento. Lector read-only compatible con v1/v2; algoritmos de controles/cámara
y fachada COM sin cambios, renderer desactivado.62 Python/4 C y3 fixtures de
captura ASan/UBSan pasan; instalador y hash propio verificados. Paquete completo,
mods/API idénticos, se permite reutilizar carpeta tras desinstalar con el paquete
anterior y retirar sus archivos. Prueba real de arranque/captura pendiente.

## Confirmación de captura0.4.17 en Windows

graphics-capture-20261011-142924.json: hook_status1, fase DLL_PROCESS_ATTACH,
attempts1, factory_calls/success2, device_calls/success1, HRESULT S_OK,
wrap/device failures0 y escaped_interfaces0. Ruta cargada SYSTEM32 coincide
sin distinguir mayúsculas con SystemDirectory; SysWOW64 es la otra ruta derivada.
Esto confirma la diferencia de formato excluida por0.4.16 y el éxito de la
corrección, sin recurrir al reintento. Parámetros de creación: ventana1366×768,
adapter0, type1, flags0x44. Sin dibujo, colisiones ni vida actual del dispositivo
garantizada por estas direcciones históricas. Se documenta la evidencia sin
alterar ni republicar los ZIP0.4.17; no se pide otra instalación para este resultado.

## 0.4.18 — prueba explícita de cubo gráfico

Se parte de la captura0.4.17 confirmada. CreateDevice correcto entrega una fachada
IDirect3DDevice9 externa119métodos: forwarding tipado, referencias balanceadas,
objeto/vtable nativos intactos, fallback original al fallar asignación/IPC.
Bloqueo por dispositivo serializa escena, llamadas delegadas y Reset. Interfaces
desconocidas se delegan y suspenden dibujo; recursos siguen devolviendo interfaces
nativas cuya identidad/cobertura difiere. No se reactiva block_render.h.

Cubo de100unidades de aristas verdes, anclado600unidades delante de la cámara
al activar, proyección por CAMERA.v_mat/FOV validada y clipping. Dibujo antes
de EndScene original, BeginScene válido, backbuffer y permiso fresco. Backup
D3DSBT_ALL restaurado aun ante setter/draw fallido, referencias temporales
liberadas, cero recursos GPU persistentes. Fallo de restauración bloquea hasta
reiniciar; pérdida/Reset requieren otra activación. Sin textura/depth/colisión
ni integración de construcción Minecraft. Controles/cámara/correr no cambian.

TEST-RE4-CUBE: cinco segundos de preparación,30de prueba, F8 detiene, caducidad
500ms y mutex de escritor único; publica únicamente CommandIPC32bytes. Telemetría
92bytes/readonly/version1/build418 y CaptureIPCv3 compatible. Informes cube-test
incluyen contadores inicial/final y parada; CAPTURE no activa dibujo.
70 Python/4 C y6 fixtures ASan/UBSan pasan;23fallos de setter y restore/lifetime
comprobados con APIs simuladas. Instalador/respaldo/hash propio verificados.
Mods/API conservados. Arranque/cubo visible/compatibilidad real pendientes de
Windows;0.4.17 se conserva como recuperación y sus ZIP no se sobrescriben.

### 0.4.18 — cubo visible confirmado en Windows

El usuario confirmó el cubo visible en RE4 y aportó captura e informe
`cube-test-20261011-151401.json`. En 30,04 segundos hubo 1.766 dibujos adicionales,
cero fallos de dibujo/restauración y ningún incremento de fallos de Present o
Reset. Los 138 fallos de Present y 3 Reset eran anteriores al inicio de esta
prueba. Pendientes la precisión del anclaje al moverse, recuperación del dispositivo
y estabilidad prolongada. Colocación/eliminación desde Minecraft y colisiones
siguen sin implementar. Se conserva el paquete publicado sin reconstruirlo.

Nueva prueba de recorrido: el usuario confirmó que el cubo se quedó en su sitio,
con vídeo `testeadocubo.mp4` e informe `cube-test-20261011-153629.json`.
1.164 dibujos adicionales en 30,02 segundos, sin fallos nuevos de dibujo,
restauración, Reset o Present. Anclaje observado en ese recorrido; causa del
desplazamiento extraño previo sin determinar. Sin cambios de código o paquete.

## 0.4.19 — un bloque colocado desde Minecraft

Mod/JAR 0.4.19: `blocks on` observa intentos de BlockItem y confirma cambio de
estado durante diez ticks. La última colocación crea una caja fija delante de
Leon; romper/sustituir ese bloque o cambiar dimensión la retira. Solo una caja,
sin correspondencia de cuadrícula, profundidad, texturas, colisiones o daño.
blocktest manual usa tamaño inicial 100; estado/capacidad de dibujo actualizados.

Host/control policy conecta NativePreview al mismo mutex/canal de cubo validado
en 0.4.18. CubeCommand v2/56 bytes lleva modo 0 diagnóstico o 1 bounds explícitos;
telemetría v1/92 y CaptureIPC v3 conservados con build 419. Límite/finito, TTL y F8,
fallback de fábrica, referencias y restauración gráfica se conservan. Fallo de
dibujo/pérdida/Reset durante activación desarma, nueva arm requerida; fallo de
restauración exige reinicio. El host no abre el canal BlockPreview retirado.

72 pruebas Python, 17 bridge-core, 5 Minecraft y 4 suites C pasan. Seis fixtures
nativos ASan/UBSan pasan con APIs simuladas; intercambio TCP real con comandos
en mappings Windows simulados. DLL/JAR compilados, respaldo/desinstalación propios
verificados. La nueva colocación/retirada visual requiere prueba real en Windows.
0.4.18 queda como recuperación y sus ZIP se conservan intactos.

## 0.4.20 — escala provisional y detección observable

Usuario confirma blocktest100 diminuto cerca del suelo; blocks on no visible.
No se demuestra una causa única de la ausencia automática. Default100→1000
(es ajustable, aún sin calibración física), un solo bloque de aristas.
START_CLIENT_TICK observa uso antes de vanilla además del callback; cola16,
confirmación10ticks, deduplicación sin reiniciar plazo ni anclaje. Se observa
aun en primer tick de arm y se sigue publicando solo armado. HUD/estado muestran
selección, confirmaciones y orden al host; log incluye bounds y pases omitidos.

73 pruebas Python, 17 bridge-core, 10 Minecraft pasan; JAR0.4.20 compilado.
DLL SHA256 80ea2448268e72492bf1421a7f232b44031e5cfd927ba14e08ebfc8565a0e885
idéntica a0.4.19, build419/ABI conservados; no reinstalación nativa requerida.
Causa/visibilidad automática y escala siguen pendientes de prueba Windows.

## 0.4.21 — varios bloques en cuadrícula fija y formas de losas

Usuario confirma colocar/retirar 0.4.20 y envía vídeo del resultado. La referencia
1011(1) muestra el objetivo visual Minecraft/RE4 con HUD, espada, TNT y criatura;
el vídeo no establece cómo lo implementó el autor. Se avanza a escena visual,
sin atribuir colisión física al cubo ni asegurar precisión de terreno.

Primera colocación fija origen y orientación común; hasta64 bloques/128 cajas.
Se mantienen separaciones/alturas MC, retirada individual incluso de referencia,
losas inferiores/superiores/dobles, actualizaciones de formas por vecinos y
conservación al descargar chunks. Cola confirma todas las colocaciones de un
tick. blocks height ajusta la altura visual, no obtiene suelo. Nueva blocks on
borra/reancla; dimensión/desconexión elimina escena; no persistencia en disco.

CubeCommand v3/3152 bytes mantiene modos diagnóstico0/único1 y añade escena2,
origen xyz/yaw rígido y count≤128. Telemetría v1/92 bytes/Capturev3/3296 se
conservan, build421. NativePreview acepta v2/419 para diagnóstico único, pero el
mod exige scene_available para blocks on. IPC íntegro, reservados/cantidad,
validación completa de formas antes de dibujar, TTL/F8/mutex se conservan.
Dibujo batch una llamada y un respaldo completo para128 cajas, sin recursos GPU
persistentes ni detector retirado. Creación/hook/lifetime de fábrica sin cambios.

79 Python,19 bridge-core,15 Minecraft y4 suitesC pasan;6 fixtures ASan/UBSan.
Instalación/copia de partidas/verificación hash/desinstalación propias pasan.
DLL PE32 imports KERNEL32/msvcrt/USER32; JAR compilado. SHA nativa:
dbcc928f14ae5e7ef5f990691ea715bdf09ba6b2182008c057139ed913a63496.
Versiones anteriores conservadas; nueva prueba Windows necesaria antes de
superficies o física. No se transferirán las estructuras/mobs del superplano.

## 0.4.22 — caras rellenas con sombreado provisional

El usuario confirma la cuadrícula 0.4.21: dos filas de piedras y losa, retirada
individual sin desplazar las restantes; captura con cinco cajas visibles.
Ahora se añaden caras grises opacas con sombreado por orientación, recorte de
polígonos contra near y cuatro planos de pantalla, culling de caras traseras
más eliminación de caras interiores completamente cubiertas, orden far-first.
No hay texturas, profundidad compartida con RE4 ni colisión física.

blocks style solid/wire conserva referencia y formas; blocks on selecciona
solid cuando el host anuncia surface_available. CubeCommand v4/3152 conserva
layout: style0/1 sustituye scene_reserved en offset76; solo mode2 admite style1.
Build422, CubeTelemetryv1/92 y CaptureIPCv3/3296 conservados. Host nuevo acepta
v3/421 para wire y v2/419 para único; solid incompatible desarma antes de escribir
input. Snapshot inválido se rechaza antes de publicar. Nueva DLL/JAR/host22.

Mesh CPU temporal acotada a 334848 bytes, como máximo16128 vértices; una llamada
DrawPrimitiveUP(TRIANGLELIST), respaldo completo y24 setters restaurados.
Memoria liberada en el mismo frame incluso ante errores; sin recursos GPU
persistentes. Interceptación de creación, factory/device wrappers y hooks de
control/cámara sin cambios. TTL/F8/desconexión y bloqueos de fallos conservados.

84 Python,20 bridge-core,16 Minecraft,4 suitesC pasan. Siete fixtures pasan
ASan/UBSan; clipping/extremos/2000 escenas aleatorias, orden, losas, retirada,
estilo, límites, estado/referencias/heap y fallos cubiertos. Instaladores y
respaldo propios pasan. DLL PE32: imports KERNEL32/msvcrt/USER32. SHA nativa:
4decaf42a3b6dc99b9846522ccc234de2cf621dd7b7bd43a0d48abc32b458c2e.
0.4.21 intacta para recuperación. Arranque/dibujo/rendimiento Windows pendientes.

## 0.4.23 — textura procedural de piedra y comparación de modos

Usuario confirma caras/aristas0.4.22 con dos capturas de la misma construcción.
Este paso añade tile propia16×16 A8R8G8B8, sombreado multiplicativo y filtro POINT;
se aplica a todas las cajas como prueba, sin materiales/resourcepacks de Minecraft.
Cada cara usa tesela completa, por lo que una losa puede estirar el lateral.
No cambia suelo/cámara/física/ocultación RE4. wire/solid conservan escena y anclaje.

UVs se interpolan durante recorte en camera space, vertices28bytes con rhw=1/depth,
FVF TEX1 para textura. Mesh acotada a463872bytes/16128vértices. Texture transitoria
16×16 se crea/lock DISCARD/copia respetando pitch/unlock después de Capture, dentro
de escena visible explícita, y Release tras Apply. No recurso GPU propio retenido
entre frames. Coste de crear textura por dibujo pendiente de driver/prueba Windows.
Setters completos:30 wire/solid,38 textura incluyendo sampler/stages/wrap/alpha.
Fallos restauran/liberan y bloquean el epoch; Apply fallido exige reinicio.

CubeCommandv5/3152, style2=stone además de0/1, build423, telemetríav1/92 y Capturev3/3296.
texture_available anuncia soporte; estilo2 incompatible desarma antes de escribir
input/mapping. Host acepta(v2,419),(v3,421),(v4,422),(v5,423) con capacidades distintas.
Se mantienen TTL/F8/TCP, retirada/slots vacíos, validación completa y hooks previos.

88 Python,20 bridge-core,16 Minecraft,4 suitesC pasan. Siete fixtures ASan/UBSan,
UV clipping/perspectiva/2000 escenas/cotas, textura opaca/variada/pitch, máximos,
fallos de recursos y todos los setters, estado/COM/heap, F8/TTL y rollback pasan.
Instaladores/copias/hashes/desinstalación/diagnóstico propios pasan simulados.
DLL PE32 importaKERNEL32/msvcrt/USER32; SHA256:
20cf5143e37054031b10ce925c82c73cb2e81565cfda7fbc5c40a9fc7f57dbdc.
0.4.22 se conserva intacta. Prueba real de arranque/textura/rendimiento pendiente.

## 0.4.24 — recursos locales por cara y barrera horizontal de Leon

Usuario confirma0.4.23 con movimiento y textura adherida. Solicita textura del
bloque y colisiones. Se implementan materiales reales locales desde modelos
BakedQuad/PNG activos, semilla por posición, UV afines/crop, tintes y capas alpha,
primer frame de animación, reducción16×16. Paleta64teselas con fallback magenta;
recarga de modelos invalida paleta antes del snapshot. No se publican imágenes.
Modelos especiales siguen aproximados por cajas, sin fidelidad de malla completa.

Atlas256×256,64celdas32×32 con imagen16×16 y bordes replicados, point/clamp,
alpha SRC/INV_SRC. Se conservan caras interiores para verlas tras transparencia;
orden de caras aproximado y sin profundidad RE4. Una llamada de dibujo,43setters
restaurados para atlas. Recursos transitorios; sin cache GPU permanente.

Barrera opt-in de Leon: huella cuadrada radio250, altura1600, barrido horizontal,
deslizamiento/esquinas, salida si aparece una caja encima. Se corrige X/Z en los
callbacks existentes de pad/cámara primaria después de validar jugador/pose;
Y/velocidad/animación/inventario sin modificar. F8/TTL/gen/desarmado/renderer,
reentrada, movimiento>500 y pausa>100ms gestionados. No apoyo encima/gravedad,
saltos, enemigos/Ashley/proyectiles ni objetos de colisión del motor. Métricas
separadas48bytes con generación y contactos/correcciones; fallos actuales desarman.

CubeCommandv6/96352:3152previos +4uint32(collisions,arm_generation,tile_count,
reserved),128x6faces(tile+8UV) y64x256ARGBuint32. SHA de tiles no se persiste.
Style3=materialesMinecraft;0/1/2conservados, flags y fotos completas acotadas.
Host acepta pares(v2,419),(v3,421),(v4,422),(v5,423),(v6,424), rechaza capacidades
nuevas antes de activar input. CaptureIPCbuild ahora424 (version3/3296); Cube92/v1.

97Python,21core,20Minecraft,4suitesC pasan;9fixturesASan/UBSan. PNG/tinte/UV/crop,
RGBA/paleta/TCP/versión/retiro/cleanup, render/fallos y callbacks físicos cubiertos
con APIs simuladas. Instalación/respaldo/hash/retirada propios y diagnósticos pasan.
DLL PE32 importaKERNEL32/msvcrt/USER32; SHA256:
f75dc2d963d984fa7da1822aca08a95607bac0ab33bdcfdf14749be8fe68b3e4.
Nueva prueba real necesaria;0.4.23 intacta. Física completa del motor sigue pendiente.


## 0.4.25 — corrige fotograma estático, reutiliza escena y separa el movimiento

El usuario confirmó colisión con piedra, pero materiales mostraban el damero y
la construcción podía desaparecer al mantener W. La prueba con SpriteContents
real reproduce la causa del damero: getDistinctFrameCount devuelve IntStream.of(1)
cuando animation es null. Ese 1 se trataba como índice de segunda imagen. Ahora
los sprites estáticos usan cero; los animados su primer índice declarado.

Se toma NativeImage ya decodificada mediante access widener de solo acceso;
no se reabre PNG ni se consulta GPU, y nunca se modifica/cierra la imagen prestada.
ABGR→ARGB, tinte y composición conservados. Pruebas con BakedQuadFactory real
para las seis caras, sprite generado sin recurso PNG y animación declarada.
Errores por cara quedan en status y log acotado. Snapshot/JSON se reutilizan
cuando la escena no cambia; formas/estado, estilo, origen, altura, colisión y
recarga invalidan. Cada reconstrucción recicla la paleta de materiales retirados.

Input temporal inerte mientras arm/pending controlan RE4; las keybindings crudas
siguen siendo las que envían W/A/S/D/correr. No cambia coordenadas/velocidad ni
pausa el servidor; ratón y uso de items continúan. F8/chat/foco/stop/desconexión
restauran el Input previo y no sobrescriben reemplazos de otro componente.
walking on/off ofrece comportamiento conjunto anterior, desarmando al cambiar.

No se confirmó que los chunks provocaran la desaparición. Las extracciones PNG
fallidas se repetían antes por cara/tick y se elimina esa carga. Se mantienen las
caducidades de input200ms/nativo500ms y rechazo gráfico/colisión. No se cambiaron
ABI, hooks, física nativa, instaladores ni DLL: SHA256 f75dc2d963d984fa7da1822aca08a95607bac0ab33bdcfdf14749be8fe68b3e4,
build424/v6. Se publica paquete completo 0.4.25 con JAR/core/host actualizados.
Prueba visual y de rendimiento real pendiente de Windows.


## 0.4.26 — coordenadas comunes, callbacks de cámara, barrera persistente y HUD

Usuario confirmó texturas0.4.25; el controlador fijo limita construcción y
mantener W permite atravesar bloques. Cámara aún desvía en reposo y salta con
A/D/W/S. Sugiere mapa auxiliar de cristal del inicio. No hay geometría RE4
extraída: se implementa map on/off para editar manualmente el auxiliar sin
transferir nuevas colocaciones, conservando las construcciones anteriores.

ControllerMapping fija escala/rotación/posición al armar; cajas usan centros
reales de las colocaciones, no la distancia arbitraria tres bloques. Transformación
inversa lleva el editor a la posición de Leon conservando referencia entre arms.
Incluye ojo1620 y desplazamiento de altura, creativo local, input temporal y
vuelo/noclip de editor restaurados al parar. Cliente antes/después del tick;
servidor con tarea acotada/fresca, sin paquetes de teleportación de rotación.
Cambiar mundo renueva el seguidor; salir de creativo detiene el control.

Cámara auxiliar ya no restaura/invalida la principal. Projection recalcula mirada
independiente/posición actual aunque pad haya restaurado la cámara anteriormente.
Giro FP acumulado completo para no mantener cola de30deg al girar rápido.
Modelo/armas se ocultan sin perder sus bits guardados con proyecciones repetidas.

Un comando temporalmente ilegible o gráfico no disponible borraba referencia
segura de colisión; se podía adoptar el punto penetrado y la salida permisiva lo
dejaba atravesar. Fallos breves ahora conservan referencia/edad; comprobación
adicional en projection existente. Saltos >500 y referencia >=500ms paran sin
arrastrar. No cambian input200ms/control500ms ni se escribe Y/velocidad/animación.

HUD v1/32792 separado: imagen256x32 local con fondo/selección GUI, partículas
planas de items y cuentas; mira nativa, solo FP. Sin imágenes de juego publicadas,
ni HP/hambre/mano/inventario completo. Captura/restauración D3DSBT_ALL, recursos
transitorios, generación/F8/TTL/dispositivo/reset. HUD no habilita active de la
escena para evitar colisión invisible. Helper de estado compartido conserva el
orden anterior de43 setters del atlas y agrega5 para mira. Instalar DLL nueva
build426; ABI control80/v2, escena96352/v6 y telemetrías conservados. Host acepta
v6/424 y v6/426. CameraTrace añade métricas de colisión/dibujo a20Hz; registry SHA.

Pruebas nuevas usan funciones reales extraídas de cámara/projection/pad/lectorHUD,
11 fixtures nativos, presión1999 frames con pérdidas intermitentes, dirección y
mirada independientes, mouse120deg inmediato, modelos restaurados, HUD48 fallos
+ texturas + recursos + flags/caducidad; transformatrices y ojo/altura, modos de
editor, GUI NativeImage real y TCP con HUD. Ver VALIDATION. Prueba Windows pendiente;
no se da la cámara, colisión ni seguimiento real por confirmados en la nube.
DLL SHA2560c26f90b1f1aff959a72a207c5e80b17c9943ccec924eb6b37e8a5030be9e35d.
