# Registro de desarrollo

## 2026-10-10 — Inicio del puente de diagnóstico

- Solicitud: ambos juegos reales en Windows; Minecraft/Steve en primera persona
  dentro de RE4, bloques con colisión, TNT y armas con daño cruzado. Losas para
  aproximación inicial de terreno irregular; formas más precisas cuando haga falta.
- RE4: Ultimate HD Edition, 1.1.0 según pantalla principal del usuario;
  `D:\Games\Resident Evil 4 - Ultimate HD Edition\Bin32\bio4.exe`.
- Minecraft: el usuario instaló Fabric con Minecraft Java 1.21.1 y usa SKLauncher.
  Su carpeta de instancia es `%APPDATA%\.minecraft`.
- Ruta: dos procesos con puente de estados y posteriormente renderizado integrado.
  Se empieza con el lado Minecraft y un receptor sintético, sin direcciones de
  memoria inventadas ni loaders copiados en RE4.
- Referencias leídas: Universal Modder `mod-any-game`, `mashup-mods`, nota de
  colisión/combate y ejemplos GTA V/RE2. RE2 usa RE Engine; no se traslada su
  REFramework a RE4 clásico. La base `re4_tweaks` usa Win32 y Direct3D9.
- Autoridad inicial: Minecraft mueve al jugador; el simulador solo recibe datos.
  El contrato está en `docs/CONTRACT.md`. No se transmite todavía renderizado ni
  se conoce el cambio de ejes/escala de RE4.
- Versiones Fabric/Yarn/Loom verificadas en metadatos oficiales. Se fijaron en
  Gradle; no se requiere cambiar la versión de Minecraft del usuario.
- La máquina cloud tenía Java 21 runtime pero no `javac`. Se preparó Temurin
  JDK 21 con descarga oficial y SHA-256 verificado, fuera del checkout.
- Se añadieron requisitos de red oficiales Fabric/Mojang al borrador. La primera
  consulta dio 403; una consulta posterior a Fabric Maven respondió 200.
- No se lanzaron juegos en el PC del usuario, no se tocaron sus perfiles ni
  archivos de RE4 y no se publicó nada en GitHub.

Resultados actuales y límites de evidencia: `docs/VALIDATION.md`.

## 2026-10-10 — Lanzador Python en Windows

- El usuario ya puso los JAR y abrió el mundo superplano en SKLauncher, pero el
  lanzador batch falló porque Windows no encontró el comando `py`.
- Se añadió detección de `python` como alternativa y comprobación de Python 3.10+,
  con instrucciones de instalación si ninguno de los dos comandos funciona.
- No se cambiaron los JAR ni el protocolo. La ejecución batch en Windows sigue
  pendiente de confirmar en el equipo del usuario.

## 2026-10-10 — Lector externo de RE4

- El usuario confirmo conexion Minecraft/simulador y cambios de `blocks` al
  colocar/romper; se actualizo la evidencia de validacion manual.
- Solicito crear el primer componente de RE4. Se implemento lector externo
  Python/Win32, permisos de consulta/lectura exclusivamente, sin instalacion en
  el juego. Comprueba ruta, PE32/i386, version 1.1.0, unicidad de firmas y punteros.
- Referencia: nipkownix/re4_tweaks revision
  `92c0208bd09c29c9640e13a2493b8e6bd6edd700`, Game.cpp para version/pPL y
  SDK/cCoord.h para posicion/angulos. No se copio el loader ni se uso una
  direccion absoluta de una instalacion particular.
- Seis pruebas sinteticas del lector y cinco del simulador pasaron. El lector
  no esta conectado al protocolo Minecraft, no controla RE4 y su API Win32 aun
  debe probarse en el PC del usuario. No se afirma integracion nativa completa.
- Paquete independiente `re4bridge-re4-reader-0.1.0-windows.zip`, con lanzador,
  lector e instrucciones; generado por `tools/package_re4_reader.py`.

## 2026-10-10 — Conexion RE4 -> Minecraft 0.2.0

- El usuario resolvio acceso denegado (OpenProcess Windows 5) y confirmo que
  posicion/angulos reales cambian al caminar/girar. Capturas con `state: player`.
- Se implemento host Win32 externo de solo lectura y modo autenticado
  `re4_reader`, diferenciado de `simulator`. Minecraft solicita muestras a 10 Hz,
  valida contenido y descarta estados sin personaje o muestras de mas de 1 s.
- Comandos re4/calibrate/scale/ysign/turnsign/follow. Seguimiento optativo RE4 ->
  Minecraft en espectador por teleports del servidor integrado; referencia local,
  escala provisional 100, sin asumir orientacion absoluta. No escribe memoria
  RE4 ni importa render/geometria/dano. El objetivo final Minecraft -> RE4 sigue
  pendiente; esta etapa valida transporte y coordenadas en el sentido de lectura.
- Se detiene por cambios de dimension/modo, perdida de conexion y saltos mayores
  de 64 bloques por eje. Pausa suspende movimiento y conserva conexion.
- Evidencia cloud: 8 pruebas Java core, 12 Python y 2 geometria Minecraft;
  incluye Java/Python con TCP real y fixture de personaje que desaparece.
- Paquete Windows 0.2.0 conserva Fabric 1.21.1 y API. Se sustituye el JAR anterior;
  no se instala ningun archivo dentro de RE4. Prueba grafica conjunta pendiente.

## 2026-10-10 — Control nativo 0.3.0 (en desarrollo)

- El usuario confirmo seguimiento 0.2.0, con captura `follow: true` y 4555 ACK.
- Ruta elegida: proxy dinput8 x86 que reenvia a la DLL original del sistema y
  modifica un CALL identificado de PadRead en memoria, despues del input nativo.
  Agrega solo bits avanzar/retroceder/girar; no teletransporta RE4 ni escribe su
  posicion. La logica de movimiento/colision sigue perteneciendo a RE4.
- Referencia publica re4_tweaks revision 92c0208bd09c29c9640e13a2493b8e6bd6edd700:
  KeyboardMouseTweaks.cpp (llamada PadRead), Game.cpp (Key_btn_on), SDK/pad.h (bits).
  No se asume API de plugins estable. Se rechaza instalar sobre un dinput8 existente.
- IPC local por memoria compartida con contador/heartbeat, corte de 500 ms y
  parada F8 enclavada. Red sigue loopback autenticada. Instalacion con juego cerrado,
  copia previa de saves/config suministrados y desinstalacion por hash propio.
  Validacion real del modulo en Windows pendiente; no se ejecuta RE4 en cloud.

- Ruta de saves confirmada por el usuario:
  `D:\Games\Resident Evil 4 - Ultimate HD Edition\Bin32\Profile\Player\Saves`.
- Se resolvio el CALL interior exacto de la referencia publica, verificando
  ReadCall/InjectHook en CookiePLMonster/ModUtils; no se envuelve el CALL exterior.
- DLL compilada PE32 x86, cinco exports DirectInput, sin runtime adicional.
  La DLL carga la original por ruta absoluta SystemDirectory para evitar recursion.
- 9 pruebas Java, 15 Python y 2 Minecraft pasaron; suite C con 9 aserciones.
  Pruebas PowerShell con archivos sinteticos verifican backup y no sobreescritura,
  retirada exclusiva del DLL propio y saves vacios. Host autentica antes de input.
- Native hook y movimiento real pendientes de prueba en Windows; no se afirma
  colision comprobada, camara primera persona ni input de raton. W/S/A/D mapean
  movimiento nativo clasico. No se fuerza ejecucion RE4 en segundo plano.

## 0.3.1: diagnostico del modulo nativo

- Se conserva el handle de la memoria compartida durante la vida de RE4.
- El host informa el codigo Windows y PID al fallar OpenFileMapping.
- Diagnostico PowerShell de solo lectura: hashes, ruta de DLL cargada y estado IPC.
- Paquete mantiene el mod 0.3.0. Actualizacion retira primero la DLL con el
  desinstalador del paquete anterior; nunca reemplaza una DLL desconocida.
- Validacion cloud: compilacion PE32, politica C, 15 pruebas Python y fixtures
  de instalador PowerShell. La causa en Windows y movimiento real siguen pendientes.

## 0.3.2: compatibilidad del diagnostico PowerShell

- Constructor UIntPtr explicito y enumeracion Toolhelp de modulos WOW64.
- Valida magic/version de IPC antes de mostrar estado/ticks.
- Pruebas PowerShell compilan el interop real y ejecutan el script con una API
  sintetica: acceso solo lectura, puntero de 40 bytes, DLL cargada y canal ausente.
- La captura del usuario fallo despues de abrir IPC: no prueba DLL ausente.
  Conserva mod 0.3.0 y DLL 0.3.1; se publica ZIP pequeno solo con el diagnostico.

## Auditoria de prerequisitos nativos

- Usuario confirma DLL local y del sistema cargadas, IPC presente estado 2/ticks 0.
- Se agrega lector de solo lectura de firmas, punteros, llamadas y regiones
  VirtualQuery para distinguir la comprobacion rechazada sin reemplazar DLL.
- Tres pruebas sinteticas comprueban lectura sin modificaciones, regiones
  divididas, firmas ausentes/ambiguas y version/llamada no compatibles.
- Aun pendiente la salida del lector en el ejecutable real; no se relajan
  comprobaciones ni se activan hooks basandose solo en pruebas sinteticas.

- El lector del usuario encuentra todas las firmas y valida version/memoria;
  falla exclusivamente wrapper_starts_call. Se amplian los datos de la rama
  con bytes/RVA para distinguir CALL, JMP relativo o prologo de funcion.
  ReadCall de ModUtils lee rel32 sin distinguir E8/E9; no se cambia aun el
  hook sin conocer los bytes de la entrada real. Cuarta prueba del lector
  comprueba salto relativo y que se conserva la memoria sintetica.

## 0.3.4: entrada PadRead con salto E9

- El usuario confirma entrada E9 52 FE 55 00, RVA 0x7509, destino 0x567360.
- Se acepta E8/E9 manteniendo opcode y validacion de destino ejecutable;
  no se usan direcciones fijas ni se omiten controles de firmas/version.
- Regresion C usa bytes/RVA observados y verifica CALL, JMP, desplazamiento
  negativo y rechazo de opcode/destino/rango invalidos.
- Se conserva el mod Minecraft 0.3.0. DLL nueva requiere retirar la anterior
  con su desinstalador y reinstalar/reiniciar RE4. Ejecucion real pendiente.

## 0.3.5: armado al cerrar chat e indicador de control

- Usuario confirma estado nativo 1/ticks crecientes pero no movimiento;
  chat confirma comando arm y consola muestra desarmado, sin causa precisa.
- El cliente espera cerrar el chat antes de activar/contar confirmacion.
- Indicador en actionbar muestra motivos de desarmado, teclas solicitadas,
  armado del host y mascara aplicada realmente por la DLL. No requiere alt-tab.
- Nuevo host devuelve motivo, mascara solicitada y mascara nativa en ACK.
- Se mantiene DLL 0.3.4; actualizacion requiere solo mod 0.3.5 y host nuevo.
- Se mantienen cortes por foco/menu/pausa/muestra antigua y rearme explicito.
  Pruebas sinteticas de la secuencia no sustituyen movimiento real en Windows.

## 0.4.0: raton y camara experimental

- Usuario confirma W/S y A/D sin retraso notable, Minecraft enfocado y RE4
  en ventana. Perder foco desarma como previsto; no es fallo del host.
- Giro relativo acumulado y consumo nativo por secuencia; delta limitado y
  PlayerPtr/ang_A0 resueltos con firmas, sin teleport.
- Camara QuasiFPS opt-in, wrapper fastcall con original conservado. Ojo
  configurable y pitch limitado, sin tocar colision/modelo/guardados.
- IPC v2 de80 bytes, host acepta movimiento v1 y anuncia capacidades.
  Comandos mouse/firstperson/eyeheight y HUD compacto con muestras/FP.
- Actualiza DLL/mod/host, retirando DLL con paquete anterior. Conserva Fabric API.
- Pruebas cloud no confirman giro/camara real. Primero giro sin FP, despues FP.

## 0.4.1: ajuste de altura de primera persona

El usuario confirma giro por raton y Leon/camara acoplados en UHD. La primera
persona de 0.4.0 muestra las piernas pese a FP activo. Se aumenta altura
inicial de 160 a 1600 y rango absoluto de 250 a 2500 en cliente, host y DLL.
Referencia: Trainer.cpp de re4_tweaks 92c0208bd09c29c9640e13a2493b8e6bd6edd700
eleva la matriz de su camara libre 2000 unidades para quedar sobre el personaje.
Esto apoya probar otra escala, pero no confirma la altura de los ojos ni que
nuestro punto local use exactamente el mismo espacio. Se conserva ajuste
manual y signo. No cambia el giro, el hook, el protocolo ni los cortes de entrada.
Regresiones Java/Python/C cubren rango, transporte IPC y direccion a altura
alta. La posicion final de primera persona necesita prueba en Windows.

## 0.4.2: ojo anclado y cuerpo oculto

El usuario confirma que 0.4.1 situa la camara dentro del modelo de Leon y
solicita primera persona sin orbitas ni cuerpo visible. Se reemplaza la matriz
de camara por ejes identidad y traslacion pies + altura; mirada mundial yaw/pitch
normalizada. Se conserva giro nativo W/S/A/D y raton. cUnit be_flag_4 documenta
Trans 0x2 y DrawFootShadow 0x10: se limpian temporalmente solo esos bits del
jugador validado, con restauracion antes de logica nativa y sin escribir objetos
anteriores tras cambio de PlayerPtr. El argumento de matriz puede ser local;
solo se retiene copia si es la matriz estatica CamCtrl/QuasiFPS documentada.
Regresiones C comprueban que girar no desplaza el ojo, pitch vertical y mirada
unitaria, valores no finitos y conservacion de bits Alive/Move/ajenos al restaurar.
La efectividad del dibujo y la transformacion final siguen pendientes de Windows.

## Complemento de vista 0.1.0

VIEW-RE4.bat/ps1 y backend Win32: F9 presenta RE4 borderless/topmost sin activar
la ventana; Minecraft conserva el foco. F8/boton Normal escriben solo parada
IPC validada y restauran estilos/bordes/tamano/posicion/topmost/foco de RE4.
Preparar Minecraft y F9 de vuelta desarman; perder foco o cerrar panel tambien.
No cambia mod/host/DLL ni evita cortes de seguridad. Desktop sintetico y memoria
IPC prueban foco, orden parada/retorno, fallo parcial, proteccion de HWND/pid,
restauracion reintentable y parada v1/v2 sin modificar el frame del host.
Presentacion real WinForms/RE4 pendiente de prueba Windows.
