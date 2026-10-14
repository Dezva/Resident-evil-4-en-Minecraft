# 0.4.50: escaleras nativas sin altura bloqueada

El usuario confirmó que la colocación y la altura real de León funcionan en
0.4.49 y que los ganados parecen dejar de alcanzarlo desde abajo. Su nuevo
video muestra retrocesos al subir una escalera nativa.

El log de ese tramo contiene cero bloques transferidos y cero correcciones de
colisión. El mapa permanece «incompleto o excede límite local» y el actor conserva
Y=315.845642 incluso mientras RE4 intenta caminar por la escalera. La simulación
esperaba un mapa válido, pero seguía imponiendo la altura absoluta anterior.

## Cambio

Al caminar sobre suelo nativo, RE4 controla la altura y Minecraft sigue la
posición observada. Esto permite subir y bajar peldaños aunque falte el mapa
local, y evita predecir una altura distinta por redondeo de sus superficies.
No hace falta sustituir esa escalera por medias losas ni representar cada
peldaño con un bloque de Minecraft.

El puente conserva la altura absoluta durante el salto y al apoyarse en bloques
colocados. Perder el mapa en una columna no devuelve el control de la altura al
suelo inferior. Al aterrizar en una plataforma nativa se devuelve la altura a
RE4; el siguiente salto empieza en el nuevo suelo. Se conserva la espera del
salto ante techos o mapas desconocidos.

Runtime v4 conserva sus 128 bytes; build450 añade el modo de apoyo nativo sin
escritura de Y y mantiene las dimensiones de las colisiones X/Z según la escala.
En ese modo `vertical_active=False` es normal: la DLL está dejando la altura a
RE4. `actor_y` conserva la última altura aplicada por el puente; la posición
actual se ve en `position_raw`/`position` del lector.

## Actualizar y comprobar

Cierra ambos juegos, el host y VIEW. Extrae el paquete completo. Deja únicamente
`re4bridge-0.4.50.jar` y Fabric API en los mods de la instancia. Ejecuta primero
`UNINSTALL-RE4-CONTROLS.bat` y después `INSTALL-RE4-CONTROLS.bat` del paquete nuevo.
El desinstalador reconoce la DLL449 publicada por checksum. Actualiza DLL, host
y JAR juntos, inicia el host nuevo y prepara Minecraft en VIEW como antes.

Prueba subir y bajar la escalera del video, saltar desde su parte superior,
construir una columna y moverte sobre ella. Las regresiones reproducen el fallo
de 0.4.49 y pasan con el cambio; queda confirmar esta compilación en RE4/Windows.
