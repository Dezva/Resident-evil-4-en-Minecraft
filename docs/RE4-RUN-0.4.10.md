# 0.4.10: correr desde Minecraft

Paquete completo: mod0.4.10, host, DLL0.4.10, Fabric API, START, VIEW,
TRACE e instaladores. La cámara conserva el comportamiento de0.4.9: sigue
siendo experimental y no se presenta como resuelta.

## Actualización desde 0.4.9

1. Cierra Minecraft, RE4 y el host. Ejecuta **UNINSTALL de0.4.9**.
2. Extrae este ZIP en una carpeta nueva. Ejecuta su **INSTALL**.
   Mantiene la copia de seguridad de partidas y no reemplaza DLL desconocidas.
3. En `%APPDATA%\.minecraft\mods`, retira el antiguo mod RE4 Bridge y copia
   `re4bridge-0.4.10.jar` desde `mods` del ZIP. Conserva Fabric API; no dejes
   dos versiones de RE4 Bridge instaladas.
4. Abre ambos juegos y usa el **START-RE4-CONTROLS nuevo** con los permisos
   de RE4. Conserva los comandos `/re4bridge controller`, firstperson y arm.

## Cómo correr

Mantén **W + la tecla configurada para sprint en Minecraft**: Ctrl izquierdo
por defecto. Puedes asignarla a Shift u otra tecla desde Controles de
Minecraft. Usa la tecla configurada aunque estés en espectador/creativo.
El puente lee la tecla mientras la mantienes pulsada; no usa doble W ni
el estado automático de sprint del personaje Minecraft.

En FP, correr también acompaña las diagonales y el movimiento relativo a
mirada A/D/S: el puente orienta al personaje y solicita avance nativo con
KEY_RUN. En tercera persona se conservan los límites del movimiento nativo.
RE4 decide velocidad, animación y colisiones; no se escriben posiciones,
velocidades ni parámetros del personaje.

Al soltar sprint debes volver a caminar; al soltar dirección se retira correr.
W+S o A+D sin otra dirección cancelan su eje y no mantienen correr. F8,
chat/pérdida de foco, TTL y cierre del host siguen deteniendo el control;
un arm nuevo es necesario para reanudar. No añade salto ni daño/construcción.

Prueba W solo, W+Ctrl y después W solo de nuevo, en el mismo tramo libre.
Después prueba diagonal con sprint y F8 mientras corres. Si no corre, revisa
que hayas reemplazado mod, host y DLL; el host debe mostrar `correr=True`.

## Diagnóstico recibido

La captura `camera-trace-20261011-022618.jsonl` contiene223 muestras y confirma
build409 realmente cargado. El ratón acumulado permanece en0 durante las
pruebas armadas. El contador de proyección aumenta2853492→3203928 en45s,
pero **writes se queda en0** y todas las llamadas contadas se rechazan.
Por tanto el ajuste final no se aplicó en esa ejecución. Esto no determina
qué condición de validación lo impide. La cámara sigue desviándose en reposo
y alineándose al caminar; el usuario considera el resultado jugable y
autoriza continuar. No es necesario repetir esa captura por falta de timer.

TRACE conserva elapsed en JSONL y ahora muestra segundos transcurridos y
restantes en su consola. No añade una ventana que tape la imagen de RE4.

## Validación

Compilación Java/Fabric1.21.1 y PE32; pruebas de transporte autenticado con
correr/soltar y rechazo de bits ajenos; política nativa de direcciones
opuestas, correr sin dirección, TTL, stop/desarmado y movimiento relativo.
La velocidad efectiva y animación requieren la prueba en RE4 Windows.
