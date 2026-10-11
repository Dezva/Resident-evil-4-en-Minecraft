# Controles Minecraft -> RE4 UHD 1.1.0

Prueba visual nueva: [0.4.11, bloque seleccionado](RE4-BLOCK-0.4.11.md).
El usuario ya confirmó que correr funciona. Esta versión conserva ese control.

Actualización vigente: [0.4.10 con correr](RE4-RUN-0.4.10.md). Requiere
mod0.4.10, host y DLL nuevos. Mantén la tecla de sprint de Minecraft (Ctrl
izquierdo por defecto) mientras avanzas. La velocidad la calcula RE4.

Para 0.4.0, sigue [raton y camara](RE4-MOUSE-CAMERA.md). El usuario confirmo
W/S/A/D de 0.3.5 sin retraso notable. Las actualizaciones siguientes son historicas.

## Actualizacion del armado y estado visible 0.3.5

Con la DLL 0.3.4 ya instalada (estado 1/ticks), no reinstales el componente
nativo. Cierra Minecraft y el host Python, extrae el paquete nuevo en otra
carpeta, retira `re4bridge-0.3.0.jar` de mods y coloca `re4bridge-0.3.5.jar`.
Conserva Fabric API y la DLL de RE4. Arranca el nuevo host como antes.

El comando arm deja el control pendiente hasta cerrar el chat. Solo entonces
se envian entradas armadas y empieza el plazo de confirmacion del host.
Abrir otro menu/chat, perder foco, perder muestras o pausar siguen desarmando;
no se reactiva automaticamente. Espectador, Creativo y Survival admiten
esta prueba de teclas. El armado no convierte ni sincroniza los personajes.

El indicador sobre la barra de items muestra:

- Estado local: esperando chat, solicitando, activo o motivo de desarmado.
- `teclas`: mascara enviada (W=1, S=2, D=4, A=8 con teclas configuradas).
- `nativo`: confirmacion de armado del host.
- `aplicado`: mascara de input que la DLL aplico en su ultimo tick.
- `host`: activo, cliente desarmado, sin personaje, sin ticks o parada.

Ejecuta controller, espera conexion, arm, cierra chat y pulsa W un segundo.
Observa el indicador sin abrir status ni cambiar de ventana. Envia su texto
o captura si no se mueve. El host incluye el mismo detalle en su consola.
Las pruebas cloud verifican la secuencia y los campos con IPC sintetico;
el movimiento real con esta actualizacion sigue pendiente de prueba.

## Correccion del salto nativo 0.3.4

El lector del usuario confirmo todas las firmas y una entrada E9 (salto relativo)
en RVA 0x7509 que apunta a 0x567360. El modulo anterior exigia E8 (CALL), por eso
rechazaba la entrada con estado 2. Ahora acepta ambos opcodes y conserva el
opcode original al cambiar solo el desplazamiento. Mantiene los controles de
version, unicidad, limites de imagen y proteccion ejecutable. No usa esos RVA
como direcciones fijas: se siguen resolviendo por firmas.

Para actualizar desde la DLL 0.3.1: cierra RE4, ejecuta el desinstalador de tu
carpeta anterior (paquetes 0.3.1/0.3.2/0.3.3 usan el mismo hash), extrae 0.3.4
en una carpeta nueva y ejecuta su instalador. No copies la DLL manualmente.
El mod Minecraft 0.3.0 se conserva. Reinicia RE4, carga una partida y ejecuta
el diagnostico de la carpeta nueva. Debe informar estado 1; con RE4 activo el
contador ticks debe crecer. La prueba real sigue pendiente hasta observarlo.

## Correccion del diagnostico 0.3.2

Con DLL cargada e IPC estado 2, ejecuta `PROBE-RE4-NATIVE.bat` como
administrador con partida cargada. El informe lee las firmas de version,
entrada y PadRead, sus punteros/llamadas y protecciones de las secciones.
No modifica memoria ni instala un hook. Esto distingue firmas ausentes o
ambiguas, llamadas incompatibles y secciones que abarcan varias regiones
VirtualQuery (el comprobador nativo actual exige una sola).
El paquete pequeno `re4bridge-native-probe-0.3.3.zip` se extrae en la carpeta
del paquete existente. Conserva la DLL, mods y configuraciones.

Corrige la conversion UIntPtr en Windows PowerShell y enumera los modulos
mediante Toolhelp con soporte WOW64; el aviso anterior DINPUT8 NO CARGADA
no era fiable. Si el script anterior llego al error UIntPtr, OpenFileMapping
ya habia abierto el canal, aunque no consiguio leer su estado.

Si ya instalaste 0.3.1, conserva la DLL y el mod. Extrae el ZIP pequeno
`re4bridge-diagnostic-0.3.2.zip` dentro de la carpeta de tu paquete, aceptando
reemplazar solo `DIAGNOSE-RE4-CONTROLS.bat` y `.ps1`. Ejecuta el diagnostico
con RE4 abierto y una partida cargada. El paquete completo tambien contiene
el script corregido; los binarios de Minecraft y RE4 no cambiaron.

## Diagnostico y actualizacion desde 0.3.0

El paquete 0.3.1 mantiene abierto el identificador de la memoria compartida
durante la vida del proceso y muestra el codigo de error Windows del host.
No se ha confirmado aun la causa del fallo en el PC del usuario.

Con RE4 abierto y una partida cargada, ejecuta `DIAGNOSE-RE4-CONTROLS.bat`
como administrador. Solo lee rutas, hashes, modulos cargados y estado IPC;
no instala, reemplaza ni activa controles. Puede usarse antes de actualizar.
Envia toda su salida si el host no encuentra el modulo.

Para actualizar la DLL: cierra RE4, ejecuta `UNINSTALL-RE4-CONTROLS.bat`
del paquete **0.3.0 anterior**, y luego `INSTALL-RE4-CONTROLS.bat` del nuevo
paquete. Si la desinstalacion rechaza el hash, conserva la DLL y envia el
diagnostico: no es seguro asumir que sea nuestra. El mod Minecraft 0.3.0
no necesita reinstalarse. Abre RE4 de nuevo y ejecuta el nuevo host.

`Ya existe dinput8.dll` al repetir el instalador significa que se conserva
un archivo existente; no demuestra que RE4 haya cargado la DLL.
`DINPUT8 NO CARGADA` o una ruta de sistema en el diagnostico indican que
la DLL local no esta proporcionando este puente. Una ruta local con IPC
ausente requiere investigar su inicializacion. Estado IPC 2 indica que
la comprobacion del ejecutable/firmas no se acepto; 3, fallo del hook.

Prueba experimental de movimiento nativo. Con Minecraft activo, sus teclas de
movimiento se transmiten a RE4: avanzar/retroceder y girar. El nuevo modulo
procesa esa entrada dentro del frame del juego; no cambia coordenadas de RE4
por teletransporte. Todavia no hay camara libre Minecraft en RE4, geometria
compartida, bloques con colision nativa ni dano cruzado.

## Instalar

1. Cierra Minecraft y RE4. Extrae todo el ZIP fuera de la carpeta del juego.
2. En `%APPDATA%\.minecraft\mods` retira el JAR anterior y coloca
   `re4bridge-0.3.5.jar` de la carpeta `mods` del ZIP. Solo un JAR de RE4 Bridge.
   Conserva Fabric API y Minecraft/Fabric 1.21.1.
3. Ejecuta `INSTALL-RE4-CONTROLS.bat` y lee su resultado. Antes de instalar,
   copia y verifica por SHA256 tus partidas de:
   `D:\Games\Resident Evil 4 - Ultimate HD Edition\Bin32\Profile\Player\Saves`.
   Copia tambien los INI de Bin32. Las copias quedan en
   `%LOCALAPPDATA%\RE4Bridge\Backups\<fecha-identificador>`; anota la carpeta.
4. Debe indicar `Instalado` y `Copia de seguridad verificada`. Solo agrega
   `D:\Games\Resident Evil 4 - Ultimate HD Edition\Bin32\dinput8.dll`.
   No reemplaza `bio4.exe`, guardados ni configuraciones. Si ya hay un
   `dinput8.dll`, el instalador se detiene: comunica ese error y conserva el archivo.
5. Abre RE4 en modo ventana y carga una partida en un sitio donde puedas probar
   movimiento. Abre Minecraft y tu mundo local de prueba. Cierra hosts anteriores
   y ejecuta `START-RE4-CONTROLS.bat` con los permisos que funcionaron para el lector.

Los BAT del instalador ejecutan solo los PS1 incluidos con ExecutionPolicy
Bypass para ese proceso. No cambian la politica global de PowerShell, registro,
antivirus o permisos del sistema. Revisa los scripts si quieres antes de abrirlos.
Si las rutas cambiaron, usa los parametros `-GameBin` y `-SavesDir` del PS1.

## Primera prueba corta

Mantene visibles ambas ventanas. Deja el foco en Minecraft y ejecuta:

```text
/re4bridge controller
/re4bridge status
/re4bridge arm
```

`controller` exige el host nativo; no se conecta al simulador ni al lector simple.
Empieza desarmado. `status` muestra contador de ticks del modulo. Tras `arm`,
cierra el chat y mantene la ventana Minecraft activa:

- W / tecla configurada de avanzar en Minecraft: avanzar en RE4.
- S / retroceder: retroceder en RE4.
- A / izquierda: girar a izquierda en RE4.
- D / derecha: girar a derecha en RE4.

Son controles de giro del RE4 clasico; A/D no son desplazamiento lateral.
El raton todavia no dirige su camara. Minecraft tambien mantiene su movimiento
normal local; esta prueba no alinea aun ambos personajes como una unica entidad.
No combines `follow` con control: activar uno desactiva el otro.

Prueba W durante un segundo y sueltala. Mira si el personaje de RE4 avanza y se
detiene. Prueba A/D durante un segundo. La consola debe mostrar `control` con
`armed: True` y `hook_ticks` creciente mientras controla. Abrir el chat para
consultar `status` desarma el control; por eso puede mostrar nativo desarmado.

## Parar y volver a controlar

F8 produce una parada de emergencia enclavada. `/re4bridge stop` tambien
desarma. Abrir chat, inventario/menu, pausar Minecraft, perder su foco, salir del
mundo o perder la muestra RE4 desarma. Despues repite `/re4bridge arm` para volver
a activar. No se rearma automaticamente.

La red envia input hasta 20 Hz. Si el cliente deja de muestrear, el worker manda
desarmado tras 200 ms. Si el host se bloquea/desconecta, el modulo deja de aplicar
input remoto tras 500 ms y enclava parada 2. Tras parar, la entrada normal de RE4
vuelve a quedar a cargo del juego. F8 es parada 1.

Si no se mueve, envia el resultado de `status` y el mensaje de la consola. El
contador `hook_ticks` permite distinguir DLL no inicializada de un juego que no
ejecuta frames al perder el foco. No se fuerza a RE4 a continuar en segundo plano
ni se modifica su gestion de ventanas en esta version. Evita minimizar RE4;
la prueba requiere que siga ejecutando sus frames con Minecraft activo.

## Retirar y restaurar

Cierra RE4 y ejecuta `UNINSTALL-RE4-CONTROLS.bat`. Solo elimina la DLL si su hash
coincide con la del paquete; conserva partidas, configuraciones y backups.
La desinstalacion no restaura partidas automaticamente. Para restaurar una copia
que elegiste, con RE4 cerrado, usa en PowerShell (sustituye la ruta de backup):

```powershell
Get-ChildItem -LiteralPath "C:\ruta\backup\Saves" -Force | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination "D:\Games\Resident Evil 4 - Ultimate HD Edition\Bin32\Profile\Player\Saves" -Recurse -Force }
```

## Evidencia y limites

Compilado como DLL PE32/i386, con exports DirectInput y solo imports de DLL del
sistema Windows. Pruebas C ejecutan la politica real de TTL, estados desarmados,
seqlock y mascara. Pruebas Java/Python usan TCP real con IPC sintetico y comprueban
armado, input y corte por falta de nuevas muestras. Instalador/desinstalador
probados con PowerShell en Linux y archivos sinteticos: copia verificada,
rechazo de DLL existente, retirada exclusiva de DLL propia y carpeta saves vacia.
El cargador Windows, el hook PadRead y movimiento/colision de RE4 reales aun no
se probaron en cloud. Esta prueba en tu PC es necesaria antes de ampliar el modulo.

Referencia y diseno nativo en el repositorio: `native/README.md`.
Puedes seguir usando el modo de lectura/seguimiento 0.2.0 sin armar controles;
`START-RE4-BRIDGE.bat` y `/re4bridge re4` conservan ese modo.
