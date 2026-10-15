# 0.4.53: golpes letales, cajas y recogida de objetos

Conserva clic izquierdo, daño de herramienta/espada, alcance, recarga y partes
alcanzadas de 0.4.52. Corrige un camino que devolvía daño cero cuando RE4
procesaba el impacto en otro hilo, dejándolo caducar sin confirmación. El mensaje
propio del puente tampoco conserva la bandera nativa de daño no letal. Los
impactos ajenos al puente mantienen su daño y sus banderas originales.

Las cajas de madera `cEmBox` reciben el mensaje de impacto del juego. La
confirmación de su rotura espera la transición nativa de intacta a rota, vida
cero y desactivación de su bandera de objeto intacto; vida cero por sí sola no
basta. No se escriben rutinas ni se oculta la caja: RE4 ejecuta su rotura, sonido
y aparición del objeto. Este seguimiento también cubre la ruta que no llama
al helper general de daño. No convierte decorados o cofres de abrir en cajas
rompibles. Corrige además el tipo de objetivo en el host, que podía confundirse
con el último actor del catálogo y elegir partículas equivocadas.

## Recoger objetos con Minecraft enfocado

1. Pulsa **R** frente al objeto para abrir la pregunta de RE4.
2. Usa **las flechas** para elegir una opción.
3. Pulsa **Enter**, Enter del teclado numérico o **R** para confirmar.
4. **Retroceso** envía cancelar/volver cuando RE4 lo admite.

Estos controles solo se envían durante la recogida nativa. Enter/R no se repiten
por mantener la tecla pulsada. Flecha y confirmación conservan su orden y dejan
un intervalo de liberación entre pulsos. Cerrar el diálogo, abrir una pantalla
Minecraft, perder el control, caducar o pulsar F8 descarta las acciones pendientes.
Los objetos continúan en el inventario y la lógica de RE4. Arcos, ballestas y
conversión del botín a Minecraft quedan para una actualización posterior.

## Actualizar

Cierra RE4, Minecraft, host y VIEW. Extrae el paquete completo en una carpeta
nueva. Ejecuta su `UNINSTALL-RE4-CONTROLS.bat` y después
`INSTALL-RE4-CONTROLS.bat`; reconoce la DLL 0.4.52 publicada. Sustituye el mod
anterior por `re4bridge-0.4.53.jar`, conserva Fabric API y usa el host de esta
carpeta. Prepara Minecraft y pulsa F9 como antes.

**Actualiza DLL, host y JAR juntos:** build453, Combat v3 de 5324 bytes y
Runtime v5 de 160 bytes. El host rechaza canales Combat/Runtime anteriores.

Compilación, pruebas automatizadas e instalación simulada pasan en Linux.
El diagnóstico del usuario confirma el tipo/estados de caja y la bandera del
menú; todavía falta comprobar esta versión dentro de RE4 en Windows. Prueba
un ganado hasta matarlo, un cuervo, una caja con sonido/botín y la elección
«No»/«Yes» desde Minecraft. Ante un fallo conserva `logs/re4bridge-host.log`.
El resumen añade callbacks recibidos/propios/en otro hilo, mensajes ajenos y
`boxes_broken`, sin volcar el catálogo ni los datos completos del juego.
