# 0.4.12: detección del dispositivo gráfico y retorno de foco

La captura de0.4.11 muestra **estado3, frames0, dibujos0**: el detector no
enganchó ningún dispositivo y nunca ejecutó el dibujado. No permite concluir
si la caja estaba bien situada. Además, controles estaban detenidos por parada
nativa1; después de F8/chat hace falta otro arm para mostrar el bloque.

Esta versión amplía la búsqueda: ya no exige tabla completa idéntica a la sonda
ni que la ventana pertenezca al hilo de entrada. Valida tres métodos de consulta
contra Direct3D9 del sistema y comprueba que todos los métodos pertenecen a ese
módulo ejecutable. Recorre regiones de datos del juego separadamente y examina
hasta256bytes de estructuras referenciadas por esos datos (máximo4096 raíces
únicas). Solo acepta un dispositivo de una ventana visible del mismo proceso;
si hay dos, no instala. No escanea todos los heaps ni admite wrappers arbitrarios.

Esto elimina restricciones del detector, **no demuestra todavía que encuentre
el dispositivo real o que se vea el bloque**. La prueba en Windows sigue pendiente.
Añade contadores para localizar la causa si vuelve a fallar.

## Actualizar desde0.4.11

1. Cierra RE4, el host y VIEW. Ejecuta **UNINSTALL-RE4-CONTROLS del paquete0.4.11**.
2. Extrae0.4.12 en otra carpeta y ejecuta su **INSTALL-RE4-CONTROLS**.
3. **Conserva re4bridge-0.4.11.jar y Fabric API**. El ZIP completo incluye esos
   mismos mods; no pongas una segunda copia del JAR.
4. Abre RE4 en ventana y carga una partida. Inicia el **START-RE4-CONTROLS de
   la carpeta0.4.12** con los permisos de RE4. Usa también su **VIEW-RE4**.
5. En Minecraft: `/re4bridge controller`, apunta al bloque colocado,
   `/re4bridge blocktest`, `/re4bridge arm`. Cierra chat; F9 muestra RE4.
   Espera unos15 segundos desde cargar la partida para terminar la detección.

Con ratón/primera persona activados puedes usarlos como antes. Si el bloque
aparece, camina alrededor y comprueba que permanece fijo. Romper el bloque
seleccionado lo retira. **El cubo sigue siendo de líneas verdes, sin colisión,
texturas ni ocultación por terreno.** No copia el mundo superplano o sus mobs.

## Si no aparece

Comparte `/re4bridge status` y **la última línea `BLOQUE DIAGNOSTICO:` del host**.
La línea se imprime al cambiar estado/intento y no contiene el token de conexión.
`discovery` informa:

- `attempts`: intentos de detección (máximo5, cada2segundos).
- `slots`: punteros de objetos legibles y escribibles examinados.
- `tables`: candidatos con tabla gráfica validada.
- `devices`: candidatos válidos de ventana de RE4 (incluye referencias repetidas).
- `window_rejected`: tabla válida pero ventana de otro proceso, oculta o de la sonda.
- `roots`: estructuras únicas examinadas indirectamente;4096 indica el límite.

Estado1 significa hook instalado; **frames debe aumentar**. Dibujos requiere
selección, arm vigente y líneas visibles. Estado2 indica sonda fallida;
3 no encontró dispositivo;4 ambiguo/cambio al instalar. La captura actual
se tomó tras una parada, así que no sirve para comprobar si se dibuja armado.

## Retorno con F8

Siempre se intenta detener el puente y restaurar la vista antes de cambiar foco.
Si Windows rechaza activar RE4, el panel indica que pulses su ventana en vez de
mostrar una excepción de retorno. No vuelve a armar por sí solo.

## Validación y límites

DLL PE32 compilada con advertencias como errores;4 suites C.43 pruebas Python,
incluyendo detector/callback reales compilados con memoria/COM simulados, variantes
de tabla, referencias indirectas, regiones divididas, ambigüedad y rechazos.
IPC visualv2 de92bytes conserva los primeros68bytes; host admitev1/v2 y preserva
contadores nativos al publicar/desarmar. Backend/acción real de VIEW probados con
rechazo de foco simulado. No se ejecutaron D3D9/WinForms ni los juegos en Linux.
Mod0.4.11 sin cambios, previamente verificado con17 pruebas core y2 Minecraft.
No se altera el algoritmo de cámara, inventario, guardados o colisión.

Para revertir: cierra RE4/host/VIEW, UNINSTALL0.4.12 e INSTALL0.4.11; usa host/VIEW
anteriores. Conserva el mod0.4.11 y las copias de seguridad.
