# 0.4.42: escenas nativas, interacción y colisiones de enemigos

La prueba del usuario confirmó que, al salir de Pacífico, el hambre/vida se
comportan como Minecraft y cero corazones lleva a la muerte nativa de RE4.
El enlace de vida de 0.4.41 se conserva. Esta actualización aborda los otros
problemas: altura bajo techos, cinemáticas e interacción, y añade las primeras
colisiones de enemigos contra construcciones.

## Actualizar y controles

Cierra RE4, Minecraft, host y VIEW. Sustituye el JAR anterior por
**re4bridge-0.4.42.jar** y actualiza todos los archivos del paquete.
Hay una **DLL nueva, build442**: ejecuta UNINSTALL-RE4-CONTROLS y después
INSTALL-RE4-CONTROLS del paquete nuevo. El desinstalador reconoce las DLL
publicadas de 0.4.41 y 0.4.39/40; conserva DLL ajenas. No mezcles versiones.
Fabric API sigue siendo 0.116.6+1.21.1.

Abre ambas partidas, START-RE4-CONTROLS y VIEW. Preparar Minecraft → esperar la
confirmación → F9. Los cambios funcionan también con ambos juegos en ventana,
sin la presentación de VIEW.

| Tecla durante el puente | Función |
|---|---|
| R | Acción contextual de RE4: usar junto al aviso de puerta/recogida/interacción |
| E | Inventario real de Minecraft |
| Esc | Menú de Minecraft; conserva el puente al continuar |
| F5 | Alternar creativo/supervivencia, sin repetir Preparar |
| F8 | Terminar el puente y volver a RE4 normal |

R envía una pulsación por cada press, sin cambiar el foco ni simular teclas en
otra ventana. Mantenerla no repite acciones. No se envía desde un menú de
Minecraft. No se añade una tecla para saltar cinemáticas: Esc sigue perteneciendo
al menú de Minecraft. RE4 continúa su simulación mientras ese menú está abierto.

## Supervivencia y altura

Preparar en supervivencia, Mantener modo si ya estás en supervivencia, o F5 para
entrar en supervivencia cambia **Pacífico a Normal**. Fácil, Normal y Difícil se
conservan. No se rellenan corazones, comida ni objetos al cambiar de modo.
La regeneración, saturación y comida siguen las reglas de Minecraft; los golpes
transferidos de RE4 también generan la extenuación habitual de recibir daño.

Mientras Steve sigue a Leon, conserva la pose y ojos de pie. Un techo de cristal
del mapa auxiliar no provoca que Minecraft lo convierta automáticamente en un
personaje agachado o nadando. F8 devuelve la física/pose normal de Minecraft.
Esto no cambia los límites de colocación contra la geometría de RE4.

## Cinemáticas

Durante películas, eventos, muerte y transiciones de puerta/carga, el puente
cede a RE4 su cámara, modelo e interfaz. No dibuja mano, barra ni cubos encima
de esas escenas, y mantiene a Steve quieto en su posición actual. Al terminar,
retoma primera persona con la misma referencia de construcción y sesión armada.
No fuerza movimiento durante la escena ni reanima a un actor muerto.

Las paradas reales por F8, conexión perdida, actor desconocido o fallo nativo
siguen exigiendo una nueva preparación/armado; no hay rearme automático.

## Colisiones de enemigos

Las construcciones frenan horizontalmente a los ganados humanoides habituales,
incluidos aldeanos, zelotes y soldados vivos en movimiento o reacción de daño.
Se utiliza el mismo barrido de cajas que para Leon, con un presupuesto acotado.
No se escriben altura, HP, animación, velocidad ni rutinas de los enemigos.

Es una primera extensión: **no añade navegación alrededor de una pared**, ni
impide ataques que atraviesen paredes. Jefes, enemigos voladores, colisión
vertical/subir sobre construcciones y armas Minecraft contra enemigos quedan
pendientes. Tampoco expulsa un enemigo que ya estaba dentro del volumen cuando
colocaste el bloque. Para comprobarla, construye la barrera antes de acercarlo.

## Prueba breve

1. Prepara supervivencia desde Pacífico: la dificultad debe quedar en Normal.
   Recibe golpes separados, come y comprueba hambre/vida compartida como antes.
2. Entra en la casa inicial y pasa bajo el mapa auxiliar: la vista debe mantener
   la altura de pie y permitir apilar bloques con la misma referencia.
3. Inicia una cinemática: debe verse la escena original de RE4. Al acabar, debe
   volver primera persona sin F9 ni nueva preparación. Comprueba también una
   transición de puerta y que la pantalla de muerte nativa sigue visible.
4. Junto a un aviso de acción de RE4, pulsa R. E debe seguir abriendo inventario.
5. Construye una pared de dos bloques de alto antes de atraer a un ganado vivo.
   Debe frenarse delante; comprueba también Leon y que F8 restaure el juego.

`/re4bridge status` añade escena, contactos/correcciones de enemigos y acciones
R. El host publica `runtime_state`, con disponibilidad de acción/enemigos.
Si algo falla, conserva esos datos y las líneas alrededor de `stop`/`reason`.

## Verificación y límites

Build Fabric/JDK21 y DLL PE32; pruebas de TCP autenticado, cambios de dificultad,
pose vanilla, escenas, acción de 64 bits, TTL/F8/generación y barridos repetidos.
Los fixtures nativos utilizan los callbacks reales con memoria y driver
sintéticos. No se ejecutaron RE4 ni un driver D3D9 real en cloud: el aspecto de
cinemáticas, los avisos de acción y la física de enemigos necesitan esta prueba
en tu Windows. La muerte compartida anterior sí está confirmada por tu prueba.

Referencias de estructuras y flags:
[re4_tweaks SDK@92c0208](https://github.com/nipkownix/re4_tweaks/tree/92c0208bd09c29c9640e13a2493b8e6bd6edd700/dllmain/SDK).
