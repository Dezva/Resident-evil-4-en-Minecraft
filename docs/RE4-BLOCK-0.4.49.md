# 0.4.49: altura absoluta y posición real de León

El video del usuario muestra que 0.4.48 todavía expulsa al jugador al moverse
sobre una columna. En el registro, la altura nativa del suelo cambia unos
62 puntos mientras la elevación se expresa como un desplazamiento relativo.
Los dos textos adjuntos son copias del mismo registro. No hay correcciones de
enemigos en ese tramo: el fallo también ocurre sin ganados.

## Cambios

El mod envía la Y absoluta de los pies en la misma cuadrícula fija que los
bloques. Cámara y colisión dejan de sumar el salto a un suelo nativo que cambia
entre paquetes. Una regresión reproduce la pendiente antes de la corrección y
comprueba movimiento lateral estable, una pared elevada y salir del apoyo.

A petición del usuario, esta versión aplica esa altura también a la posición
real de León en RE4. Las versiones anteriores solo elevaban la vista y los pies
virtuales del puente. La cámara interpola entre extremos de 20 Hz; el actor y
las colisiones usan el último extremo físico validado.

La simulación consulta los suelos y pendientes del mapa nativo para aterrizar
en plataformas de RE4 al pasar desde una columna. No interpreta la posición
real elevada que vuelve por el lector como un nuevo suelo. Se conserva la
caída al quitar un bloque o salir del borde. Un techo limita la subida y la
falta de cobertura del mapa aplaza el movimiento vertical desconocido.

El host registra `actor_y`, `vertical_active` y `vertical_writes`, publicados
por la DLL después de aplicar la altura. Permiten distinguir una solicitud
del mod de una escritura realizada por el módulo nativo.

## Actualizar

1. Cierra ambos juegos, el host y VIEW. Extrae el paquete completo nuevo.
2. Deja solamente `re4bridge-0.4.49.jar` y Fabric API en los mods de la instancia.
3. Ejecuta `UNINSTALL-RE4-CONTROLS.bat` y después `INSTALL-RE4-CONTROLS.bat`
   del paquete nuevo. Se reconocen las DLL 447 y 448 publicadas por checksum.
4. Abre los juegos, el host nuevo y VIEW; prepara Minecraft y pulsa F9.

DLL 449, host y JAR deben actualizarse juntos: Runtime v4 tiene 128 bytes.

## Comprobar en RE4

Repite el movimiento sobre la columna del video, sal de su borde y rompe el
apoyo. Después construye hacia una plataforma transitable del pueblo y pasa a
ella. Comprueba también el comportamiento de los ganados desde abajo.

F8, caducidad de 500 ms, cambio de generación/actor y escenas nativas detienen
las escrituras verticales. F8 devuelve el control a RE4 desde la posición actual;
no teletransporta a León de vuelta al suelo anterior.

Las compilaciones y pruebas automatizadas pasan en Linux con APIs simuladas;
esta entrega todavía requiere confirmación en RE4/Windows. Elevar a León corrige
la diferencia entre su altura real y la vista, pero no demuestra que todos los
ataques nativos respeten esa altura. Las superficies decorativas sin colisión
nativa y los accesos controlados por eventos no se convierten en plataformas.
