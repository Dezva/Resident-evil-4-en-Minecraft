# 0.4.32 — cockpit de RE4 y barra vanilla de Minecraft

La captura del usuario confirma que0.4.30/31 no ocultaban vida/munición de RE4.
F8 debe devolver RE4 normal y restaurar ese HUD; no es una tecla independiente
para alternar el HUD. Los iconos planos del puente tampoco reproducían la barra
real de Minecraft. Esta entrega cambia DLL, mod y la compatibilidad del host.

## Actualización

1. Cierra RE4, Minecraft, VIEW y el host. Ejecuta UNINSTALL del paquete anterior,
   extrae0.4.32 y ejecuta su INSTALL-RE4-CONTROLS. La DLL nueva es build432.
2. En mods deja **solo re4bridge-0.4.32.jar** del puente, reemplazando0.4.30.
   Conserva Fabric API0.116.6+1.21.1 y Fabric Loader/Minecraft1.21.1 habituales.
3. Abre ambos juegos en ventana y START-RE4-CONTROLS del paquete nuevo. En VIEW,
   pulsa **Preparar Minecraft antes de arm**; conserva la configuración anterior
   de controller, mouse/primera persona, blocks y HUD. Haz arm, cierra el chat y
   pulsa F9 desde Minecraft, sin minimizarlo.
4. Comprueba que en primera persona desaparecen el círculo de vida/balas y
   el nombre Leon. Cambia de ranura y comprueba piedra, tronco, losa y cristal:
   sus iconos deben tener la perspectiva de la hotbar de Minecraft. Las cantidades
   y barras de durabilidad usan el renderizador del juego, no dígitos propios.
5. Pulsa **F8**: se detiene el puente y vuelven cámara, controles y HUD normal
   de RE4. Repite arm/F9 para comprobar que la ocultación se aplica de nuevo.

No necesitas repetir la exportación ni las pruebas del cubo para este cambio.
Si el HUD nativo sigue visible, /re4bridge status indica `HUD RE4=ocultacion
 disponible` o `firma no disponible`. Envía esa línea y una captura. Una firma
no validada no activa escrituras alternativas ni direcciones adivinadas.

## Qué cambia

La ocultación anterior tomaba prestado DPF_COCKPIT en Global.Flags_DISP+0x58.
Ese intento no ocultó los elementos ID ya creados en la prueba real. Se retira
por completo. Ahora se valida la firma única del IDSystem estático de UHD1.1.0
(public SDK fijado en native/README.md) y se toman solo los tres bits de
m_disp_off correspondientes a LIFE_METER0x21, BLLT_ICON0x32 y LASER_GAUGE0x34.
Se conservan/restauran sus valores originales al salir de FP, caducar el control,
parar o pulsar F8; otros bits permanecen intactos. No se borran unidades ID ni
se cambian salud, munición o inventario. La capability16 informa de la firma
validada; no certifica la apariencia visual. No se modifica la vtable del HUD.

Minecraft llama a su **InGameHud.renderHotbar** sobre un framebuffer transparente
256×32: modelos GUI tridimensionales, texturas instaladas, selección, mano
secundaria, cantidades, durabilidad, cooldown y brillo según vanilla. Esto
reemplaza las partículas planas y el font numérico propio. No se distribuyen
imágenes del juego. Sigue siendo la hotbar: no se añade todavía la pantalla E
de inventario a RE4, ni corazones/hambre/mano/combate de Minecraft.

La captura GL ocurre solo en el callback de render, con buffers separados y
restauración en finally de framebuffer, viewport, matrices, shaders/cachés,
luces, texturas, blend, profundidad, scissor y configuración de lectura/PBO.
El publicador de ticks solo consume la imagen cacheada. Inventarios estáticos
no vuelven a capturarse hasta cambiar selección/objetos/recursos; las animaciones
se limitan a10 capturas/s. F1 respeta la ocultación de la GUI de Minecraft.
La fila de píxeles se invierte y convierte RGBA premultiplicado a ARGB recto
antes del IPC. HUD v1/32792, control v2/80 y escena v6/96352 conservan su layout.
La última apariencia y el coste GPU necesitan la comprobación Windows indicada.

## Exportación recibida

re4-map-20261012-030617: lectura completa de los managers SAT/EAT para el radio
local solicitado, sin errores. SAT tiene18 referencias coincidentes al manager
RVA0x7f9a7c, distinto de EAT0x7f9aa0. Las tablas también son distintas: no es
una copia duplicada. SAT aporta205 triángulos y EAT225; en total430, clasificados
como98 suelo,109 pendientes y223 paredes. El OBJ y el visor contienen esa misma
geometría relativa al jugador, escala1000, radio20000 unidades nativas.

Esto valida la extracción de colisiones locales, **no un mapa completo de toda
la partida ni la malla visual**. No se publica la geometría original enviada.
Todavía no se ha importado como suelo/losas/obstáculos en Minecraft, ni bloquea
las colocaciones dentro del escenario. Se conserva el suelo temporal del editor.
La siguiente integración del mapa debe usar estos polígonos como soporte y
ocupación, sin tratar un hueco/error de lectura como espacio libre confirmado.

## Validación / recuperación

115 pruebas Python,25 core y32 Minecraft pasan;11 fixtures C con ASan/UBSan y
4 suites C portátiles pasan. Compilación PE32 de DLL y remap/validación de
access widener de Fabric completados. Los fixtures nativos prueban ocultación,
reaplicación y restauración de los bits de clase, incluyendo bits ajenos y layout
inválido; Java prueba orientación/canales/alpha y límites del readback. El TCP
prueba la capability31 junto a HUD, movimiento, sprint y parada existentes.
No hay prueba visual ni ejecución de RE4/OpenGL real en esta nube.

DLL SHA256:24e43b11e2bfc8b652d6b91b96456bbd792480031e785f36bc1f126c9a781b81.
Si hay una regresión, vuelve al paquete0.4.31 completo con DLL430 y mod0.4.30;
no mezcles DLL/host/JAR de entregas distintas. VIEW conserva la corrección de
clics/F9 de0.4.31. Cámara, velocidades y colisiones no se recalibran en esta entrega.
