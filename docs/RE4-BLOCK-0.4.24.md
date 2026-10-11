# 0.4.24 — texturas de Minecraft y barrera horizontal para Leon

Confirmaste que la textura de0.4.23 permanecía adherida. Ahora el mod obtiene
las imágenes de los recursos de tu Minecraft y las envía por bloque y cara:
piedra, tierra, tablones, laterales/extremos de troncos, hierba y losas pueden
usar sus imágenes correspondientes. Se aplican tintes de Minecraft y las losas
recortan la imagen según su forma. Los modelos usan la semilla de su posición
para orientación/variantes. También se conserva transparencia, con orden visual
aproximado. La tesela se reduce a16×16 si el recurso tiene mayor resolución.
Las animaciones usan su primer frame; no se animan todavía en RE4.

**Ninguna imagen original de Minecraft se añade al ZIP o a GitHub.** Se leen los
PNG activos en tu equipo, incluyendo los resource packs compatibles. Una recarga
de modelos reconstruye la paleta al siguiente tick. Modelos generados sin PNG,
entidades de bloque o formas especiales pueden quedar incompletos: las caras sin
material aparecen en damero magenta y el indicador `sin tex` cuenta esas caras.
La geometría continúa usando cajas de las formas de colisión, no toda la malla
visual de Minecraft. Plantas sin forma de colisión no se transfieren.

## Qué colisión incluye

Una **barrera horizontal experimental para Leon**, opcional y desactivada al
iniciar `blocks on`. Usa las cajas de la construcción y una aproximación al
cuerpo: radio250 y altura1600 unidades RE4, con huella cuadrada conservadora.
Impide cruzar paredes y permite deslizarse por sus lados. Comprueba el recorrido,
no solo el punto final. Si colocas una caja donde está Leon, permite salir para
no encerrarlo. Romperla retira su barrera junto al dibujo.

**Todavía no permite subirse/apoyarse sobre los bloques, saltar como Minecraft,
ni bloquea enemigos, Ashley, armas o proyectiles.** No registra objetos físicos
en el motor RE4 ni cambia su suelo: corrige solamente X/Z de Leon dentro de los
callbacks existentes, conservando Y, velocidad, animación e inventario. La
estabilidad con la física real necesita tu prueba. No corrige pendientes ni la
altura inicial aproximada de la construcción. Tampoco hay ocultación por suelo,
paredes u objetos RE4: los bloques pueden superponerse al escenario.

La corrección solo actúa con controles y escena armados/frescos, generación
coincidente, cámara/jugador validados y renderizado disponible. F8, desarmado,
pérdida de dibujo o caducidad la detienen. Una pausa mayor100ms, cambio de jugador,
origen/generación o movimiento mayor500 unidades reinicia la referencia sin
arrastrar al jugador. Antes de escribir se comprueba que el personaje y su pose
siguen siendo los observados. No instala otro hook de juego.

## Actualizar desde0.4.23

Cambian **DLL, host y JAR**. Conserva0.4.23 para recuperación.

1. Cierra RE4, Minecraft, host, VIEW y las pruebas de cubo.
2. Ejecuta **UNINSTALL-RE4-CONTROLS del paquete0.4.23**.
3. Extrae0.4.24 en una carpeta vacía, o retira la versión anterior después de
   conservarla y desinstalar. Ejecuta **INSTALL-RE4-CONTROLS de0.4.24**. El
   instalador verifica la copia de partidas como antes. No sustituyas DLL manualmente.
4. Sustituye el JAR del puente por **re4bridge-0.4.24.jar** en
   `%APPDATA%\.minecraft\mods`, dejando solo un JAR de RE4 Bridge. Fabric API
   0.116.6+1.21.1, Fabric Loader0.16.10 y Minecraft1.21.1 siguen iguales.

## Prueba de materiales

1. RE4 en ventana, partida cargada en una zona abierta; Minecraft en ventana,
   mundo creativo local, enfocado y sin minimizar. VIEW es opcional.
2. Inicia **START-RE4-CONTROLS de0.4.24**, con los mismos permisos que RE4 y sin
   otros hosts o pruebas ocupando el canal.
3. Usa **/re4bridge controller**, espera conexión, después **/re4bridge blocks on**
   y **/re4bridge arm**. Cierra el chat. El modo por defecto con la DLL nueva es
   `Minecraft`; la colisión permanece desactivada.
4. Coloca piedra, tierra, tablones de roble, un tronco, hierba y una losa de piedra.
   Comprueba imágenes distintas, caras laterales/superiores del tronco y hierba,
   recorte de la losa y retirada individual. Puedes añadir vidrio para observar
   transparencia, teniendo presente que el orden de caras sigue siendo aproximado.
5. Camina/gira y compara `blocks style solid` o `blocks style wire`, con otra
   `/re4bridge arm` después de cada cambio. **/re4bridge blocks style minecraft**
   y otra arm recuperan los materiales. `style stone` conserva la prueba antigua.

## Prueba de barrera para Leon

1. Construye una pared de piedras de tres bloques de ancho y dos de alto, en una
   zona lo más plana posible, sin encerrar al jugador.
2. Usa **/re4bridge blocks collisions on**, después **/re4bridge arm** y cierra el
   chat. El indicador debe mostrar `col=si` tras dibujarse la escena.
3. Camina contra ella con W y después prueba correr. Leon debe detenerse ante la
   pared; retrocede con S y prueba acercarte en diagonal o deslizarte por un lado.
   No pruebes aún subir encima: la respuesta vertical no está implementada.
4. Rompe una piedra por la que pueda pasar el cuerpo: esa zona debe liberarse.
   El margen del cuerpo puede impedir atravesar un hueco demasiado estrecho.
5. **F8** debe detener dibujo, control y barrera, devolviendo RE4 al manejo normal.
   Para continuar sin barrera usa **/re4bridge blocks collisions off** y otra arm;
   esto conserva la construcción. `blocks off` borra todo.

Envía un vídeo corto de los distintos materiales y otro acercándote/retrocediendo
ante la pared. Indica si hay temblores, retrocesos o caída de rendimiento. Copia
`/re4bridge status` después: incluye contactos y correcciones nativas. Abrir el
chat desarma, por lo que el flag activo puede estar apagado en ese momento; los
contadores de contactos se conservan. Si algo se comporta mal, F8 devuelve el
control normal; puedes dejar `collisions off` y seguir probando texturas.

Mantén los límites64 bloques/128 cajas y64 teselas totales (una es el damero).
Se rechazan nuevas colocaciones complejas como antes. Una paleta llena muestra
el damero en materiales nuevos; `blocks off/on` borra y reinicia la construcción
junto a su paleta. Se conserva lo que queda en chunks descargados. Dimensión,
desconexión o nueva `blocks on` borran la escena. Usa otra `blocks on` al cambiar
zona RE4, para fijar la referencia nueva. Solo se transfieren tus colocaciones;
no el superplano, estructuras ni mobs. `blocks height` ajusta la altura visual.

## Validación y recuperación

Pasan **97 pruebas Python,21 bridge-core,20 Minecraft y4 suites C portátiles**.
Nueve fixtures nativos pasan ASan/UBSan con APIs simuladas. Se verifican paleta,
RGBA, UV/crop/tinte/composición/transparencia, límites, TCP autenticado, publicación
completa, eliminación, rechazo de versiones anteriores, recursos gráficos,
43 fallos de setters para atlas y restauración completa. La colisión comprueba
paredes/barrido/deslizamiento/esquinas/rotación/altura/salida, pausas/transiciones,
F8/TTL/generación, memoria/puntero/pose cambiantes, reentrada y conservación de Y.
JAR/DLL PE32 compilados; instaladores/respaldo/diagnósticos propios pasan simulados.

La nube no ejecuta RE4 ni un driver D3D9 real. La extracción desde modelos/recursos
activos, arranque, apariencia, rendimiento y respuesta física real requieren tu
prueba en Windows. Se conserva el camino de creación de D3D9 y los hooks previos.
La textura256×256 se libera en cada dibujo tras restaurar el estado; su coste
real debe compararse con `solid`. No hay recursos GPU propios persistentes.

Para recuperar0.4.23: cierra todo, desinstala con **0.4.24**, instala con **0.4.23**
y repón su JAR y host. F8 sirve para volver al control normal sin desinstalar.
El siguiente trabajo físico necesita apoyo vertical/gravedad y las colisiones
del motor RE4 para otros actores; esta barrera no acredita esa integración.
