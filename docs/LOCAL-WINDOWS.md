# Continuidad local en Windows — 16 de octubre de 2026

Esta auditoría recupera el proyecto existente; no cambia mecánicas ni publica
una nueva versión del mod. Los resultados locales se guardan en
`D:\logs\re4-local-audit-20261016`. No se añaden logs privados, tokens,
partidas, ejecutables ni geometría de los juegos a Git.

## Referencia recuperada

- El checkout inicial estaba limpio en `main`, commit `e20d988` (0.4.45).
  `git fetch origin --prune` recuperó/verificó las ramas posteriores.
- Los avances 0.4.46–0.4.55 están publicados en ramas distintas de `main`.
  La última punta es `b3440e2`,
  `origin/fix/re4bridge-0.4.55-runtime-stability`.
- La rama `codex/windows-local-audit` parte de esa punta y conserva su historial.
  No se ha fusionado ni sobrescrito `main`.
- El usuario confirma que la última versión jugada es 0.4.54. La 0.4.55 sigue
  pendiente de validación en una partida.
- Al empezar estaban el JAR0.4.54 y la DLL retirada por el usuario. Durante la
  auditoría el usuario actualizó: la segunda lectura confirma un solo JAR0.4.55
  y DLL455 con SHA256
  `e034d92c2e4e79732da6f66efce630dced46003e80edec2e08f0653c0e3ad362`.
  El agente no instaló ni retiró archivos de los juegos.
- Los ZIP0.4.54/55 descargados coinciden por SHA256 con los publicados en Git.

## Estado funcional que se conserva

Las guías recientes y la evidencia histórica documentan primera persona,
movimiento/carrera, bloques con materiales y colisión, construcción/minería,
inventario/mano/HUD vanilla, supervivencia y vida compartida, salto/apoyo,
acciones contextuales y cesión de cámara durante escenas nativas.

La lectura exclusiva de `main` daba un estado antiguo del combate: 0.4.51 añadió
cuerpo a cuerpo; 0.4.52/53 conectaron los impactos y las reacciones nativas, cajas,
cuervos y diálogos. La guía0.4.54 registra confirmación del usuario de muertes y
cajas rotas. Desde0.4.49 el salto también aplica altura al actor real de Leon;
la limitación de altura exclusivamente visual de0.4.45 es histórica.

0.4.55 añade trampas/bombas, guardia de desplazamiento horizontal, motivos
de parada diferenciados y límite de renderizado Minecraft durante el control.
Son cambios compilados/probados en cloud, todavía no confirmados jugando aquí.

Siguen pendientes la estabilidad prolongada de0.4.55, transición continua y
construcciones separadas por zona, navegación de enemigos alrededor de bloques,
cobertura de ataques a través de paredes y más tipos de actores. Arco/ballesta,
otras vías de daño entre juegos y vuelo creativo libre tampoco se consideran
resueltos. No se debe reiniciar ni simplificar la integración a un reemplazo visual.

## Capacidades comprobadas

| Capacidad | Evidencia y límite actual |
|---|---|
| Windows local | Windows10 build19045, PowerShell7.6.5, rutas y procesos locales reales |
| Procesos | `Get-Process` funciona; `find_pid()` usa Toolhelp Win32 y confirmó cero `bio4.exe` al consultar. Minecraft/RE4 no estaban activos durante estas comprobaciones |
| Instalación/configuración | Lectura de `bio4.exe`, DLL, metadatos JAR y configuración Minecraft. Se comprobó presencia de token sin mostrar su contenido |
| Python | Runtime local3.12.14 utilizable directamente; no depender del alias de Microsoft Store |
| Java/Gradle | JDK Microsoft21.0.7 completo en el runtime de Minecraft; wrapper8.11.1 descargado con checksum fijado, compilación del core completada |
| C#/PowerShell | Tres suites existentes pasan con Windows PowerShell5.1; VIEW y declaraciones interop compilan |
| C nativo | No se encontró GCC/Clang/MinGW utilizable en PATH. La recompilación de DLL y fixtures C queda pendiente de preparar toolchain; no se ejecutaron los instaladores Linux |
| Memoria/IPC reales | Existen lectores de proceso/IPC; falta probar permisos y lecturas con RE4 abierto. Las pruebas sintéticas no lo sustituyen |
| Captura de ventanas | `computer-use` enumeró ventanas reales. Dos intentos de capturar WinRAR fallaron por timeout; captura de RE4 y grabación no verificadas |
| Información del sistema | CIM devolvió acceso denegado. No hizo falta solicitar privilegios de administrador |

Gradle usa caché ignorada `.gradle/local-user-home`, un worker, heap512MiB
para su proceso de build y `--no-daemon`. No se abrieron juegos ni herramientas
gráficas para compilar. El build completo Fabric/remap y los tests Minecraft
no se ejecutaron en esta auditoría; continúan como evidencia cloud, no local.

## Resultado de las pruebas locales

| Prueba | Resultado |
|---|---|
| Python sin módulos que requieren GCC | 157 ejecutadas:156 pasan,1 falla,0 omitidas dentro de la selección |
| `:bridge-core:test` inicial | 57 ejecutadas:53 pasan,4 fallan; fuentes principales y tests compilan |
| Repetición de la prueba Java/Python del simulador con `PYTHONUTF8=1` | Pasa; no equivale a repetir toda la suite |
| `tools/test_view.ps1` | Pasa con desktop simulado y preparación autenticada por loopback |
| `tools/test_diagnostic.ps1` | Pasa; declaraciones Win32 y lectura IPC simulada |
| `tools/test_crash_report.ps1` | Pasan cuatro fixtures de eventos; no consulta eventos reales en este test |

Los cuatro módulos Python no ejecutados son `test_native_capture`,
`test_native_block_discovery`, `test_native_block_render` y
`test_native_frame_read`. Requieren GCC; no se cambió su código para omitirlos.

Hallazgos reproducidos sin cambiar código:

1. `test_re4_import_audit` falla con `seconds=0`: `Budget.get` compara
   `time.monotonic() > deadline`. En este Windows el reloj reporta resolución
   15.625ms y varios accesos pueden tener la misma hora. Los límites por bytes
   y número de lecturas sí rechazan. Corregir la expiración y añadir/revisar una
   comprobación determinista de igualdad será un cambio pequeño independiente.
2. El simulador Python redirigido escribe con la codificación Windows; Java lee
   UTF-8. La espera de `READY` falla al no poder decodificar la línea. Habilitar
   `PYTHONUTF8=1` en el entorno hace pasar esa prueba sin cambiar el protocolo.
3. Tres tests de `ControlTests` fallan al limpiar logs bloqueados por el proceso
   fixture. Sus `finally` llaman `destroyForcibly()` sin esperar a que termine.
   Debe esperarse la terminación antes de la limpieza JUnit, conservando timeout.

El `latest.log` existente corresponde a Minecraft1.21.1/Fabric Loader0.19.5 y
mod0.4.54. Registra paradas nativas1 con foco verdadero y pérdidas de foco;
stop1 antiguo no distingue una causa única. También contiene un error de Realms,
que no demuestra un fallo del puente offline. No se encontraron logs host/VIEW
recientes en las rutas del proyecto/instalación inspeccionadas; no se afirma
que no existan en otra carpeta de ejecución del paquete.

## Diagnósticos que puede ejecutar el agente

| Necesidad | Herramientas existentes y forma de automatizar |
|---|---|
| Inventario/versiones | Procesos, rutas, hashes, metadatos JAR, logs y configuración con token excluido |
| Pose y firmas | `re4_reader.py --once`, `re4_native_probe.py`; captura de stdout a una carpeta de prueba |
| Cámara y cortes | `re4_camera_trace.py --seconds 15 --output ...`; IPC solo lectura, muestra acotada y análisis posterior |
| Renderizado/colisión | `re4_graphics_capture.py --output-dir ...`, `read_cube_status`, `read_collision_status` y `NativeDepth(readonly=True)` |
| Entidades/objetos | Función `snapshot` de `re4_object_probe.py`, con JSON guardado directamente en la carpeta de prueba |
| Geometría local | `re4_map_export.extract`/`write_outputs` con destino explícito local, solo cuando cambie la zona o falte evidencia |
| Fallos | Lectura de logs rotados del host, `latest.log`, `view-events.log` y recolector de eventos filtrado a RE4 |
| Regresión | Tests Python/Java/C#/PowerShell seleccionados por componente; C cuando esté disponible el compilador |

No usar `NativeControl`, `NativeRuntime`, `NativeCombat`, `NativeHealth` ni
`NativePreview` como lectores pasivos: abren mappings con escritura. Para observar
esos reportes se necesita acceso `FILE_MAP_READ` y sus decodificadores existentes.
`TEST-RE4-CUBE`, el host `--control` y VIEW alteran comportamiento/ventanas;
no forman parte de una auditoría pasiva.

Varios CLI antiguos guardan en la raíz del paquete. Para respetar `D:\logs`,
usar sus opciones de destino donde existan o llamar a las funciones de lectura
existentes y guardar allí el resultado. Antes de un ciclo de juego conviene
añadir un destino configurable al log del host y de VIEW: hoy usan rutas
relativas al paquete. No duplicar extractores ni ejecutar sondas amplias si la
telemetría acotada o un informe anterior ya responden la pregunta.

## Orden propuesto para continuar

1. Adaptar la ejecución de tests a Windows: UTF-8, cierre esperado de fixtures y
   expiración de presupuesto. Repetir solo las pruebas afectadas y después core.
   Preparar un compilador x86 portátil/local antes de modificar C; no se promete
   que el script Bash pensado para cloud funcione sin adaptación en Windows.
2. Añadir una única entrada de diagnóstico pasivo, con destino por prueba bajo
   `D:\logs`, duración máxima, detección de procesos, lectura IPC y resumen.
   Reutilizar lectores existentes y permitir detenerla sin activar controles.
   Hacer configurables los destinos de host/VIEW para el ciclo habitual.
3. Validar0.4.55 ya instalada, manteniendo0.4.54 publicada como referencia de
   recuperación. Primero observar host/IPC/versiones; el agente no reinstala
   DLL ni arranca controles como parte de la observación. Cualquier operación
   sobre instalación o que pueda cerrar/bloquear el juego necesita aviso y,
   para DLL, autorización expresa del usuario.
4. Prueba humana breve, en partida offline: caminar por escaleras, construir
   una columna de tres bloques y bajar; pared de tres bloques ante un ganado
   durante10s; un golpe cuerpo a cuerpo y un diálogo de recogida. Terminar con
   F8. El agente recoge/analiza logs e IPC; no pedir ejecutar scripts separados.
   Probar trampa/bomba por separado después de establecer esa referencia.
5. Si el ciclo es estable, abordar transiciones y propiedad de construcciones
   por zona antes de ampliar combate. Definir primero una transición concreta,
   identidad de zona, invalidación del mapa/actores y comportamiento seguro de
   los bloques existentes. Arco/ballesta y otras funciones quedan en etapas aparte.

Para cada cambio: revisión de evidencia, modificación pequeña, compilación y
tests del componente, análisis, documentación en MODLOG y commit publicado.
Toda actualización que necesite una prueba del usuario debe entregarse con un
enlace de descarga directa del paquete versionado en GitHub, acompañado de su
versión, cambios y prueba breve. Un enlace al commit no sustituye al paquete.
Publicar únicamente código/documentación y artefactos propios verificados.
F8, TTL, generación, validación de identidad y recuperación permanecen requisitos;
no se permiten escrituras exploratorias ni modificaciones de ejecutables/saves.

## Elevación de RE4 — diagnóstico y corrección autorizada

La revisión posterior encontró `RUNASADMIN` para la ruta documentada de
`bio4.exe` bajo
`HKLM\Software\WOW6432Node\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers`.
El manifiesto embebido del EXE declara `asInvoker`; el acceso directo público
apunta a ese EXE y no lleva el flag de ejecución elevada. START-RE4-CONTROLS
tampoco solicita elevación por sí mismo. La diferencia de privilegios puede
impedir que el host acceda al proceso o a los canales creados por la DLL.

Con autorización expresa del usuario se retiró únicamente ese valor, después
de guardar una copia restaurable y validar su hash. La operación necesitó UAC
una vez; no se cambió UAC global, ningún otro valor de compatibilidad, el EXE,
la DLL ni las partidas. Evidencia/restauración en
`D:\logs\re4-privileges-20261016`. La lectura posterior confirma el valor ausente.

El usuario informó que UAC seguía apareciendo. Se encontró además el mismo
`RUNASADMIN` en HKCU del usuario real. La primera revisión de HKCU había usado
la cuenta aislada del sandbox de comandos, aunque USERPROFILE apuntaba a la
carpeta habitual. Se guardó otra copia restaurable y se retiró exclusivamente
el valor de la ruta actual, sin UAC; se conservó una entrada de otra instalación.
La comprobación final, ejecutada como el usuario real, confirma ausencia del
ajuste para esa ruta en HKCU/HKLM y ambas vistas32/64. El hash del EXE no cambió.
Para futuros diagnósticos por usuario, comprobar la identidad/SID del proceso
de consulta; USERPROFILE por sí solo no identifica el hive HKCU consultado.

La prueba pendiente es iniciar RE4 y el host normalmente y comprobar el token
de proceso y el acceso real; retirar el ajuste no demuestra por sí solo que
no exista otro problema de permisos o compatibilidad.

El toolchain de cloud no se trasladó a Windows: los scripts de instalación C
actuales usan rutas `/workspace`, paquetes Debian y apt/dpkg. El código está
conservado en Git; preparar un toolchain local y adaptar los fixtures necesarios
es independiente del juego. La prueba del simulador ya pasa con `PYTHONUTF8=1`;
no exige instalar un simulador nuevo ni modificar RE4.

Para verificar el arranque habitual hay que usar también el intérprete del BAT:
en este equipo `python` abre Python3.13 de Microsoft Store; el runtime de Codex
es3.12. Sus cachés son distintas. Hostfix1 conservaba tamaño/fecha del paquete
original, permitiendo reutilizar bytecode incorrecto en3.13. Hostfix2 invalida
esa caché y añade una prueba de extracción/importación con el Python real.
Evitar atribuir un error de versión IPC a privilegios sin comparar primero
cabecera observada, constantes importadas, intérprete y fuente efectivo.

## Continuación0.4.56

El usuario confirmó el arranque normal y el gameplay estable de0.4.55-hostfix2,
incluidas trampas de osos y bombas. No se restaura RUNASADMIN. Compilador C
local preparado en .gradle/native-toolchain (ignorado), archivo oficial verificado
por SHA256, sin instalación ni cambios de PATH persistidos. build_native.ps1
compila x86 con declaraciones Win7 y SSE2, sin DLLs de GCC adicionales.
Loom/Fabric/Minecraft se resuelven en la caché local del proyecto; JDK21 y
Gradle se usan con un trabajador y heap768MB, tras cerrar el usuario los juegos.

Fixtures C de cámara adaptados a memoria32bits de Windows y calling convention
del motor; cálculo float de fixtures usa SSE2. Limpieza de ControlTests espera
el cierre del hijo antes de borrar sus logs; auditoría Python considera agotado
un presupuesto temporal cero aun si monotonic repite el mismo valor.
Resultados privados en D:\logs\re4-0.4.56-20261016. Las pruebas automatizadas no
equivalen a validación visual, daño/foco de gameplay real ni medición de FPS.
