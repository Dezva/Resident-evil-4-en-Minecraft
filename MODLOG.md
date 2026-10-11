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
