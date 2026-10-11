# 0.4.22 — caras grises para la construcción

Tu prueba de 0.4.21 confirmó la disposición de varias piedras y una losa, y que
romper una conserva las otras en su sitio. En 0.4.22 esa misma cuadrícula puede
mostrar **caras grises rellenas**, con tonos distintos en lados, arriba y abajo.
El modo de aristas verdes se conserva para comparar sin perder la construcción.

**Todavía no hay texturas ni colisión física de los bloques en RE4.** Las caras
se ordenan por distancia dentro de la cuadrícula, de forma aproximada. No usan
la profundidad del escenario de RE4: pueden aparecer sobre Leon, paredes u
objetos del juego. El relieve y la altura del suelo siguen sin calibrar. Son
limitaciones de este paso visual, no una prueba de física integrada.

## Actualizar desde 0.4.21

Cambian **DLL, host y mod de Minecraft**. El ZIP contiene todos los componentes,
START-RE4-CONTROLS y Fabric API. Conserva el paquete 0.4.21 para recuperación.

1. Cierra Minecraft, RE4, host, VIEW y las pruebas de cubo.
2. Ejecuta **UNINSTALL-RE4-CONTROLS del paquete 0.4.21**.
3. Extrae 0.4.22 en una carpeta vacía, o reutiliza la anterior después de retirar
   sus archivos y conservar una copia del paquete anterior. Ejecuta
   **INSTALL-RE4-CONTROLS de 0.4.22**. Verifica la copia de partidas como antes.
   Usa los instaladores; no sustituyas la DLL manualmente.
4. En `%APPDATA%\.minecraft\mods`, retira el JAR anterior del puente y copia
   **re4bridge-0.4.22.jar**. Deja solo un JAR de RE4 Bridge. Fabric API
   0.116.6+1.21.1, Fabric Loader 0.16.10 y Minecraft 1.21.1 siguen iguales.

## Prueba breve

1. Abre RE4 en ventana y carga una partida en una zona abierta. Abre Minecraft
   en ventana, mundo local creativo; mantenlo enfocado y sin minimizar. VIEW
   es opcional. Para comparar empieza sin ratón/firstperson.
2. Ejecuta **START-RE4-CONTROLS de 0.4.22**, con los mismos permisos que RE4,
   y cierra otros hosts/pruebas que ocupen el canal.
3. Usa **/re4bridge controller**, espera conexión, después
   **/re4bridge blocks on** y **/re4bridge arm**. Cierra el chat. Con la DLL nueva,
   `blocks on` selecciona caras grises por defecto.
4. Repite las dos filas de tres piedras y la losa. Comprueba caras visibles,
   media altura de la losa, anclaje al caminar/girar y retirada individual.
5. Para comparar usa **/re4bridge blocks style wire**, después **/re4bridge arm**.
   Deben quedar las mismas cajas en las mismas posiciones, con aristas verdes.
   **/re4bridge blocks style solid** y otra **arm** recuperan las caras grises.
6. Pulsa **F8** para detener control y dibujo. Otra `arm` conserva y vuelve a
   mostrar la construcción; `blocks off` la borra.

Envía una captura o un vídeo corto con piedras y losa, y la retirada de un bloque.
Si falta el dibujo, copia `/re4bridge status`, incluyendo modo, cajas, solicitud,
dibujos y causa. `blocktest` sigue siendo un diagnóstico de un único cubo de
aristas: para probar caras usa `blocks on` y coloca bloques.

Se conservan los límites de 64 bloques/128 cajas, las formas reales de Minecraft,
la referencia fija desde la primera colocación y la retirada de celdas cargadas.
Los chunks descargados no se consideran aire. `blocks height 250` y otra `arm`
suben el conjunto; `height 0` restablece la base. Ese ajuste no detecta el suelo.
Nueva `blocks on`, dimensión distinta o desconexión borra la escena. Solo se
transfieren colocaciones de esta sesión; el superplano, estructuras y mobs no.

## Comprobaciones y recuperación

Pasan **84 pruebas Python, 20 bridge-core, 16 Minecraft y 4 suites C portátiles**.
Siete fixtures nativos pasan ASan/UBSan con APIs simuladas. Incluyen proyección,
recorte cerca de la cámara y del borde de pantalla, orden de caras, losas,
retirada individual, límites, cambio de estilo, TCP autenticado, publicación
íntegra, rechazo de DLL antigua para caras, F8, caducidad y restauración de
estado gráfico ante fallos. DLL PE32 y JAR compilados; instalación, copia de
partidas, hashes, desinstalación propia y diagnósticos pasan con APIs simuladas.

La nube no ejecuta RE4 ni un controlador gráfico Windows: el arranque, apariencia
y rendimiento reales de 0.4.22 quedan pendientes de tu prueba. No se ha cambiado
la interceptación de creación de D3D9 que ya funcionaba en 0.4.21.

Para recuperar 0.4.21: cierra todo, desinstala usando **0.4.22**, instala usando
**0.4.21** y repón su JAR y host. F8 devuelve el control normal sin desinstalar.

Tras confirmar las caras, el siguiente paso visual son materiales/texturas y
ocultación por el escenario. Las colisiones requieren una etapa propia de
geometría del terreno y física antes de importar objetos o daño entre juegos.
