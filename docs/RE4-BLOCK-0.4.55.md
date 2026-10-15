# 0.4.55: trampas, altura y rendimiento

Esta versión incluye DLL455, mod y host nuevos. Los cambios se han compilado y
probado con APIs simuladas en Linux; falta comprobar el resultado en Windows.
No se presenta como la solución demostrada de todas las pérdidas de foco.

## Actualizar desde0.4.53/54

1. Cierra RE4, Minecraft, host y VIEW.
2. Ejecuta `UNINSTALL-RE4-CONTROLS.bat` del paquete nuevo para retirar la DLL
   anterior reconocida. Después ejecuta `INSTALL-RE4-CONTROLS.bat`, que verifica
   la copia de partidas y configura la DLL455. No sobrescribe DLLs ajenas.
3. Sustituye el mod anterior por `mods/re4bridge-0.4.55.jar`; conserva una sola
   versión del mod y Fabric API. No instales el JAR de fuentes.
4. Abre RE4 y Minecraft en modo ventana. Usa `START-RE4-CONTROLS.bat` y
   `VIEW-RE4.bat` de esta carpeta; Preparar Minecraft y F9. F8 devuelve el control.

## Golpes a trampas y bombas

Las trampas de osos y bombas del perfil comprobado (cEm2a) reciben el mismo
mensaje de impacto nativo validado por alcance, pared y parte del modelo.
Sus manejadores, identificados en el diagnóstico de solo lectura, ejecutan el
cierre/activación y la explosión nativa, conservando efectos y sonido del juego.
No se fuerzan HP, rutinas, explosiones ni sonidos desde el puente. La confirmación
exige identidad/variante intactas, mensaje consumido, HP0 y la transición nativa
correspondiente; HP0 por sí solo no confirma. No cuentan como bajas vivas ni
producen corazones. El golpe conserva desgaste/efectos Minecraft tras confirmarse.
Las funciones de trampas se verifican por perfil; si no coinciden se excluyen
estas dianas sin deshabilitar el combate de ganados. Aún no hay arco/ballesta.

## Paradas y fluidez

El log anterior incluye una parada con Minecraft enfocado y muestras recientes;
la DLL antigua reutilizaba stop1 para varias causas. Se encontró un fallo en la
comprobación de desplazamiento: sumaba Y aunque el solver solo corrige X/Z.
Una caída, un cambio de suelo o la devolución de altura a RE4 podía dispararla.
Ahora comprueba únicamente X/Z, manteniendo la detección de desplazamientos
horizontales fuera de rango y la caducidad de la referencia de colisión.

Minecraft limita su renderizado a60FPS durante el control activo en primera
persona, respetando un límite inferior existente. Al detenerse aplica de nuevo
el límite habitual sin cambiar las opciones guardadas. La interfaz/mano ya se
capturaba aproximadamente a60Hz; dibujar cientos o miles de cuadros completos
competía por CPU/GPU con RE4 sin transferir más imágenes. No se afirma una
mejora medida en tu equipo ni se elimina la generación de chunks.

## Si vuelve a interrumpirse

El primer `Cambio de control` en `logs/re4bridge-host.log` y la consola incluye
`stop_cause`, estado gráfico y de colisión, además del motivo/edades del cliente.

| stop | Motivo |
|---|---|
| 1 | F8 nativo |
| 2 | Lease nativo de entrada caducado |
| 3 | Referencia de colisión sin actualizar durante500ms |
| 4 | Desplazamiento horizontal fuera de rango |
| 5 | Cambio de personaje durante altura compartida |
| 6 | Altura fuera del rango permitido |
| 7 | Reinicio/fallo de dispositivo o interfaz gráfica |
| 8 | Retorno/parada de VIEW |

VIEW conserva una causa nativa ya registrada. Una pérdida real de foco del
cliente sigue indicada como `Minecraft perdio el foco`; no se fuerza el foco
ni se rearma automáticamente después de una parada persistente. Para errores
locales del mod sigue disponible `logs/latest.log` de Minecraft.
