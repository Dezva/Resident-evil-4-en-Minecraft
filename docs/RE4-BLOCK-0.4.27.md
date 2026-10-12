# 0.4.27 — revisión del rendimiento de RE4

Esta entrega responde a la caída de rendimiento observada al instalar 0.4.26.
Conserva el editor sincronizado, las texturas, la cámara y el HUD inicial; no
añade nuevas funciones de mapa. **El mod Minecraft sigue siendo 0.4.26, con los
mismos bytes. Cambian la DLL y los lectores/host que reconocen build427.**

## Trabajo repetido encontrado y reducido

- El HUD nuevo creaba y destruía una textura D3D9 en cada dibujo, aunque la barra
  no cambiase. Ahora conserva una textura por dispositivo y solo carga píxeles
  cuando cambia la imagen. Un nuevo paquete de estado con la misma imagen no
  provoca otra carga.
- El atlas de los bloques también se creaba y cargaba en cada dibujo. Ahora se
  conserva y solo se actualiza si cambian las teselas o su cantidad; cambiar la
  cámara, colocar una caja con materiales ya presentes o renovar la época no
  obliga a cargar otra imagen. Cambiar entre atlas y piedra básica recrea la
  textura del tamaño correspondiente.
- La comprobación adicional de colisiones en proyección copiaba una escena de
  96352 bytes y validaba las coordenadas UV, además de repetir esa validación en
  el barrido. Ahora lee un prefijo coherente de **3168 bytes**, que contiene
  permiso, referencia y cajas, y valida la geometría una sola vez. La validación
  de materiales sigue en el renderizador. Si el motor no ha movido a Leon, no
  hace otro barrido. Se mantienen los callbacks finales y la referencia segura
  que evita atravesar una pared al mantener W.
- Se evitan inicializaciones completas redundantes de las imágenes y la escena.
  El HUD comprueba primero si los controles/primera persona están activos.

Las cachés son acotadas por dispositivo. Se liberan al desactivar su dibujo,
perder/reiniciar el dispositivo, escapar de la interfaz o liberar la última
referencia del puente. Las texturas DEFAULT se liberan **antes** de llamar a
Reset. Las capturas de estado siguen siendo temporales y se restauran en cada
dibujo: no conservamos referencias a shaders/texturas del juego entre frames.

Esto reduce operaciones comprobables, pero **no demuestra aún una recuperación
de FPS en el RE4 del usuario**. La nube no ejecuta bio4.exe ni su driver D3D9.
La generación de superficies y la captura/restauración gráfica siguen teniendo
un coste; no se afirma que todo el coste del puente haya desaparecido.

## Actualizar desde 0.4.26

1. Cierra RE4, Minecraft, host y VIEW. Conserva el ZIP anterior.
2. Ejecuta **UNINSTALL-RE4-CONTROLS del paquete 0.4.26**.
3. Extrae el paquete 0.4.27 completo y ejecuta **INSTALL-RE4-CONTROLS**. Puedes
   reutilizar la carpeta después de desinstalar y retirar sus archivos viejos.
4. Conserva **re4bridge-0.4.26.jar** y Fabric API en Minecraft. El ZIP incluye
   esos mismos JAR; no instales dos copias del puente.
5. Abre los juegos y **START-RE4-CONTROLS de 0.4.27**. Los comandos son los mismos.

DLL SHA256: `44e9f3fbb0531acc6a5460beaaa8960951b5dca9f2bf770b97e25293da39a141`.
Build427; control v2/80, escena v6/96352 y HUD v1/32792 conservados.
Instaladores y VIEW sin cambios.

La primera comprobación pendiente es el rendimiento en la misma zona de RE4,
con una cantidad parecida de bloques y primera persona. No hace falta preparar
otro mapa para ella. Si sigue lento, comparar con F8 y después con `hud off`
seguido de `arm` permite distinguir el coste de control/dibujo del de la barra.
No se requiere ejecutar un registro para esta primera comparación.

## Qué hace map

`/re4bridge map on` **detiene el puente** y permite construir manualmente en
Minecraft un suelo o unas paredes auxiliares, por ejemplo de cristal. Esas
nuevas colocaciones no se envían a RE4. `map off` termina ese modo; al volver a
usar `arm`, las nuevas construcciones vuelven a enviarse. La referencia inicial
se prepara antes con `blocks on` y `arm`, como describe la
[guía del editor 0.4.26](RE4-BLOCK-0.4.26.md).

Mientras está armado, Steve sigue la posición que realmente alcanza Leon. Si
una pared nativa detiene a Leon, Steve deja de avanzar con él: es seguimiento
de posición, **no una copia de la geometría de colisión a Minecraft**. No hay
todavía superficies Minecraft sobre las que apuntar o colocar bloques en toda
la zona de RE4.

Importar automáticamente las colisiones es una vía posible para un mapa
auxiliar. Requiere localizar y validar la geometría de colisión o consultas de
suelo/pared del motor, convertir sus superficies a la cuadrícula y gestionar
los cambios de zona. Las coordenadas del actor por sí solas no revelan esa
geometría. Esta entrega no añade escaneos de memoria ni instala nuevos hooks
para conseguirla; primero debe comprobarse el rendimiento.

## Validación y recuperación

Pruebas con el renderizador real y una API D3D simulada: 200 frames de HUD y
200 de atlas estable usan una creación y una carga de textura cada uno.
Píxeles/cantidad/material básico nuevos actualizan o recrean los recursos;
se comprueban errores de actualización, liberación antes de Reset y cierre,
estado gráfico restaurado, F8 y caducidad. El lector de colisión deja intactos
los bytes de UV e imágenes y rechaza publicaciones incompletas.

Pasan 104 pruebas Python, incluyendo 11 fixtures C; esos fixtures también pasan
ASan/UBSan. Pasan cuatro suites C portátiles y los fixtures PowerShell del
instalador/diagnóstico. Se conserva la prueba de 1999 actualizaciones con presión
contra una pared y fallos temporales de lectura/gráficos. El JAR se verifica
idéntico al publicado en 0.4.26; no se han cambiado ni vuelto a compilar sus fuentes.

Para volver a 0.4.26: cierra todo, desinstala con 0.4.27 e instala con 0.4.26;
utiliza el host anterior. El JAR se conserva. F8 sigue devolviendo el control
normal durante la prueba.
