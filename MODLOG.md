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

## 0.4.3 + vista 0.1.1

Usuario confirma vista y primera persona al moverse, pero deriva lateral en
reposo y arma flotante. Se agrega bypass condicionado del suavizado lr_rate
(Trainer.cpp de re4_tweaks, revision92c0208bd09c29c9640e13a2493b8e6bd6edd700):
CMP EBX+0x1B6 exacto y unico, hook preserva GPR/flags excepto ZF mientras FP
activo. Capacidad FP exige esa firma/hook. Ocultacion de modelos de arma
cPlWep0x34/0x38 con deduplicacion y restauracion con propiedad/vtable comprobados;
no se cambia inventario. Vista oculta panel totalmente al F9; Application.Run
conserva timer al ocultar. F8 ejecuta retorno directamente, sin PerformClick
sobre un boton oculto; F9 vuelve al panel. No se presentan las regresiones y
compilacion como prueba Windows de la nueva camara/render/WinForms.

## Cámara nativa 0.4.4

El usuario confirma que 0.4.3 sigue desviándose en reposo. Su registro muestra
yaw/puntos de mirada estables durante la transición caminar/parar. Se evita
ejecutar hitCheck antes del reemplazo FP, se bloquea roll, se conserva FOV
desde QFPS_OFFSET y se alarga la mirada a 1000 unidades para preservar
precisión float en coordenadas grandes. Sin nuevos hooks ni cambios de IPC,
mod Minecraft o host. ZIP de actualización solo nativa incluye diagnóstico
ampliado de CameraControl.Camera_60 y reversión mediante instaladores propios.
Tres suites C y 27 pruebas Python pasan; eficacia visual pendiente de Windows.

## 0.4.5

Vídeo y registro 0.4.4 muestran mejora pero saltos y controles tanque. El punto
final Ctrl.Camera_60 conserva un desvío ~12° al detenerse. Se fija la cámara
final de Ctrl/Global en una llamada de proyección validada, con matrices/ejes
coherentes y restauración del estado original. La mirada del ratón pasa a ser
independiente del rumbo de locomoción: WASD lateral/diagonal relativo a la vista
solo en FP, mediante avance nativo orientado, sin modificar coordenadas ni
desactivar colisiones. Paquete completo con JAR/host 0.4.3, DLL 0.4.5 y todos
los START/view/instaladores/trace; conserva compatibilidad IPC. Tres suites C,
29 pruebas Python y fixtures PowerShell pasan. Windows queda por verificar.

## 0.4.6

Usuario bloqueado en FP por projection_matches=7; demás firmas requeridas
únicas. Sustituye la unicidad de sitio de llamada por unicidad de destino en
todas las llamadas E8 encontradas (máximo32), manteniendo rechazo si difieren
o alguna es inválida. Instala todas las llamadas solo tras validarlas. Nuevo
PROBE muestra destinos/bytes de las siete coincidencias para investigar si
todavía no se admite FP. Mod/host conservan0.4.3; paquete completo. Tres suites
C,30 pruebas Python y fixtures PowerShell pasan; activación Windows pendiente.

## 0.4.7

Diagnóstico confirma dos destinos de proyección: seis llamadas resuelven la
función de proyección documentada y una es otra función. Perfil estricto para
el layout UHD1.1.0 observado valida siete sitios, thunks E9 y destinos antes
de seleccionar las seis llamadas correctas. No selecciona por mayoría ni
habilita layouts desconocidos. Mod/host0.4.3 sin cambios. Tres suites C y31
pruebas Python pasan; activación FP0.4.7 en Windows pendiente.

## 0.4.8

FP0.4.7 se activa y mejora según usuario; cambios rápidos A/D/W/S y mezclas
todavía alteran vista. Engancha entrada E9 de proyección del perfil validado
en lugar de solo seis CALLs, conservando cuerpo original separado. ControlFrame
reintenta lectura seqlock y reutiliza únicamente última muestra completa aún
vigente, sin reiniciar ancla/giro por un frame roto ni extender timestamp.
Live stop/desarmado y TTL500ms conservan prioridad. Mod/host0.4.3 sin cambios.
Tres suites C,32 pruebas Python y fixtures PowerShell pasan. Estabilidad real
0.4.8 pendiente de Windows; no se modificaron instaladores ni movimiento.

## 0.4.9 — diagnóstico de proyección

Usuario no nota mejora con 0.4.8; vídeo gpt.mp4 revisado y216 muestras del
trace020114. En reposo sin ratón, Global aún se desvía hasta~12°; al caminar
se alinea. No se atribuye todo al movimiento inicial del ratón. Añade mapping
separado64bytes con build/PID realmente cargados, contadores de entrada,
escritura/rechazo y yaw antes/escrito/después de original_projection dentro
de la misma llamada. TRACE solo lectura y secuencia completa. Sin cambio de
algoritmo de cámara/locomoción; no se promete solución visual. Mod/host0.4.3.
Tres suites C y36 pruebas Python pasan; requiere captura Windows para decidir
si el hook no cubre la vista o si otra fase sobrescribe el ajuste.

## 0.4.10 — correr, cámara pendiente

Trace022618:223 muestras, build409 cargado, ratón acumulado0 en pruebas;
projection.calls2853492→3203928 y writes0. El ajuste final no llegó a aplicarse;
condición de rechazo pendiente. Usuario permite avanzar: cámara jugable.
Añade tecla de sprint Minecraft configurada (Ctrl por defecto, mantener) como
KEY_RUN0x40 del SDK/pad.h público fijado. Máscara0x4F rechaza otros botones;
capacidad4 evita solicitar correr en DLL anterior. FP conserva correr al
traducir direcciones relativas, cancelación de opuestos elimina correr sin
dirección. Mod/host/DLL0.4.10 requieren actualización conjunta. Sin cambios
al algoritmo de cámara, velocidad/posiciones, partidas o inventario.
TRACE muestra transcurrido/restante en consola, sin overlay nuevo.
15 pruebas core,2 Minecraft,37 Python y tres suites C pasan en Linux.

## 0.4.11 — primer bloque visual

Usuario confirma sprint0.4.10 y autoriza avanzar a bloque visible fijo. Selecciona
bloque colocado mediante /re4bridge blocktest: caja fija inicialmente a3 tamaños
delante de Leon, tamaño1000 ajustable, sin escala Minecraft↔RE4 aún validada.
Romper el bloque seleccionado/cambiar mundo/dimensión/desconectar retira preview.
No copia terreno superplano ni mobs. Dibuja aristas verdes sobre imagen RE4,
sin profundidad/texturas/colisión; no se presenta como objeto físico nativo.
IPC separado68bytes y preview opcional sobre TCP autenticado. TTL/generación,
stop/F8 y armado completo requeridos. Detector D3D9 compara tabla exacta de sonda
oficial y exige dispositivo único de ventana del proceso; vtable privada cambia
solo EndScene. Contadores visibles en status. Cámara/controles0.4.10 conservados.
17 pruebas core,2 Minecraft,41 Python y4 suites C pasan; callback EndScene real
se ejercita con COM simulado para verificar restauración incluso al fallar.
Detección del dispositivo y visualización efectiva requieren Windows del usuario.

## 0.4.12 — dispositivo gráfico y retorno de foco

Captura del usuario0.4.11: estado3, frames0, sin dibujo; parada nativa1 y
error al activar ventana. Detector deja de exigir identidad de tabla completa
y mismo hilo de ventana: valida métodos de consulta de sonda y pertenencia
ejecutable de los119 métodos a Direct3D9 oficial. Recorre regiones separadas
y prefijos de estructuras referenciadas por datos RE4, con límite y deduplicación.
Solo dispositivo único de ventana visible del proceso; no wrappers externos.
IPC92bytesv2 con contadores de descubrimiento impresos por host, compatible
con mod0.4.11. VIEW restaura/paraliza aunque Windows rechace dar foco, y pide
activación manual sin excepción.43 pruebas Python,4 suites C y fixtures VIEW
pasan; detección/visualización real permanece pendiente de Windows.

## Diagnóstico gráfico tras0.4.12

Registro del usuario:33.360 slots,1.050 raíces,5 intentos, cero tablas/devices
y frames/dibujos0; permanece así también con armado/cámara activos.
El fallo está antes del dibujo; no es una selección desarmada ni prueba de
incompatibilidad de primera persona. No se conoce aún filtro/ubicación exactos.
Se publica herramienta externa de solo lectura sin DLL/mod/instalador nuevos:
módulos gráficos, firma de fábrica D3D9 y posibles tablas/métodos con módulos
y RVAs, búsqueda indirecta hasta4096bytes limitada en tiempo/bytes/raíces.
No llama métodos COM ni escribe en el proceso; candidatos no autorizan hooks.
Cinco pruebas sintéticas pasan, incluidos límites y limpieza/reporte ante fallo.
Se requiere graphics-probe JSON del equipo para continuar la reparación.

## 0.4.13 — tabla privada y referencia a0xC30

Informe081059: bibliotecas oficiales D3D9/NVIDIA; candidata con tabla privada y
119 métodos D3D9 referenciada a0xC30. Es pista, no confirmación de interfaz activa.
Detector anterior la excluía por ubicación de tabla/prefijo256bytes. Se permite
tabla legible privada conservando coincidencia de tres métodos de consulta y
pertenencia ejecutable de los119 métodos a D3D9 oficial. Ventana visible propia,
fábrica devuelta por GetDirect3D igual a la localizada por firma única de RE4;
referencias devueltas liberadas incluso con fábrica distinta. Prefijo4096bytes,
raíces únicas limitadas, ningún offset absoluto del informe. Fábrica debe seguir
vigente al enganchar. IPC visual100bytesv3 añade probe_valid/factory_rejected,
host admitev1/v2/v3; mod0.4.11 conservado.48 Python y4 suites C pasan; render
Windows pendiente. No se sustituye cámara ni se añade colisión.
## 0.4.14 — recuperación, detector gráfico retirado

El usuario informa tres cierres antes del menú principal después de instalar
0.4.13 correctamente. No se atribuye el fallo a Minecraft ni a la primera persona.
La búsqueda ejecutaba métodos sobre objetos COM candidatos y sustituía su tabla;
validar direcciones no demuestra vida del objeto ni tamaño completo de su tabla.
Causa exacta pendiente del evento de Windows. Se excluye block_render.h de la
compilación y se impide su inclusión accidental con #error: sin sondeo, IPC visual,
llamadas COM ni hooks gráficos. Host avisa que blocktest queda desactivado.
Se preservan controles, cámara, correr y F8; mod0.4.11/API sin cambios.
Colector de eventos1000/1001 anteriores de bio4.exe, sin acceder al proceso ni
provocar otro cierre. Compilación PE32, 48 pruebas Python,4 suites C y4 fixtures
de eventos pasan. Instalador/desinstalador probados con hash propio y copias.
0.4.13 retirada, sin sobrescribir artefactos antiguos.

## Confirmación del usuario de la recuperación 0.4.14

RE4 abre y carga una partida sin problemas tras instalar0.4.14. El colector
re4-crash-20261011-054530.json devuelve events: [] para los últimos siete días;
sin módulo, excepción ni offset para atribuir la causa exacta de0.4.13. Se
documenta éxito de arranque/carga y se conserva0.4.14 como base, con renderer
desactivado. No se republica ZIP ni se requiere reinstalación para esta evidencia.

## 0.4.15 — captura de la creación real, dibujo desactivado

Se parte de0.4.14, cuyo arranque/carga confirma el usuario. Se intercepta solo
la importación PE32 única Direct3DCreate9 si apunta a la exportación oficial de
D3D9 en el directorio de sistema (SysWOW64/Windows32). Cambio CAS con protección
restaurada/rollback; DllMain no llama COM, carga DLL, reserva heap ni crea IPC.
Fábrica recién devuelta envuelta en fachada externa tipada17métodos, identidad
IUnknown/IDirect3D9 y referencias balanceadas; allocationfail devuelve original.
CreateDevice delegado conserva parámetros/HRESULT/salida y registra números,
sin retener/tocar dispositivo ni cambiar su vtable. Interfaces desconocidas
se delegan y se cuentan; GetDirect3D del dispositivo devuelve fábrica original,
por lo que igualdad con fachada/cobertura necesita Windows. Sin EndScene, sonda
auxiliar, barrido de heap ni blocktest. Control/cámara/correr no se modifican.
IPC independiente96bytesv1, lector read-only y CAPTURE-RE4-GRAPHICS para probar
solo con RE4, sin Minecraft/host. Mod0.4.11/API conservados.55 Python y4 suites C
pasan;3 fixtures de captura reales con APIs simuladas pasan ASan/UBSan. Instalador
verifica copias/hash y desinstalador solo retira DLL propia. Arranque/captura real
pendientes de Windows;0.4.14 se conserva como recuperación sin sobrescribir ZIP.

## Diagnóstico tras rechazo de captura0.4.15

El usuario informa incompatible/modificada (estado3), creaciones correctas0.
El mensaje no discrimina tabla de nombres, exportación, ruta, destino, ambigüedad
ni condiciones tempranas del cargador. No se relajan filtros ni se cambian DLL.
Se publica AUDIT-RE4-IMPORTS como ZIP autónomo solo lectura: cabeceras/imports de
bio4 y exportación Direct3DCreate9 del D3D9 cargado, rutas e IPC existente. Acotado
2MiB/20.000 lecturas/10s, sin heaps/COM/escrituras. Identifica OFT ausente incluso
con IAT resuelta, destinos diferentes, ordinales/duplicados y lecturas incompletas.
Estado actual válido con rechazo temprano no se atribuye sin prueba a tiempo de
carga.5 pruebas sintéticas pasan; archivo autónomo6ficheros sin DLL/mod/instalador,
extracción/importación comprobadas y Universal Modder sin fallos/avisos.
Se requiere imports-audit JSON del equipo para resolver el rechazo concreto.

## 0.4.16 — resultado inicial y reintento único de captura

graphics-capture-20261011-135432.json confirma estado3 y cero llamadas en0.4.15.
imports-audit-20261011-135454.json encuentra importación única por nombre en
RVA0x1310CE0 con valor0x68EB4B20, idéntico a la exportación de D3D9 oficial en
SysWOW64; auditoría completa604lecturas/2361bytes. El estado actual es válido,
pero no reconstruye el rechazo inicial. No se atribuye a un motivo sin prueba.

Se conservan motivo/estado/WinError/RVA/valores/cabecera de cada intento y se
permite un único reintento antes del primer DirectInput8Create delegado. Solo
si el rechazo fue importación ausente/incompatible; nunca listo/fallo de protección.
Mismos filtros PE32/exportación/ruta, CAS y rollback. Sin búsquedas de dispositivos,
recreación ni renderer. IPC168bytes/v2/build416, lector read-only compatible con
v1/0.4.15. Paquete completo incluye auditor y los mismos JAR0.4.11/API. Algoritmos
de control/cámara/correr permanecen iguales.61 Python/4 C y3 fixtures ASan/UBSan
pasan; instalación/respaldo/hash propio verificados. Arranque y creación real de
0.4.16 pendientes de Windows;0.4.14 conserva su papel de recuperación confirmada.

## 0.4.17 — rutas oficiales bajo WOW64

El informe graphics-capture-20261011-141909.json identifica motivo7 tanto en
DllMain como en DirectInput8Create, build416, dos intentos y cero llamadas.
La comparación previa exigía SysWOW64 siempre que su API respondiese y excluía
System32, aunque GetModuleFileName conserva la ruta usada al cargar, incluso
con redirección WOW64. Se aceptan únicamente ambas rutas exactas derivadas de
las APIs de Windows; se mantienen exportación/importación únicas, CAS/protección
y rechazo de proxies/locales/subdirectorios. No se confirma que esa diferencia
sea la ruta concreta del usuario hasta obtener el nuevo registro.

IPCv3/build417/3296bytes registra rutas loaded/wow64/system y matched de cada
intento. Lector read-only compatible con v1/v2; algoritmos de controles/cámara
y fachada COM sin cambios, renderer desactivado.62 Python/4 C y3 fixtures de
captura ASan/UBSan pasan; instalador y hash propio verificados. Paquete completo,
mods/API idénticos, se permite reutilizar carpeta tras desinstalar con el paquete
anterior y retirar sus archivos. Prueba real de arranque/captura pendiente.

## Confirmación de captura0.4.17 en Windows

graphics-capture-20261011-142924.json: hook_status1, fase DLL_PROCESS_ATTACH,
attempts1, factory_calls/success2, device_calls/success1, HRESULT S_OK,
wrap/device failures0 y escaped_interfaces0. Ruta cargada SYSTEM32 coincide
sin distinguir mayúsculas con SystemDirectory; SysWOW64 es la otra ruta derivada.
Esto confirma la diferencia de formato excluida por0.4.16 y el éxito de la
corrección, sin recurrir al reintento. Parámetros de creación: ventana1366×768,
adapter0, type1, flags0x44. Sin dibujo, colisiones ni vida actual del dispositivo
garantizada por estas direcciones históricas. Se documenta la evidencia sin
alterar ni republicar los ZIP0.4.17; no se pide otra instalación para este resultado.

## 0.4.18 — prueba explícita de cubo gráfico

Se parte de la captura0.4.17 confirmada. CreateDevice correcto entrega una fachada
IDirect3DDevice9 externa119métodos: forwarding tipado, referencias balanceadas,
objeto/vtable nativos intactos, fallback original al fallar asignación/IPC.
Bloqueo por dispositivo serializa escena, llamadas delegadas y Reset. Interfaces
desconocidas se delegan y suspenden dibujo; recursos siguen devolviendo interfaces
nativas cuya identidad/cobertura difiere. No se reactiva block_render.h.

Cubo de100unidades de aristas verdes, anclado600unidades delante de la cámara
al activar, proyección por CAMERA.v_mat/FOV validada y clipping. Dibujo antes
de EndScene original, BeginScene válido, backbuffer y permiso fresco. Backup
D3DSBT_ALL restaurado aun ante setter/draw fallido, referencias temporales
liberadas, cero recursos GPU persistentes. Fallo de restauración bloquea hasta
reiniciar; pérdida/Reset requieren otra activación. Sin textura/depth/colisión
ni integración de construcción Minecraft. Controles/cámara/correr no cambian.

TEST-RE4-CUBE: cinco segundos de preparación,30de prueba, F8 detiene, caducidad
500ms y mutex de escritor único; publica únicamente CommandIPC32bytes. Telemetría
92bytes/readonly/version1/build418 y CaptureIPCv3 compatible. Informes cube-test
incluyen contadores inicial/final y parada; CAPTURE no activa dibujo.
70 Python/4 C y6 fixtures ASan/UBSan pasan;23fallos de setter y restore/lifetime
comprobados con APIs simuladas. Instalador/respaldo/hash propio verificados.
Mods/API conservados. Arranque/cubo visible/compatibilidad real pendientes de
Windows;0.4.17 se conserva como recuperación y sus ZIP no se sobrescriben.

### 0.4.18 — cubo visible confirmado en Windows

El usuario confirmó el cubo visible en RE4 y aportó captura e informe
`cube-test-20261011-151401.json`. En 30,04 segundos hubo 1.766 dibujos adicionales,
cero fallos de dibujo/restauración y ningún incremento de fallos de Present o
Reset. Los 138 fallos de Present y 3 Reset eran anteriores al inicio de esta
prueba. Pendientes la precisión del anclaje al moverse, recuperación del dispositivo
y estabilidad prolongada. Colocación/eliminación desde Minecraft y colisiones
siguen sin implementar. Se conserva el paquete publicado sin reconstruirlo.

Nueva prueba de recorrido: el usuario confirmó que el cubo se quedó en su sitio,
con vídeo `testeadocubo.mp4` e informe `cube-test-20261011-153629.json`.
1.164 dibujos adicionales en 30,02 segundos, sin fallos nuevos de dibujo,
restauración, Reset o Present. Anclaje observado en ese recorrido; causa del
desplazamiento extraño previo sin determinar. Sin cambios de código o paquete.

## 0.4.19 — un bloque colocado desde Minecraft

Mod/JAR 0.4.19: `blocks on` observa intentos de BlockItem y confirma cambio de
estado durante diez ticks. La última colocación crea una caja fija delante de
Leon; romper/sustituir ese bloque o cambiar dimensión la retira. Solo una caja,
sin correspondencia de cuadrícula, profundidad, texturas, colisiones o daño.
blocktest manual usa tamaño inicial 100; estado/capacidad de dibujo actualizados.

Host/control policy conecta NativePreview al mismo mutex/canal de cubo validado
en 0.4.18. CubeCommand v2/56 bytes lleva modo 0 diagnóstico o 1 bounds explícitos;
telemetría v1/92 y CaptureIPC v3 conservados con build 419. Límite/finito, TTL y F8,
fallback de fábrica, referencias y restauración gráfica se conservan. Fallo de
dibujo/pérdida/Reset durante activación desarma, nueva arm requerida; fallo de
restauración exige reinicio. El host no abre el canal BlockPreview retirado.

72 pruebas Python, 17 bridge-core, 5 Minecraft y 4 suites C pasan. Seis fixtures
nativos ASan/UBSan pasan con APIs simuladas; intercambio TCP real con comandos
en mappings Windows simulados. DLL/JAR compilados, respaldo/desinstalación propios
verificados. La nueva colocación/retirada visual requiere prueba real en Windows.
0.4.18 queda como recuperación y sus ZIP se conservan intactos.

## 0.4.20 — escala provisional y detección observable

Usuario confirma blocktest100 diminuto cerca del suelo; blocks on no visible.
No se demuestra una causa única de la ausencia automática. Default100→1000
(es ajustable, aún sin calibración física), un solo bloque de aristas.
START_CLIENT_TICK observa uso antes de vanilla además del callback; cola16,
confirmación10ticks, deduplicación sin reiniciar plazo ni anclaje. Se observa
aun en primer tick de arm y se sigue publicando solo armado. HUD/estado muestran
selección, confirmaciones y orden al host; log incluye bounds y pases omitidos.

73 pruebas Python, 17 bridge-core, 10 Minecraft pasan; JAR0.4.20 compilado.
DLL SHA256 80ea2448268e72492bf1421a7f232b44031e5cfd927ba14e08ebfc8565a0e885
idéntica a0.4.19, build419/ABI conservados; no reinstalación nativa requerida.
Causa/visibilidad automática y escala siguen pendientes de prueba Windows.
