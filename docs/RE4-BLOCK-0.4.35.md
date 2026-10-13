# 0.4.35: preparar con un botón, geometría compartida y profundidad

## Instalación y uso

Con ambos juegos cerrados, desinstala la DLL anterior con el desinstalador y
extrae el paquete nuevo. Instala su DLL con `INSTALL-RE4-CONTROLS.bat`.
En `.minecraft/mods` sustituye el JAR antiguo por **re4bridge-0.4.35.jar**;
conserva Fabric API. No mezcles host, mod, VIEW o DLL de otras versiones.

1. Abre RE4 **en ventana**, carga una partida. Abre Minecraft **en ventana**,
   sin minimizar, en un mundo local creativo. Cierra chat e inventario.
2. Abre **START-RE4-CONTROLS.bat** y **VIEW-RE4.bat**.
3. Pulsa **Preparar Minecraft**. Conecta controller, activa ratón/primera
   persona, HUD de Minecraft, construcción con texturas, colisiones de bloques y seguimiento. Arma
   el puente y espera confirmación del juego. No necesitas escribir comandos.
4. Cuando el panel diga **Minecraft preparado**, pulsa **F9** para jugar.
   E abre el inventario. **F8** devuelve RE4 normal con su interfaz.
   Para regresar al puente: **Preparar Minecraft → F9**.

La preparación mantiene el panel disponible y F8 puede cancelarla. No rearma
tras perder foco. Repetirla dentro de la misma sesión conserva las construcciones
registradas y su referencia. Cambiar de mundo, reconectar o reiniciar puede
requerir recuperar la referencia; esta versión no añade persistencia entre sesiones.
VIEW busca la configuración en `%APPDATA%\.minecraft`; para otro perfil ejecuta
`VIEW-RE4.bat -GameDir "ruta del perfil"`. El endpoint del mod solo admite
la acción fija de preparación, autenticada y en localhost, junto al puerto del host.
No prepara partidas ni instala archivos al pulsar el botón.

## Qué probar

Primero prueba **Preparar → F9 → F8 → Preparar → F9**. No uses controller/arm
entre estos pasos. Luego mueve rápido el ratón y alterna varias teclas.
Una lectura temporalmente ocupada de los contadores gráficos/colisión ya no
cierra el host: conserva el último diagnóstico mientras los inputs siguen
su autorización y caducidad normales. Los errores reales detienen la sesión.

Para geometría, mira el suelo o un muro de RE4 y coloca piedra. La selección
consulta superficies nativas y la colocación comprueba el volumen del bloque;
puede apartarlo hasta dos celdas de una superficie irregular para evitar
enterrarlo. Si el mapa local está pendiente, caducado o incompleto, no permite
colocar. No convierte automáticamente todo el mapa en cubos o losas.

Para profundidad, coloca un bloque detrás de un tronco y míralo desde el otro
lado. El dibujo ahora usa el depth buffer y una proyección validada de RE4,
antes de la limpieza de profundidad para su interfaz cuando se detecta ese
paso. El tronco debe ocultar la parte del bloque que quede detrás. Superficies
transparentes o que RE4 no escriba en profundidad pueden seguir teniendo límites.
`/re4bridge status` informa mapa/profundidad; `CAPTURE-RE4-GRAPHICS.bat` guarda
las matrices y el estado de profundidad sin activar dibujo.

## Alcance de las colisiones

El host captura SAT/EAT en un worker de lectura, como máximo dos veces por
segundo, solo con primera persona armada y solicitada. La búsqueda de firmas
se reutiliza; los objetos y transformaciones siguen validándose. Cada captura
limita lecturas a 32 MiB y envío a 1024 triángulos en un radio de 12 bloques
nativos. Capturas parciales o sobredimensionadas representan espacio desconocido.
No genera chunks con una copia completa del mapa y no hace lecturas en el hilo
de inputs. La caducidad del mapa es independiente de la de controles.

Los rayos de construcción y las flechas de tu jugador local consultan superficies
nativas y bloques construidos, ignorando el suelo auxiliar de Minecraft. Las
flechas usan el segmento completo del tick y quedan detenidas en una superficie
nativa estática durante la sesión. **Todavía no se dibujan las flechas como entidades
3D en RE4, no dañan enemigos de RE4 ni acompañan objetos móviles**; recogida y
persistencia de flechas nativas también quedan pendientes. Esto es la base física.
No cambia proyectiles de otros jugadores ni un servidor externo.

La malla representa superficies de colisión, no todos los objetos decorativos
ni volúmenes sólidos cerrados. No sustituye las colisiones de movimiento de RE4.
La oclusión visual usa su profundidad gráfica, separada de SAT/EAT.

## Validación

Compilación del mod Fabric 1.21.1 y DLL PE32, pruebas de autenticación/cancelación,
rayos y cajas rotadas, lecturas ocupadas y geometría incompleta. Fixtures del
renderer comprueban profundidad normal/inversa, clipping cercano, generación,
F8, búfer ausente, proyección inválida y restauración gráfica; también se ejecutan
con AddressSanitizer/UndefinedBehaviorSanitizer. La presentación Win32 y el
resultado visual del depth buffer requieren comprobarse con RE4 real en Windows.
