# 0.4.23 — textura experimental de piedra

Tus dos capturas de 0.4.22 confirman caras rellenas y aristas para la misma
construcción. Este paquete añade el modo **stone**, con una textura propia de
piedra de 16×16 píxeles, filtro de píxeles nítidos y sombreado de las caras.
Se conserva la referencia fija, la disposición de bloques/losas y la retirada
individual. `solid` y `wire` siguen disponibles para comparar.

Es una primera prueba del dibujo con textura: **el mismo patrón de piedra se
aplica a todas las cajas**, aunque coloques otro material. No reproduce aún las
texturas originales de Minecraft ni los resource packs. Cada cara de cada caja
usa la tesela completa; los laterales de una losa pueden verse estirados. Las
texturas por material/cara y el recorte de la tesela según la forma siguen pendientes.
La textura se genera en código propio; no se distribuyen recursos de los juegos.

Todavía no hay colisión física de los bloques en RE4, ni ocultación por su suelo,
paredes, Leon u objetos. Las caras pueden superponerse al escenario. El orden
entre caras de la cuadrícula sigue siendo aproximado. Esta versión no mejora
las pendientes o la altura del suelo ni cambia el control de la cámara.

## Actualizar desde 0.4.22

Cambian **DLL, host y JAR**. El ZIP contiene los componentes completos, ambos
mods, instaladores, START-RE4-CONTROLS y herramientas de diagnóstico.

1. Conserva 0.4.22 para recuperación y cierra ambos juegos, host, VIEW y pruebas.
2. Ejecuta **UNINSTALL-RE4-CONTROLS del paquete 0.4.22**.
3. Extrae 0.4.23 en una carpeta vacía, o retira los archivos anteriores después
   de conservar la copia y desinstalar. Ejecuta **INSTALL-RE4-CONTROLS de 0.4.23**.
   El instalador verifica la copia de partidas como antes; no sustituyas DLL manualmente.
4. En `%APPDATA%\.minecraft\mods` retira el JAR anterior del puente y copia
   **re4bridge-0.4.23.jar**. Deja solo un JAR de RE4 Bridge. Fabric API
   0.116.6+1.21.1, Fabric Loader 0.16.10 y Minecraft 1.21.1 siguen iguales.

## Prueba de textura y comparación

1. Abre RE4 en ventana y carga una partida en una zona abierta. Abre Minecraft
   en ventana, mundo local creativo, enfocado y sin minimizarlo. VIEW es opcional.
   Comienza sin mouse/firstperson para comparar con tus capturas anteriores.
2. Inicia **START-RE4-CONTROLS de 0.4.23**, con los mismos permisos que RE4,
   y cierra otros hosts o pruebas que ocupen el canal.
3. Usa **/re4bridge controller**, espera la conexión, después
   **/re4bridge blocks on** y **/re4bridge arm**. Cierra el chat. El modo por
   defecto con la DLL nueva es `piedra`.
4. Repite tu pared y losa. Debes ver el patrón pixelado, conservando los tamaños.
   Camina y gira alrededor para comprobar que la textura sigue adherida a cada cara,
   incluso al acercarte. Rompe una piedra: las demás deben permanecer fijas.
5. Usa **/re4bridge blocks style solid** y después **/re4bridge arm** para comparar
   con las caras grises. `blocks style wire` recupera las aristas, también con otra arm.
   **/re4bridge blocks style stone** y otra **arm** recuperan la textura.
6. Pulsa **F8**: dibujo y controles se detienen. Otra `arm` conserva la construcción;
   `blocks off` la borra. Comprueba que RE4 vuelve a su manejo normal.

Envía una captura o vídeo corto donde se vea el patrón al girar y acercarte.
Indica si notas una caída de rendimiento respecto a `solid`. Si falta el dibujo,
copia `/re4bridge status`, incluyendo modo, cajas, solicitud, dibujos y causa.
Si una operación gráfica falla, puede ser necesario otra `arm` después de cambiar
al modo `solid`; un fallo de restauración exige reiniciar RE4.

La escala inicial sigue siendo1000 unidades RE4/bloque. `blocks height 250` y
otra arm suben la construcción; `height 0` restablece la base. Es un ajuste
visual y no detecta el suelo. Se mantienen los límites64 bloques/128 cajas,
las formas de Minecraft, los chunks descargados y el borrado al desconectar,
cambiar dimensión o empezar otra `blocks on`. No se transfieren mobs, estructuras
ni suelo del superplano. `blocktest` sigue mostrando solo el cubo de diagnóstico
sin textura: para este ensayo usa `blocks on` y coloca bloques.

## Validación y recuperación

Pasan **88 pruebas Python, 20 bridge-core, 16 Minecraft y 4 suites C portátiles**.
Siete fixtures de helpers nativos reales pasan ASan/UBSan con APIs simuladas.
Se comprueban UV interpoladas antes del recorte, perspectiva, geometría acotada,
textura opaca y variada, copia con pitch distinto al ancho, máximos, liberación
de recursos y estado completo restaurado ante fallos. Cobertura de38 fallos de
setters para textura y30 para los otros estilos, creación/LockRect/UnlockRect,
Capture/Apply/Draw/heap, F8/TTL, retirada individual, TCP, rechazo de DLL anterior,
y desactivación al cerrar. DLL PE32 y JAR compilados; instaladores y diagnósticos
pasan con partidas/APIs simuladas. Paquete y hashes comprobados.

La textura se crea solo para una escena armada y visible, y se libera en la misma
llamada después de restaurar el estado. Esto evita recursos propios pendientes
entre frames/Reset; el coste real de crear una tesela por dibujo necesita prueba
en tu driver. No se han modificado la interceptación de creación ni los hooks
que ya funcionaban. La nube no ejecuta RE4 ni un driver D3D9 real: arranque,
apariencia y rendimiento de0.4.23 requieren tu prueba en Windows.

Para recuperar0.4.22: cierra todo, desinstala con **0.4.23**, instala con **0.4.22**
y repón su JAR y host. F8 permite volver al control normal sin desinstalar.

Tras confirmar este dibujo texturizado, quedan materiales por bloque/cara y
profundidad del escenario; la integración física y del terreno requiere una
etapa propia antes de entidades, TNT o daño cruzado.
