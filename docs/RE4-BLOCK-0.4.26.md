# 0.4.26 — editor sincronizado, cámara, HUD inicial y presión contra paredes

Las texturas de 0.4.25 fueron confirmadas por el usuario y se conservan. Esta
versión sustituye el controlador fijo por un editor creativo que sigue a Leon.
La primera `arm` fija una transformación común de posición, escala y orientación.
Las colocaciones usan sus coordenadas reales de Minecraft, no un cubo a tres
bloques delante de Leon. Los bloques permanecen donde se colocaron; moverte,
detener y volver a armar no cambia su referencia. El seguimiento incluye altura
y el origen del ojo en primera persona, incluyendo `blocks height`.

RE4 controla el desplazamiento y sus colisiones; Minecraft controla la mirada,
selección y edición de bloques. Mientras está armado, el avatar creativo de
Minecraft usa vuelo/noclip temporales para que su terreno y empujones no alteren
el origen de la mira. Se actualiza la posición antes/después del tick y en el
servidor local, sin paquetes de teleportación que cambien la rotación del ratón.
F8/chat/menú/foco/desconexión restauran el input, vuelo y noclip previos; vuelo
solo se restaura cuando el modo de juego lo permite. Salir de creativo detiene
el seguimiento. No funciona en servidores multijugador.

## Cambios de cámara y barrera

Las cámaras auxiliares ya no invalidan la primera persona. La cámara final se
recalcula desde la posición actual y una mirada independiente del giro de Leon,
incluso si se ejecutó otro callback de mando después de actualizar la cámara.
En primera persona se consume el giro acumulado del ratón completo, evitando la
cola anterior de pasos de 30 grados que desalineaba temporalmente ambas miras.
Altura inicial 1620 unidades con la escala provisional 1000.

Se corrige un fallo que permitía atravesar bloques al insistir con W: una lectura
incompleta de la escena o un pase gráfico no disponible borraban la posición
segura, y el siguiente callback podía adoptar una posición dentro del bloque.
Ahora esos fallos breves conservan la referencia y su edad. También se comprueba
la barrera antes de calcular la cámara final, después del movimiento del juego.
Una discontinuidad mayor500 unidades o una referencia demasiado antigua detienen
el puente sin arrastrar al jugador a otra zona. No se amplían los plazos de input
200ms ni el comando nativo500ms. La corrección sigue modificando solo X/Z.

## HUD inicial

En primera persona aparece una mira y una barra de nueve ranuras dentro de RE4,
sin paneles ni otra ventana. Selección, rueda/números y cantidades siguen el
inventario real de Minecraft. Fondo y selección vienen de los recursos GUI ya
cargados. Los iconos son **previsualizaciones planas del material del objeto**,
todavía no el render3D de los items de Minecraft. No incorpora corazones, hambre,
armadura, inventario completo, mano ni barras de daño compartidas. El HUD original
de RE4 continúa visible.

`/re4bridge hud off` desactiva esta capa; `hud on` la recupera, seguido de `arm`.
Solo se activa con primera persona. Se usa un canal separado v1/32792 bytes,
generación/TTL/F8 y el dispositivo D3D9 capturado. Estado completo capturado y
restaurado, recursos liberados en cada dibujo. Fallos gráficos detienen el HUD;
no se considera que dibujar el HUD habilite una barrera sin bloques visibles.
No se distribuyen imágenes originales de Minecraft en el ZIP.

## Actualizar desde 0.4.25

**Cambian DLL, JAR y host.** Conserva el ZIP anterior como recuperación.

1. Cierra RE4, Minecraft, host, VIEW y cualquier prueba/registro.
2. Ejecuta **UNINSTALL-RE4-CONTROLS del paquete 0.4.25**.
3. Extrae 0.4.26 completo y ejecuta su **INSTALL-RE4-CONTROLS**. Puedes reutilizar
   la carpeta después de desinstalar y retirar los archivos anteriores; no es
   obligatorio crear otra, pero evita mezclar versiones. No sobrescribas DLL ajenas.
4. En `%APPDATA%\.minecraft\mods` sustituye el JAR por **re4bridge-0.4.26.jar**,
   dejando solo uno del puente. Minecraft/Fabric/Fabric API siguen iguales.
5. Inicia **START-RE4-CONTROLS de 0.4.26**. RE4 en ventana y partida cargada;
   Minecraft en ventana, creativo local, enfocado y sin minimizar. VIEW/F9 opcional.

DLL SHA256: `0c26f90b1f1aff959a72a207c5e80b17c9943ccec924eb6b37e8a5030be9e35d`.
Build426, control v2/80 y escena v6/96352 conservados. Instaladores sin cambios.

## Prueba en orden

1. En Minecraft, empieza **de pie sobre una superficie** en una zona despejada;
   evita empezar volando muy alto. En RE4 usa una zona plana del inicio.
2. `/re4bridge controller`, espera conexión; `/re4bridge blocks on`.
3. `/re4bridge firstperson`, `/re4bridge blocks collisions on`, `/re4bridge arm`.
   Cierra el chat. Sync y HUD se habilitan por defecto; no hace falta `walking off`.
4. Coloca varios materiales. Camina, detente, alterna A/D y W/S sin mover el ratón.
   La mirada debería permanecer centrada. Después gira solo con el ratón.
5. Construye una pared y mantén W **15 segundos**, luego prueba correr y diagonales.
   Comprueba que no atraviese ni se desarme y que la mira siga estable.
6. F8; mueve a Leon con los controles normales. Vuelve a Minecraft y usa `arm`.
   Steve debe seguir su nueva posición y las nuevas colocaciones estar allí;
   las anteriores deben quedarse en su sitio. Prueba romperlas desde la mira.
7. Cambia ranura con rueda/números: la barra de RE4 debe actualizarse. F8 debe
   retirar dibujo/HUD/barrera y devolver los controles normales.

Si algo falla, copia `/re4bridge status`, que incluye referencia, sync, HUD,
contactos y correcciones. **TRACE-RE4-CAMERA** ahora registra también la barrera
y dibujo a20 muestras/segundo, con el temporizador habitual, para localizar un
paso a través de la pared junto a los saltos de cámara. Es de solo lectura.

`sync off` permite comparar con movimiento independiente de Minecraft;
`walking on/off` solo controla ese movimiento independiente cuando sync está off.
Para cambiar desde una cuadrícula antigua sin referencia sincronizada a sync on,
usa antes `blocks on`. Para otra zona de RE4 utiliza `blocks on` y vuelve a armar.

## Mapa auxiliar de cristal

La propuesta del usuario sirve como suelo y paredes **solo de Minecraft** para
construir con una referencia del escenario. No debe duplicar las colisiones de
las paredes reales de RE4. Se añade un modo de edición, sin modificar el mundo
ni construir un mapa automáticamente:

1. Fija primero la referencia con `blocks on` y `arm`, sin moverte de zona.
2. `/re4bridge map on` detiene el puente. Camina normalmente y construye el suelo
   y límites de cristal en Minecraft. **Estas nuevas colocaciones no se transfieren
   a RE4.** No uses este modo para borrar construcciones que quieras conservar.
3. `/re4bridge map off` y otra `arm` vuelven al editor sincronizado. Las colocaciones
   nuevas vuelven a transferirse, pudiendo apoyarse sobre ese mapa de cristal.

Todavía hace falta medir manualmente la zona inicial para alinearla con precisión;
no se ha extraído su geometría. Mientras sync está armado, los límites físicos
los impone RE4; el cristal sirve de superficie de edición y limita tu movimiento
normal al preparar el mapa. No es un recinto automático que frene a Leon.
El tamaño provisional de bloque es1000; limita la primera prueba a una zona
pequeña. Puedes bajar renderizado/simulación de Minecraft a4 chunks. El movimiento
carga los chunks necesarios, pero ya no hay un Steve caminando por su cuenta al
mantener W contra una pared que detiene a Leon.

## Límites, validación y recuperación

64 colocaciones/128 cajas/64 teselas. Barrera horizontal solo para Leon; no
incorpora apoyo sobre bloques, salto, colisiones de enemigos/Ashley/proyectiles,
ocultación por terreno RE4, combate ni sincronización de vida.

Pasan103 pruebas Python,24 bridge-core y27 Minecraft, sin fallos/omisiones.
Once fixtures nativos pasan ASan/UBSan y cuatro suites C portátiles.
Se prueban transformaciones rígidas/altura/ojo, TCP con HUD y caducidad, sprites
reales de Minecraft, modos temporales/restauración, orden real de callbacks de
cámara, 1999 actualizaciones de presión con fallos temporales de escena/gráficos,
HUD con48 fallos de setters y fallos de textura/restauración. Las APIs Windows/D3D
son simuladas: apariencia, arranque, seguimiento de cliente/servidor y respuesta
física reales requieren esta prueba en tu equipo. No se afirma que los fallos
visuales estén resueltos dentro de RE4 hasta verla.

Para recuperar 0.4.25: cierra todo, desinstala con0.4.26 e instala con0.4.25;
repón su JAR/host. F8 conserva el retorno normal durante la prueba.
