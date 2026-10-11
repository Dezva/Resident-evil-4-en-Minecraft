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
