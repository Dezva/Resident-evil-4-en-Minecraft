# 0.4.18 — cubo gráfico de prueba

Esta versión dibuja **las aristas verdes de un cubo** delante de la cámara de
RE4 al iniciar una prueba explícita. El cubo queda fijo en el escenario: caminar
o girar debería cambiar su posición en pantalla. Todavía no tiene textura,
ocultación por terreno, colisión ni colocación desde Minecraft. Esta prueba
verifica el dibujo antes de conectar los bloques del mod.

Se parte de0.4.17, cuya captura gráfica confirmó tu informe. Los JAR del mod
Minecraft0.4.11 y Fabric API siguen iguales. Controles/cámara/correr no cambian.
No se activa dibujo al abrir RE4, el host o CAPTURE-RE4-GRAPHICS.

## Instalar y probar solo con RE4

1. Cierra RE4, host y VIEW. Ejecuta **UNINSTALL del paquete0.4.17 instalado**.
2. Extrae0.4.18 y ejecuta su **INSTALL-RE4-CONTROLS**. Puedes reutilizar la carpeta
   tras desinstalar y retirar los archivos anteriores.
3. Abre RE4 **en modo ventana** y carga una partida. Para esta prueba no necesitas
   Minecraft, host, VIEW, controller/arm ni primera persona.
4. Sitúate en un lugar abierto, mirando hacia el frente. Ejecuta
   **TEST-RE4-CUBE.bat** con los mismos permisos que RE4.
5. La consola cuenta **cinco segundos**. Vuelve a RE4 con Alt+Tab durante esa
   cuenta. Al llegar a cero se activa la prueba durante **30 segundos**.
6. Busca el cubo de aristas verdes delante de la cámara. Camina unos pasos y
   gira para comprobar que se mantiene fijo en el escenario. **F8 lo detiene**;
   también desaparece al terminar los30segundos.
7. Comparte **cube-test-*.json**, que queda junto al BAT, e indica si viste el cubo.
   Una captura o vídeo corto ayuda a comprobar su proyección visual.

Si no aparece, comparte igualmente el informe. Contadores de frames/dibujos,
rechazos de cámara/pases, fallos y HRESULT ayudan a distinguir la causa.
CAPTURE-RE4-GRAPHICS incluye ahora el estado del cubo y sigue siendo solo lectura.
No uses `/re4bridge blocktest`: su renderer anterior sigue retirado.

F8, cerrar la consola o dejar de renovar la activación detienen el dibujo. La
autorización nativa caduca a los500ms desde la última publicación; se renueva
durante la prueba. Solo un proceso de prueba puede publicar a la vez. Después de
F8 o de reiniciar/perder el dispositivo, inicia otra prueba manualmente; el host
no la rearma. Un fallo al restaurar estados exige reiniciar RE4.

Si RE4 se cierra al arrancar, retira la DLL con UNINSTALL de0.4.18 y reinstala
**0.4.17**, cuya captura ya funcionó. Conserva el paquete anterior. El recolector
COLLECT-RE4-CRASH puede recoger eventos sin abrir RE4.

## Implementación y límites

Se recibe el dispositivo de una llamada CreateDevice correcta. Una fachada
externa IDirect3DDevice9 con sus119métodos delega al objeto original y balancea
referencias propias. No se modifica el objeto ni su vtable nativa. Si falla la
reserva del wrapper o el canal, devuelve el dispositivo original. DllMain solo
valida/intercepta la importación, como0.4.17; no crea wrappers/IPC/recursos GPU.

El dibujo ocurre antes del EndScene original y solo tras un BeginScene correcto,
una orden completa/fresca y un jugador/cámara válidos. Se verifica que el destino
actual sea el backbuffer. Se proyectan12aristas usando CAMERA.v_mat/FOV, clipping
y bounds finitos. Se guarda D3DSBT_ALL, configura dibujo simple, restaura y libera
el backup en el mismo frame. Un error de setter/dibujo detiene esa prueba y se
intenta restaurar; un error de restauración bloquea toda activación hasta reiniciar.
Reset/Present delegan argumentos y resultados; no se conservan recursos GPU
entre frames ni se fuerza recrear el dispositivo.

Cada fachada serializa llamadas/dibujo/Reset con un bloqueo reentrante; el
dibujo permanece suspendido durante Reset. Las interfaces desconocidas se
delegan al original y desactivan el dibujo de esa fachada si escapan. Los recursos,
swapchains y GetDirect3D conservan interfaces nativas: GetDevice en un recurso
puede devolver el dispositivo original, cuya identidad difiere de la fachada.
No se acredita cobertura de rutas externas ni de accesos concurrentes mediante
esas interfaces. La compatibilidad real con RE4 requiere esta prueba en Windows.

Command IPC32bytes es el único canal escrito por TEST; no escribe memoria del
proceso, EXE ni partidas. Telemetría independiente92bytes/version1/build418;
lector FILE_MAP_READ y snapshots completos. CaptureIPC mantiene v3/3296bytes.
El indicador `last_scene_active` describe el último frame observado, no una
autorización vigente; el informe de prueba registra por separado el periodo y
motivo de parada. Cerrar la prueba desactiva la orden aun al producirse errores.

## Validación cloud

DLLPE32 compilada con warnings como errores. Pasan70 pruebas Python y4 suites C;
seis fixtures nativos de captura/cubo pasan ASan/UBSan. La fachada se ejecuta con
los119slots tipados: argumentos/resultados, identidad, referencias con interfaces
nativas vivas, fallos de asignación, llamadas de escena/Reset/Present y LastError.
Comprueba reentrada durante Reset y rechazo del dibujo mientras se reinicia.
El renderer real se ejecuta con estados simulados:23setters fallidos, dibujo
fallido, Capture/CreateStateBlock fallidos, Apply fallido y bloqueo posterior,
referencias temporales equilibradas, target auxiliar, cámara ausente, lease/F8
y anclaje fijo. Reader nativo rechaza publicaciones incompletas y no reutiliza
activaciones. CLI verifica snapshot sin escritura, mutex, limpieza/cierre,
cuenta atrás/duración/F8/parada nativa con reloj virtual e informe al fallar.
La nube no ejecuta RE4/Direct3D real: el arranque, el cubo visible y la proyección
correcta siguen pendientes de tu prueba; los contadores por sí solos no los prueban.
