# 0.4.43: corrección de Preparar y control bloqueado

La 0.4.42 confundía el indicador de formato cinematográfico de RE4 con una
cinemática activa. En tu registro, ese indicador estaba presente mientras
jugabas normalmente: el puente seguía armado, pero anulaba movimiento, mirada
y construcción. Preparar esperaba una primera persona que nunca se activaba
y terminaba cancelándose.

Esta versión deja de usar ese indicador como prueba de una cinemática. Conserva
la detección de películas, eventos, muerte y transiciones nativas. El estado de
escena se actualiza también estando desarmado, para que una preparación nueva
no reciba un estado antiguo de cinemática.

## Actualizar

1. Cierra RE4, Minecraft, host y VIEW.
2. Sustituye los archivos del paquete y el JAR por **re4bridge-0.4.43.jar**.
   Debe quedar un solo JAR de RE4 Bridge; conserva Fabric API.
3. Usa **UNINSTALL-RE4-CONTROLS → INSTALL-RE4-CONTROLS** del paquete nuevo.
   Incluye una **DLL nueva, build443**. El desinstalador reconoce la DLL publicada
   de 0.4.42, además de 0.4.41 y 0.4.39/40, sin retirar DLL ajenas.
4. Abre ambas partidas, START-RE4-CONTROLS y VIEW. En la partida normal de RE4,
   pulsa Preparar Minecraft y espera la confirmación; después F9.

Actualizar solo el JAR no resuelve este fallo: estaba en la DLL. El host nuevo
rechaza la DLL442 con un mensaje de actualización, para evitar mezclar archivos.

Los controles se conservan: E inventario, Esc menú de Minecraft, F5 creativo/
supervivencia, R acción contextual de RE4 y F8 retorno a RE4 normal. También
funciona con ambos juegos en ventana sin VIEW.

## Comprobación

Primero comprueba Preparar → confirmación → F9, W/sprint, girar, colocar y romper
un bloque. En una partida normal, `runtime_state.scene` debe ser **0** y la cámara
de primera persona debe activarse. Después prueba una cinemática y su retorno.
No cambies referencias ni borres tus construcciones para comprobar esta corrección.

El registro enviado sigue mostrando una consola activa y después una parada
nativa; no demuestra un cierre del proceso host. Además registra un dispositivo
gráfico perdido anteriormente. Esta corrección no desactiva las paradas por
pérdida de dispositivo, F8, conexión o foco: si hay otro corte, conserva las
líneas alrededor de `stop` y `reason` y `view-events.log`.

Se conservan las características y límites de
[0.4.42](RE4-BLOCK-0.4.42.md): vida/muerte compartidas, pose de pie, escenas nativas
y primeras colisiones horizontales de humanoides. No se añade navegación de IA,
bloqueo de ataques a través de paredes ni nuevas armas.

## Verificación

Regresión con el valor real `50331920` (`0x03000110`) del registro: clasificación
normal, callbacks pad/camera/projection activan primera persona y mantienen
W/sprint. Agregar MOVIE sigue cediendo cámara a RE4; quitarlo retoma la misma
sesión. Repetidas consultas usan cache; las escenas se actualizan aun después de
F8 y antes de otro armado. Política de host/IPC conserva controles durante cien
mensajes y una apertura/cierre de menú; DLL442 se rechaza con error claro.

Build Fabric/JDK21 y DLL PE32; pruebas automáticas y fixtures sintéticos. No se
ejecutaron RE4 ni el driver D3D9 real en cloud; la preparación y presentación
final deben comprobarse en tu Windows.
