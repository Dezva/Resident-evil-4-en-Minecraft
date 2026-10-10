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
