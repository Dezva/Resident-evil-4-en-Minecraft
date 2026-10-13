# 0.4.38: apoyo de bloques y paradas de primera persona

El registro de37 muestra una parada nativa junto a un nuevo rechazo de
colisiones, sin fallos de dibujo. Las fotos muestran la fila de piedra separada
del suelo también en Minecraft. El salto automático a la celda superior no
resolvía el contacto con el terreno y provocaba bloques flotantes.

## Instalar y probar

1. Cierra Minecraft, RE4, host y VIEW. Desinstala la DLL anterior e instala la
   nueva con los BAT del paquete. Sustituye el mod anterior por
   **re4bridge-0.4.38.jar**; conserva Fabric API. Usa host, VIEW, DLL y mod nuevos.
2. Abre una partida de RE4 y un mundo local creativo de Minecraft, ambos en
   ventana y sin minimizar. Abre START-RE4-CONTROLS y VIEW.
3. **Preparar Minecraft → esperar confirmación → F9**. No hacen falta comandos.
4. Prueba una construcción nueva: tres piedras sobre suelo cercano, otra piedra
   sobre la primera y una losa al lado. Comprueba que la fila queda apoyada en
   Minecraft y que en RE4 no aparece elevada un bloque entero. Rodea la fila,
   coloca y rompe mientras caminas/giras; después prueba sprint.
5. F8 devuelve RE4 normal. Preparar → F9 vuelve al puente. También puedes probar
   sin F9, con ambos juegos visibles en ventana y Minecraft con el foco.

No se trasladan construcciones existentes para ocultar el fallo. Al reiniciar
Minecraft se inicia otra sesión del puente; la prueba debe usar bloques nuevos.

## Cambios

- Se transmite la clasificación nativa SAT/EAT de cada triángulo: suelo,
  pendiente, pared u otro. La clasificación ausente sigue siendo estricta.
- El suelo y las pendientes pueden entrar en la base de una celda de construcción
  mientras no corten su cara superior dentro de la huella. El terreno de RE4
  oculta la parte enterrada mediante su profundidad. Paredes y superficies sin
  clasificación siguen bloqueando una colocación que las atraviese.
- Una celda rechazada ya no se cambia por otra situada un bloque más arriba.
  La selección, el paquete de colocación y el volumen mantienen la misma celda.
- Antes de construir, la referencia vertical puede alinearse una sola vez con
  el suelo nativo próximo bajo Leon. Nunca se vuelve a fijar una construcción
  existente. Ojos y pies conservan el mismo transformado de cámara y bloques.
- El cristal auxiliar consulta el suelo por columna y usa su misma altura
  cuantizada. No se limita a truncar la altura de Steve, que podía dejar el
  apoyo una celda por debajo. Solo se consultan121 columnas locales, con caché
  por malla/posición; las construcciones del usuario no se sobrescriben.
- La detección de saltos de posición permite el avance proporcional al tiempo
  entre actualizaciones de colisión. El límite fijo500unidades podía detener
  sprint normal acumulado durante un intervalo corto sin dibujo. Se mantienen
  el barrido desde la última posición segura, el límite temporal500ms, F8 y
  el rechazo de teletransportes; no se rearma automáticamente.
- Si la colisión provoca una parada, su motivo4 permanece en el registro en
  vez de perderse en el siguiente callback inactivo. Significa salto de posición
  o intervalo prolongado sin referencia válida.
- El hook de ratón de VIEW tiene su propio hilo con cola de mensajes; ya no
  depende del bombeo de la interfaz PowerShell. Esto evita que una pausa de la
  interfaz deje sin captura los clics. Las pérdidas reales de foco siguen
  deteniendo la vista; el registro identifica proceso y clase de la ventana.

Los logs de VIEW enviados identifican pérdida de foco, pero pueden corresponder
al Alt+Tab posterior a la parada nativa descrita por el usuario. No demuestran
por sí solos cuál fue la primera causa. Esta versión corrige el límite fijo y
mejora el diagnóstico; estabilidad real de F9 pendiente de prueba en Windows.

## Límites y diagnóstico

El suelo auxiliar sigue siendo una cuadrícula de bloques completos, no una
reproducción continua del terreno ni una malla de losas. En pendientes, la
parte inferior del bloque puede quedar enterrada en RE4. No se promete ajuste
exacto de cada borde. Flechas visibles/daño nativo y salto siguen pendientes.

Si se pierde primera persona, antes de minimizar observa si el contorno y HUD
siguen dibujándose. Después guarda el tramo del host alrededor de la parada y
`view-events.log`. El campo `collision_state.reason` conserva4 si la causa fue
la protección de movimiento; `view-events.log` identifica una pérdida de foco.
CAPTURE-RE4-GRAPHICS sigue leyendo IPC sin abrir memoria ni activar controles.

## Verificación

Core36 y Minecraft49 pruebas; build Fabric/JDK21 y DLL PE32. Python138;
15 fixtures nativos con ASan/UBSan. Casos: suelo fraccionario, pendiente
recortada a la huella, pared/techo, celda original y paquete aceptable, apoyo
auxiliar que conserva construcciones, sprint4.15unidades/ms con intervalo de
150ms sin dibujo y posterior barrido, teletransporte rechazado y motivo de
parada persistente. Backend VIEW compilado y routing/restauración verificados
en escritorio simulado. El hilo Win32, la GPU y RE4 real no se ejecutan en cloud.
