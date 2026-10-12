# 0.4.29 — primera persona activa y medición del movimiento

El usuario confirma que 0.4.28 recupera el rendimiento de RE4 solo y con
Minecraft abierto. La caída aparece después de arm/primera persona. También
percibe caminar/correr más despacio que en Minecraft y que en RE4 normal;
sprint sí aumenta algo la velocidad. Esta versión reduce el coste de la cámara
activa y añade una comparación medida antes de ajustar la locomoción.

## Cámara activa

Las proyecciones repetidas reutilizan la vista calculada dentro de una misma
actualización nativa, durante **un máximo de16ms**. Cada llamada continúa
ejecutando la función original y reaplica matrices/ocultación si el juego las
cambió. Se comprueban las referencias y propietarios que se validaron para esa
actualización. Cambios de input/secuencia, pad/cámara, posición, objeto/vtable,
arma/controlador o FOV descartan la reutilización y ejecutan las comprobaciones
completas. Caducidad, F8 y desarmado siguen siendo efectivos; F8 se consulta
también en la ruta reutilizada.

La posición se comprueba de nuevo después de consultar F8, antes de escribir la
vista. Un movimiento tardío obliga a calcularla desde los pies corregidos. Se
mantiene la comprobación de colisión final; no se omite cuando el motor movió al
jugador. No se modifican Y, velocidad, animaciones, inventario ni física nativa.
La reutilización es acotada a la actualización y16ms, no una autorización para
conservar punteros entre frames/cambios de zona. El diagnóstico de cámara cuenta
las reconstrucciones completas, no cada reaplicación idéntica.

La comparación del código real bajo APIs simuladas cubre10000 llamadas iguales
de una actualización: desaparecen las consultas de memoria/control/actor y
cálculos repetidos. No es una medición de GPU/FPS en Windows. La nueva prueba
integra cámara y collider reales:2000 avances tardíos contra una pared quedan
fuera de ella y la vista usa los pies corregidos. Se prueban sobrescrituras de
matrices/bits de modelo por el motor, FOV nuevo, propietarios nuevos, expiración,
generación, F8 y el límite16ms. Se conserva la ruta inactiva de0.4.28.

## Actualizar

1. Cierra juegos, host, VIEW y pruebas. Conserva el paquete anterior.
2. **UNINSTALL-RE4-CONTROLS del paquete anterior**, seguido de extracción del
   paquete nuevo y **INSTALL-RE4-CONTROLS de0.4.29**.
3. Conserva **re4bridge-0.4.26.jar** y Fabric API. El ZIP incluye esos mismos bytes;
   deja una sola copia del puente. Usa el **START de0.4.29**.
4. Comprueba primero el rendimiento con primera persona y una cantidad parecida
   de bloques. No necesitas preparar un mapa nuevo.

DLL SHA256: `c065b2d143d8d92fc2b71f1d12b94f20086160ad568e3de91f5c9a8edf1dca5a`.
Build429. ABI de control v2/80, escena v6/96352 y HUD v1/32792 conservados.
Instaladores y VIEW sin cambios. Para recuperar0.4.28: cierra todo, desinstala
con0.4.29 e instala con0.4.28; usa su host. El JAR se conserva.

## Comparar velocidad con señales de inicio/fin

**MEASURE-RE4-MOVEMENT.bat** registra dos tramos de4s: caminar y correr.
Lee posición, teclas efectivas del motor, cadencia de PadRead y contactos con
bloques; solo usa lectura de proceso/IPC. No envía movimientos ni toma el canal
del host. La velocidad sigue siendo la nativa de RE4: esta entrega prepara su
calibración, sin imponer un multiplicador a partir de una impresión visual.

Haz las dos comparaciones en la misma zona plana y despejada, sin tocar bloques
ni paredes y sin mover el ratón durante los tramos:

1. **RE4 normal:** F8 para desarmar y volver a los controles normales. Abre
   MEASURE y enfoca RE4. Pulsa **F10** cuando estés preparado.
2. Escucharás tres tonos de cuenta atrás. **Al tono largo de inicio**, mantén
   W sin correr hasta el tono de fin. Suelta W.
3. Pulsa **F10 otra vez** para el segundo tramo. Al tono largo, mantén W y la
   tecla de correr de **RE4** durante4s, hasta el tono de fin.
4. Guarda el `movement-measure-....json` generado junto al paquete.
5. **Puente:** abre MEASURE nuevamente, prepara Minecraft, primera persona y
   arm, cierra el chat y mantén la ventana de Minecraft enfocada. Pulsa F10 y
   repite los tramos; para correr utiliza la tecla de sprint de **Minecraft**
   (normalmente Ctrl). VIEW/F9 es opcional. Guarda el segundo informe.

F10 inicia la captura sin cambiar el foco; no es necesario volver a la consola.
El programa espera hasta3min por cada F10 y muestra la cuenta atrás también en
consola. Si RE4 corre elevado, ejecuta el medidor con ese mismo nivel para que
Windows permita leerlo. El resultado indica unidades nativas/s y bloques/s
con la escala1000; si usas otra escala, pasa `--units-per-block` al BAT.

El informe marca capturas inválidas si faltó W/correr, cambió la zona o el estado
de armado, hubo saltos/contactos de bloques o demasiado tiempo sin avanzar.
La cadencia de **PadRead no es FPS de GPU**. Distancia por segundo y cadencia
permiten comparar los casos; la referencia de velocidad Minecraft en suelo
tampoco equivale al vuelo creativo. Se necesitan los dos informes para decidir
el ajuste de velocidad y distinguirlo de una caída de actualización del juego.

## Validación

Pasan109 pruebas Python y11 fixtures C bajo ASan/UBSan, cuatro suites C portátiles
y fixtures PowerShell. La medición usa el lector real con memoria/proceso
simulados, incluyendo ejecución completa de sus dos fases, reloj monotónico,
distancia/cadencia y rechazo de capturas inadecuadas. No se han ejecutado juegos,
beeps ni drivers Windows reales en la nube; rendimiento y velocidad reales
quedan pendientes de esta prueba. El JAR/API/instaladores/VIEW se verifican
idénticos a los anteriores al empaquetar.
