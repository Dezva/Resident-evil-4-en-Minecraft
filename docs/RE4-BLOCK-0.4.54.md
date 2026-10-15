# 0.4.54: estabilidad del armado, consola y ventanas

El combate conserva la DLL de 0.4.53, idéntica byte por byte (build453). El usuario
confirma muertes de ganados/cuervos y cajas rotas; esta actualización se centra
en los cortes de control. Los logs anteriores no contienen el motivo local del
mod: «cliente desarmado» también aparece al cambiar de ventana para copiarlos.
Uno de los fragmentos incluye WinError10053 y reconexión; otros cortes ocurren
con el socket respondiendo y stop0. No se atribuyen todos a una causa única.

## Pausas gráficas y foco

Antes, un tick de Minecraft que tardase200ms hacía que el hilo de red enviase
«desarmado». Ahora el movimiento sigue cortándose a los200ms, pero se conserva
la sesión con botones0, acciones/combate suspendidos y cámara/colisionadores
congelados. Un tick nuevo antes de cumplirse1s puede continuar la misma sesión.
Al llegar a1s sin ticks se desarma definitivamente y hace falta otro armado.
Los fotogramas de imagen o la red no renuevan ese plazo. F8/paradas nativas y
pérdidas persistentes de foco mantienen su parada y no se rearman solos.

Una pérdida de foco de hasta200ms suspende entradas inmediatamente, conservando
la sesión existente. La devolución del foco después de ese plazo requiere otro
armado, incluso si Minecraft no ejecutó ticks durante la pérdida. VIEW conserva
brevemente la vista solo para el HWND transitorio ForegroundStaging de Explorer
o un foreground nulo; cambiar a una aplicación real sigue deteniendo VIEW.
No se fuerza el foco de vuelta automáticamente.

La consola escribe desde otro hilo con una cola limitada. Seleccionar texto no
bloquea el socket de control. Ctrl+C sigue cerrando el host si Windows lo entrega
como interrupción; cambiar realmente a la consola también puede perder el foco.

La captura de interfaz conserva su cadencia, pero comprueba el intervalo antes
de consultar/restaurar el estado OpenGL. Se evita ese trabajo en los fotogramas
que no se van a capturar. No se afirma una ganancia de FPS medida en Windows.

## Preparar Minecraft y F9

VIEW restaura primero el estado minimizado/maximizado sin activar la ventana;
quitar WS_MINIMIZE del estilo no bastaba y podía dejar RE4 junto a la barra.
Antes de redimensionar, comprueba el modo gráfico si la captura nativa lo conoce.
Si la captura indica que RE4 arrancó en pantalla completa exclusiva, pide cambiar
a **modo ventana** en su configuración gráfica y reiniciar RE4. F9 presenta esa ventana sin bordes; no cambia el
modo de Direct3D. Usa también Minecraft en modo ventana antes de preparar.

## Actualizar y recoger el motivo real

Cierra host y VIEW, sustituye el mod anterior por `re4bridge-0.4.54.jar` y abre
el host/VIEW de esta carpeta nueva. Conserva Fabric API. La DLL453 instalada con
0.4.53 sirve: no hace falta reinstalarla. El paquete incluye esa misma DLL y los
scripts para una instalación nueva. No mezcles el host o VIEW anteriores.

Se guardan automáticamente cambios de armado, con su hora:

- `logs/re4bridge-host.log`: `Cambio de control`, generación, motivo del cliente,
  `focused`, `paused`, `screen`, `tick_age_ms` y `sample_age_ms`, escena y stop nativo.
- `view-events.log`: preparar/F9/F8, foco transitorio/recuperado y fin de vista.
- `logs/latest.log` de Minecraft: cambios del control y trazas de seguimiento,
  orientación o captura cuando producen una parada.

Si se repite, conserva las líneas de `Cambio de control` alrededor del primer
corte. «sin tick de Minecraft; entrada suspendida» distingue una pausa gráfica;
«lease caducado», foco, pausa, muestra ausente y fallo de seguimiento quedan
separados. Copiar después ya no elimina el primer motivo registrado.

Las pruebas automatizadas pasan; la presentación y el foco reales en Windows
necesitan comprobarse en tu equipo. Arcos/ballestas quedan para después de
estabilizar este paso.
