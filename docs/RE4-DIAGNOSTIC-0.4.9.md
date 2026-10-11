# 0.4.9: medir la cámara que se vuelve a desviar

El usuario no observó mejora con 0.4.8. Revisados `gpt.mp4` y
`camera-trace-20261011-020114.jsonl` (216 muestras, DLL en disco 0.4.8).
El movimiento inicial del ratón no invalida las partes posteriores:
entre 35.3 y 36.4 segundos del registro, sin teclas ni giro acumulado,
la orientación solicitada permanece en -1.911 rad y Global se desvía
hasta unos 0.209 rad (12°). Al empezar a caminar vuelve a alinearse.
Los tiempos del registro y del vídeo no tienen un reloj común registrado.

El registro también muestra retraso de Global al girar el ratón. No permite
establecer si el hook final se ejecuta durante la imagen visible o si RE4
sobrescribe su resultado. Muestrear memoria cada 200 ms no es capturar
exactamente el fotograma renderizado.

Esta versión **añade diagnóstico, no promete corregir la cámara**. Mantiene
el comportamiento de movimiento/cámara de 0.4.8, el protocolo de control
de 80 bytes, los mods/host 0.4.3 y el retorno normal mediante F8.
No modifica posiciones, colisiones, partidas o inventario.

La DLL publica una memoria separada `Local\RE4Bridge-CameraDiagnostics-PID`:
identifica la compilación realmente cargada y cuenta entradas al hook de
proyección, escrituras y llamadas rechazadas. Registra, dentro de esa llamada,
el yaw antes de escribir, después de escribir y después de la función original.
TRACE abre esta memoria únicamente con acceso de lectura. Sus muestras usan
secuencia de publicación; si coinciden con una llamada en curso se marcan
incompletas. No añade activación automática ni renueva controles.

## Instalar y probar

1. Cierra RE4 y el host. Usa el **UNINSTALL de tu paquete 0.4.8**.
2. Extrae 0.4.9 en otra carpeta y usa su **INSTALL-RE4-CONTROLS**.
   Conserva los mods actuales de Minecraft. Reinicia RE4 para cargar la DLL.
3. Inicia START-RE4-CONTROLS y TRACE-RE4-CAMERA de esta carpeta con los
   permisos de RE4. Vuelve a Minecraft; activa controller, firstperson y arm.
4. Durante la captura: quieto sin ratón unos 5 s; W unos 3 s y suelta otros
   5 s; luego alterna A/D sin ratón. Por último gira suavemente el ratón.
   Puedes usar VIEW/F9 para ver RE4. F8 devuelve el control normal.
5. Comparte el JSONL nuevo. Un vídeo es útil, pero el dato nuevo imprescindible
   está en `projection_diagnostic`.

Si calls se queda en cero mientras aumentan camera_ticks, la entrada enganchada
no cubre ese recorrido. Si yaw_written coincide con view_yaw pero yaw_after
cambia, la función original lo sustituye. Si ambos coinciden pero la vista
observada diverge, habrá que buscar una escritura posterior u otra fase de
renderizado. Una muestra de Global aislada no decide entre esas causas.

## Validación

DLL PE32 compilada en Linux; tres suites C y pruebas Python del protocolo,
lector y decodificador. Los instaladores conservan las comprobaciones de hash
y propiedad de la DLL. No se ejecutó el juego Windows en este entorno; la
captura real es necesaria para determinar el siguiente cambio de cámara.
