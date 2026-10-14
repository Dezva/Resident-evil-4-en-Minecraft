# 0.4.48: apoyo del salto separado de la cámara

El usuario confirmó R, colisiones de ganados y partículas más fluidas en0.4.47.
Encontró una expulsión al moverse sobre bloques tras saltar y colocar debajo,
también al acercarse ganados. El log aportado conserva el puente armado y sin
stop mientras aumentan las correcciones laterales y cae la elevación virtual.

## Corrección

La física horizontal utilizaba la altura suavizada de la cámara. Al aterrizar
sobre un bloque, esa altura podía seguir por debajo de su cara superior durante
la interpolación. La separación interpretaba el bloque de apoyo como una pared
y desplazaba a Leon hacia fuera.

La colisión ahora utiliza el último extremo físico validado del salto. La
cámara conserva su interpolación visual independiente. Un margen de apoyo de
0.01 bloques absorbe pequeñas variaciones del suelo nativo entre paquetes;
se aplica tanto al barrido como a la separación, sin mover la Y del actor.
El margen no elimina paredes elevadas ni fija al jugador a un borde. Menú,
F8, leases, generación y datos inválidos conservan las guardas existentes.

La regresión reproduce cámara a900unidades con pies físicos a1000 sobre un
bloque de1000: antes provoca una corrección lateral; después conserva el apoyo.
Incluye movimiento y desplazamientos nativos pequeños, una pared por encima
del apoyo y salir del borde para caer. También se valida que la altura física
se conserve al pausar y se borre con solicitudes inválidas.

## Actualizar

Cierra ambos juegos, el host y VIEW. Extrae el paquete completo nuevo y deja
solamente `re4bridge-0.4.48.jar` más Fabric API en los mods de tu instancia.
Ejecuta `UNINSTALL-RE4-CONTROLS.bat` y después `INSTALL-RE4-CONTROLS.bat` del
paquete nuevo. Se reconoce la DLL447 por checksum y se respaldan las partidas.
Arranca ambos juegos, el host nuevo y VIEW; prepara Minecraft y pulsa F9.
DLL, host y JAR deben actualizarse juntos.

Prueba de nuevo saltar y colocar debajo, aterrizar, moverte sobre la columna y
repetir con ganados cerca. La regresión automatizada pasa, pero falta confirmar
esta compilación en RE4/Windows real. Esto no soluciona todavía los ataques
desde abajo debidos a que la elevación del actor sigue siendo virtual.
