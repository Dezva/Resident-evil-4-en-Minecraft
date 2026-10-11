# 0.4.20 — tamaño y detección del bloque visual

Tu captura confirma que `blocktest 100` dibuja, pero el cubo queda pequeño cerca
del suelo. `blocks on` aún no produjo un bloque visible en tu prueba. Esta versión
aumenta el tamaño inicial a **1000 unidades RE4** y refuerza la detección de
colocaciones. Es una escala provisional ajustable, no una conversión métrica
calibrada. **Todavía se dibuja un solo cubo de aristas verdes: la última colocación**,
sin una línea de bloques, textura, superficies sólidas ni colisión.

## Actualizar desde 0.4.19

1. Cierra Minecraft y el host anterior. Extrae el paquete 0.4.20 en una carpeta
   vacía o retira los archivos anteriores antes de extraerlo en la misma carpeta.
2. En `%APPDATA%\.minecraft\mods`, retira el JAR anterior de RE4 Bridge y copia
   **re4bridge-0.4.20.jar**. Debe quedar un solo JAR de RE4 Bridge. Conserva Fabric
   API 0.116.6+1.21.1, Minecraft 1.21.1 y Fabric Loader 0.16.10.
3. Usa **START-RE4-CONTROLS del paquete 0.4.20**. **No ejecutes INSTALL ni UNINSTALL**
   para esta actualización: la DLL es exactamente la misma de 0.4.19 y permanece
   instalada. RE4 puede seguir abierto con esa DLL. Los scripts del paquete se
   conservan para instalaciones nuevas; estas siguen la guía de instalación 0.4.19.

## Prueba automática

1. Carga una partida RE4 en una zona abierta. Mantén ambos juegos en ventana,
   Minecraft con foco, en tu mundo local y modo creativo. VIEW es opcional.
2. Inicia el host nuevo; cierra otros hosts y TEST-RE4-CUBE.
3. En Minecraft usa **/re4bridge controller**, espera la conexión y después
   **/re4bridge blocks on** y **/re4bridge arm**.
4. Cierra el chat y espera a ver **activo | arm=si**. Coloca una piedra.
   Debe aparecer el mensaje **Bloque colocado detectado**, y el indicador debe
   mostrar **bloque=detectado | enviado=si | dibujos=N** con dibujos aumentando.
   El cubo debería quedar fijo delante de Leon, a tres tamaños de distancia.
5. Rompe la piedra seleccionada: debe desaparecer. Otra colocación sustituye
   al cubo anterior. Romper un bloque anterior no retira el último.
6. F8 detiene control y dibujo; `blocks off` además desactiva la observación.

Si sigue sin aparecer, conserva una captura del indicador **antes de abrir el
chat** y copia `/re4bridge status`. Incluye detección por evento/tick, confirmaciones,
selección, tamaño, solicitud al host, dibujos y pases omitidos. `enviado=si`
significa que el host ha publicado la orden; no acredita visibilidad. El motivo
de la última escena puede quedar en `desactivado` si los siguientes pases omiten
la caja. Los pases omitidos incluyen targets ajenos y cajas fuera de la vista.
Los contadores son acumulados; lo relevante es si aumentan durante la prueba.

Para comparar el tamaño manualmente, mira un bloque ya colocado y usa
**/re4bridge blocktest** (sin `100`), después **/re4bridge arm**. El valor explícito
`blocktest 100` conserva su significado y seguirá dibujando un cubo pequeño.
Puedes ajustar con `blocks on 800` o `blocktest 800`; rango permitido 40..2500.
El anclaje aún usa posición y orientación de Leon, no las coordenadas del bloque
en Minecraft; el terreno puede quedar por encima o debajo de la base de la caja.

Abrir chat, pausar o perder foco desarma temporalmente por diseño. No necesitas
mouse/firstperson para probar bloques. Esta actualización no cambia su cámara.

## Cambios y verificación

Además de UseBlockCallback se observa la tecla de uso al principio de cada tick,
antes de la colocación de Minecraft. Una cola limitada conserva intentos durante
diez ticks y evita que el segundo evento, otra mano o un clic posterior reemplace
un intento aún sin confirmar. La observación puede ocurrir al salir del chat de
arm; el dibujo sigue requiriendo control armado y muestras recientes. Un intento
sin cambio al material esperado no crea caja. El diagnóstico permite distinguir
ausencia de detección, orden no enviada y dibujo omitido. La causa exacta del
fallo anterior aún no está confirmada en un cliente real.

Pasan 73 pruebas Python, 17 bridge-core y 10 Minecraft. Se verifican duplicados,
confirmación retrasada, ambas manos, caducidad, dimensión, límite de cola y
publicación/retirada con TCP autenticado y APIs Windows simuladas. Compilación
JDK21/Fabric correcta. DLL y scripts de instalación son idénticos a 0.4.19; sus
pruebas nativas anteriores siguen aplicando. La colocación automática y escala
visual requieren comprobarse en tu Windows.
