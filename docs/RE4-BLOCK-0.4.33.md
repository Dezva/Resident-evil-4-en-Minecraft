# 0.4.33 — interfaz, inventario, mano y contorno de selección

La captura0.4.32 confirma que vida/balas seguían visibles. La firma se encontraba
(capabilities31/native_hud_supported=True), pero eso no probaba la ocultación.
F8 sigue siendo **volver a RE4 normal**, nunca un botón independiente de HUD.

## Instalar y probar

1. Cierra RE4, Minecraft, host y VIEW. Ejecuta UNINSTALL del paquete anterior y
   después INSTALL del nuevo. La DLL es build433. Reemplaza el mod anterior por
   **re4bridge-0.4.33.jar**, dejando una sola copia del puente. Fabric API sigue
   siendo0.116.6+1.21.1; Minecraft/Fabric Loader conservan las versiones habituales.
2. Abre ambos juegos en ventana y START-RE4-CONTROLS del paquete nuevo. Abre
   su VIEW y pulsa **Preparar Minecraft antes de arm**. Se ajustan ambos juegos
   al monitor; Minecraft queda detrás y recibe los inputs. F9 muestra RE4 sin
   activar ni redimensionar durante el armado. F8 restaura las dos ventanas.
3. Conecta `/re4bridge controller`. Usa `/re4bridge firstperson` y comprueba que indique `true` (también activa
   el ratón); `/re4bridge blocks on` prepara la construcción y su suelo auxiliar.
   La interfaz Minecraft está activada por defecto. Si la habías desactivado en
   esta sesión, usa `/re4bridge hud on`: esto controla la interfaz de Minecraft,
   no la vida/balas de RE4. Después `/re4bridge arm`, cierra el chat y pulsa F9.
4. Comprueba la ocultación automática de vida/balas/nombre Leon, la barra con
   iconos detallados y la mano de tu skin con el objeto elegido. Selecciona una
   ranura vacía para ver la mano sin objeto; coloca/rompe para comprobar el swing.
5. Pulsa **E**: debe aparecer el inventario real de Minecraft (creativo en el
   modo creativo usado por el editor). Prueba mover el cursor, cambiar pestaña,
   arrastrar objetos, Shift-clic y rueda. W/A/S/D no deben mover a Leon mientras
   el inventario esté abierto. E/Esc lo cierran sin requerir otro arm. Alt-tab,
   pausa, desconexión, F8 o fallo nativo siguen cancelando el control.
6. Mira el suelo auxiliar y los bloques colocados: el contorno negro marca la
   celda/caja seleccionada. Sobre el suelo auxiliar muestra la cara superior;
   sobre un bloque existente muestra su volumen. Luego F8: debe regresar RE4
   normal, con su HUD, armas/cámara y tamaño original de ventana.

Si el HUD nativo continúa visible, envía una captura y `/re4bridge status`.
`Ultimos ID HUD`/`native_hud_candidates`/`native_hud_hidden` registran la última
captura activa del administrador de interfaz y ayudan a separar firma, tabla
vacía y elementos gestionados. Leer status/chat desarma como antes: esos últimos
contadores no certifican que el HUD esté oculto en ese momento ni visualmente.

## Cambios de implementación

La visibilidad nativa ahora se mantiene **durante toda la sesión FP**. Las leases
por actualización de cámara/modelos se restauran separadamente: antes se devolvía
la visibilidad del cockpit entre callbacks. Se conservan los bits display-off de
LIFE_METER/BLLT_ICON/LASER_GAUGE y se gestionan también sus primitivas ID_UNIT reales
(bit visible0x8, stride0x144; SDK ID.h/HUDTweaks.cpp fijados en native/README.md).
La tabla/puntero/capacidad/región deben validarse; solo se tocan esas tres clases.
Hasta256 leases conservan identidad del slot, visibilidad original y bits ajenos.
No se borran unidades ni se cambian vida, munición, inventario o animación de RE4.
F8/stop/TTL/cámara inválida restauran; cambios de tabla/slot no restauran sobre
objetos distintos. No hay parche de vtable del HUD ni un rectángulo negro que lo tape.

La barra antigua se dibujaba a256×32 y se ampliaba: los modelos de iconos partían
de16 píxeles. Ahora se captura una capa transparente completa hasta1600×900;
1366×768 se conserva a su resolución. InGameHud.renderHotbar, HeldItemRenderer y
Screen.renderWithTooltip dibujan los recursos/skin/interfaz instalados. La mano,
objeto, overlays, durabilidad y pantalla E son del renderer de Minecraft. No se
incluyen recursos de Minecraft ni mallas del mapa RE4 en el ZIP. La mano es visual:
no implementa todavía daño de espada/flecha a los actores de RE4.

La captura se ejecuta al finalizar GameRenderer.render, incluso con inventario
abierto. F1 oculta GUI/mano durante juego; E sigue visible, como en Minecraft.
Los targets de elementos translúcidos se conservan dentro del framebuffer propio;
el blur de pantalla no procesa el framebuffer principal durante esta captura.
El estado GL/render/matrices/texturas/cachés se restaura; no se lee la imagen del
mundo para añadirla sobre RE4. Cursor propio solo dentro del inventario.

La conversión y compresión lossless ocurren en un hilo daemon separado, con una
cola de un frame pendiente que se sustituye por el más nuevo. Imágenes de más de
500ms no se reutilizan. Captura hasta20Hz; esto limita la animación de mano/UI,
no la cadencia del renderer de RE4. Se conservan los límites del protocolo512KiB:
zlib ARGB hasta196608 bytes comprimidos; expansión exacta y acotada, sin colas
ilimitadas ni imagen por escritorio. Fallos de captura/codificación detienen la
interfaz/control. El coste de lectura GPU y el resultado visual requieren prueba
real; no se promete equivalencia de FPS o latencia con Minecraft independiente.

Un canal independiente OverlayCommand v1 (128-byte cabecera +1600×900×4 bytes)
transporta imagen/revisión/selección; control v2/80, escena v6/96352 y HUD legado
v1/32792 permanecen. El host exige DLL y mod nuevos para la capa comprimida.
El dispositivo conserva textura y buffer de imagen en heap, sin grandes copias
en stack; revisiones estables no copian/uploadan los píxeles otra vez. Estados
D3D se capturan/restauran y se usan alpha y filtro lineal/texel alignment.

El contorno negro de2px usa el view/FOV nativo coherente de los bloques, con una
transformación fija desde el outline shape seleccionado en Minecraft. No es una
marca fija del HUD. No añade colisiones. **No lee todavía el raycast del escenario
RE4 ni impide construir dentro de un árbol/estructura**; el suelo es la cuadrícula
auxiliar, no una reconstrucción precisa del relieve. El mapa SAT/EAT430 triángulos
recibido sigue validado pero sin importar como terreno. Esa integración sigue pendiente.

VIEW ajusta/restaura también Minecraft para que la escala de GUI y las coordenadas
reales del cursor coincidan con la presentación. El IPC de menú se lee solo con
cabecera/TTL válidos durante una sesión activa; movimiento de cursor se reenvía
solo en menú y deja continuar el movimiento de Windows. En juego no se duplica
raw mouse. Clics/rueda, foreground/HWND/PID y limpieza conservan las guardas31.
E no desarma: mantiene generación, con movimiento bloqueado; pausa/chat/pérdida
de foco/stop siguen desarmando y no recuperan el control automáticamente.

## Validación

119 pruebas Python,26 core y34 Minecraft pasan;12 fixtures nativos con ASan/UBSan,
4 suites C portátiles, compilación Win32 y fixtures PowerShell de VIEW/instalación
pasan. Se prueban cola/último menú/cierre, codec1366×768, controles modal/foco/F8,
zlib truncado/extra/bomba/límites, IPC sin mutación ante errores, revisión y cursor.
El renderer nativo real se compila con APIs simuladas:200 imágenes cacheadas,
contorno negro, alpha/linear,45 errores de setters/restauración y leases/TTL/Reset.
La nueva lease ID se prueba con unidades visibles/ya ocultas/clase ajena y con
restauración/cámara/proyección. Windows, GLFW, OpenGL/D3D reales y RE4 no se ejecutan
en esta nube: la prueba visual del usuario sigue siendo necesaria.

DLL SHA256:f707036409bad6f7044baf49fdc29f790af8f654ff11617730aa350055968a11.
Recuperación: instalar el paquete0.4.32 completo (DLL432 + mod0.4.32 + host/VIEW32).
No mezclar componentes. Cámara, velocidad nativa y colisiones físicas conservan
las políticas anteriores; esta entrega no añade salto ni combate.
