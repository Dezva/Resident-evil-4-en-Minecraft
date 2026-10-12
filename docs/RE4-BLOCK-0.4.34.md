# 0.4.34 — fluidez de mano/inventario y eventos de VIEW

La prueba del usuario confirma que0.4.33 oculta correctamente el HUD de RE4 y
renderiza la mano y el inventario. Esta actualización aborda la capa visual
limitada a20Hz y las interrupciones al combinar inputs. F8 sigue devolviendo
RE4 normal; la ocultación de vida/balas continúa siendo automática en FP.

## Actualizar

1. Cierra RE4, Minecraft, host y VIEW. Ejecuta UNINSTALL del paquete anterior
   y después INSTALL del nuevo (DLL build434). Extrae el paquete nuevo y sustituye el mod
   anterior por **re4bridge-0.4.34.jar**: debe quedar una sola versión del puente.
2. Usa START-RE4-CONTROLS y VIEW-RE4 de este paquete. La DLL434 incorpora la
   reutilización de la imagen válida durante escrituras del host; mantiene la
   lógica de ocultación HUD confirmada en33. Su SHA256 es
   `c9581bc78dd4364187d0e33b2ca7adca4eeb5e7cd83d777b2bc064eda8702c91`.
3. Abre ambos juegos en ventana, sin minimizar. Pulsa **Preparar Minecraft**
   en VIEW antes de arm. Conecta `/re4bridge controller`; activa
   `/re4bridge firstperson` si está desactivada (es un toggle, sin argumento
   `on`). Para construir usa `/re4bridge blocks on`. Después
   `/re4bridge arm`, cierra el chat y pulsa F9.
4. Prueba caminar, mover rápido el ratón, cambiar de ranura, colocar/romper y
   alternar inputs. La mano y la barra ahora pueden actualizarse hasta60Hz,
   independientemente de los20ticks/s del mundo. E abre el inventario real:
   prueba cursor, Shift-clic y arrastrar; E/Esc lo cierran. F8 devuelve RE4 normal.

La frecuencia real depende de la GPU, la codificación y la conexión local.
En la nube se han probado lógica, transporte y compilación; faltan mediciones
de fluidez y ejecución OpenGL/GLFW/Win32 reales en el equipo del usuario.

## Si vuelve a interrumpirse

VIEW guarda **view-events.log** junto al BAT, con hora local y la causa concreta:
ventana cerrada/cambiada, pérdida real de foco, error del hook o fallo PostMessage
con código Win32. El log se escribe después de detener/restaurar, de modo que
un error al guardarlo no impida F8 ni el corte por pérdida de foco.

Si sucede otra vez, envía ese archivo y `/re4bridge status`. El status añade
capturas, GPU ocupada, imágenes codificadas/descartadas, tiempo de codificación
y edad del último frame. Abrir el chat/status desarma como antes; una lectura
posterior no identifica por sí sola el instante de la interrupción. Si no hay
un evento VIEW a esa hora, eso ayuda a separar el problema de la presentación
de una parada del cliente/DLL. No hay rearmado ni recuperación de foco automáticos.

## Implementación

- Lectura GPU mediante tres pixel-pack buffers y fences OpenGL3.2, sin esperas
  `glFinish` ni lectura síncrona de una imagen todavía pendiente. Al saturarse
  los tres buffers se omite la captura, sin sobreescribir trabajo en curso.
  Solo se copia el resultado completado más reciente; un resultado tardío no
  puede hacer retroceder la animación.
- Buffers de dibujo GUI/mano, arrays de readback y scratch de conversión reutilizados. Compresión y conversión continúan
  fuera del hilo de render; el alpha255 evita divisiones innecesarias.
- Capa visual y TCP hasta60Hz; los inputs siguen requiriendo un tick fresco
  de Minecraft dentro de200ms. El encoder nunca prolonga esa autorización.
  El worker combina imagen nueva y selección del último tick; las imágenes de
  inventario no se mezclan con botones de movimiento del modo de juego.
- La DLL reutiliza su textura y último header validado si una escritura de
  imagen está en curso. Ese header conserva su timestamp original: caduca a500ms
  y no cruza generaciones. Stop/F8/DLL desarmada siguen cortando inmediatamente.
- ACK leídos mediante stream con buffer, conservando el límite/validación JSON.
- Una captura demasiado grande se descarta, sin matar el encoder ni desarmar
  la cámara. La imagen anterior sigue su caducidad normal; no se envían imágenes
  sobre el límite ni se mantiene una interfaz congelada indefinidamente.
- VIEW coalesce movimientos de menú a la cadencia de presentación; antes de
  un clic se envía la posición pendiente. Se conservan los clics, la rueda,
  botones simultáneos y liberaciones. En juego no se duplican movimientos raw.

## Colisiones del escenario y ocultación visual

Se preparó y comprobó una base de consultas sobre los triángulos exportados:
impactos de rayos, suelo inclinado e intersecciones de cajas con superficies.
Todavía **no está conectada a Minecraft** ni incorpora daño o flechas.
El resultado y la integración necesaria se describen en
[RE4-GEOMETRY.md](RE4-GEOMETRY.md). El render de bloques todavía no utiliza el
depth buffer de RE4: ver bloques detrás de árboles sigue siendo un pendiente.
