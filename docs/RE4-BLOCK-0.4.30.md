# 0.4.30 — vista coherente, suelo temporal del editor y lectura de colisiones del mapa

El usuario confirma una mejora importante de rendimiento con0.4.29. Envía dos
mediciones válidas de4s, sin contactos con bloques ni cambios de zona/armado:

| Medición | RE4 normal | Puente FP |
|---|---:|---:|
| Caminar, recorrido/tiempo total | 1,540 bloques/s | 1,455 bloques/s |
| Correr, recorrido/tiempo total | 3,944 bloques/s | 3,805 bloques/s |
| Correr, solo tiempo en movimiento | 4,158 bloques/s | 4,116 bloques/s |
| Cadencia PadRead al correr | 59,75/s | 59,24/s |

Escala1000 unidades/bloque. La velocidad de carrera durante el movimiento difiere
aproximadamente1%; los comienzos de ambos tramos no tienen la misma demora de
pulsación. Caminar conserva una diferencia de8,3% durante movimiento y menor
cadencia en la captura del puente (52,25 frente a59,74/s). Son muestras cortas,
no un benchmark de GPU ni evidencia de un multiplicador de velocidad distinto.
Se mantiene la locomoción nativa; no se añade un aumento de posición/velocidad.

## Vista y primera persona

La orientación sincronizada se envía como ángulo absoluto en la transformación
fija de la cuadrícula, anunciado por capability8 y look flag4. Ya no depende de
que la primera muestra de ratón llegue antes de empezar a girar ni de recuperar
el yaw del cuerpo al detenerse/rearmar. El modo relativo anterior se conserva
para sync off y DLLs anteriores. La capacidad nueva solo aparece después de
validar todos los requisitos de la cámara. No se añaden firmas/hook de cámara.

Una escena D3D9 mantiene la misma cámara durante su dibujo. Las actualizaciones
que llegan a mitad de escena no deben alterar la vista respecto del mundo ya
dibujado; la siguiente escena puede usar la nueva mirada. La comprobación de
colisión continúa usando los pies actuales. Las proyecciones repetidas mantienen
la reutilización acotada de0.4.29 y su comprobación de F8/TTL/propietario.

El renderer de bloques usa una copia coherente de esa vista final de primera
persona, no el Global mutable que pueden reutilizar pases auxiliares. Exige
jugador/generación/lease válidos, seqlock completo y edad menor100ms. Rechaza una
vista vencida/mezclada; en FP no vuelve a una cámara orbital auxiliar. Sin FP se
conserva el renderer anterior. El orden real del motor y el resultado visual
necesitan comprobarse en Windows; las pruebas reproducen el cambio de ratón
intermedio, la restauración temporal de matrices y su coherencia con bloques.

Se oculta el cockpit nativo en FP usando únicamente DPF_COCKPIT0x10000 del campo
Global.Flags_DISP+0x58. Se guarda y restaura ese bit para el mismo Global, dejando
otros bits y sistemas de UI sin modificación. No altera munición, salud,
inventario ni modelos. F8/desarmado/retorno de cámara restauran el HUD. El bit y
layout proceden del SDK de la referencia fijada abajo; la desaparición visual
también queda pendiente de la prueba Windows. El HUD inicial de Minecraft sigue
siendo mira/hotbar, no incluye salud/combate compartidos.

## Steve deja de usar el suelo superflat como referencia física

El usuario confirma que Steve termina dentro/debajo del suelo: los desniveles
nativos movían su Y, mientras el terreno plano de Minecraft seguía inmóvil.
Al crear una referencia nueva con blocks on/arm, el editor creativo se sitúa en
un volumen16 bloques por encima de la superficie local, o conserva una altura
mayor si ya está volando. No desplaza las construcciones originales de Minecraft.
La referencia permanece fija después de crearse; no se recolocan los bloques
ya enviados al caminar ni al volver a armar.

Mientras está armado se crea una superficie temporal de cristal11×11 bajo la
posición seguida de Steve. Cambia con su altura, ocupa solo aire en chunks ya
cargados y no se envía a RE4 ni cuenta entre las64 colocaciones. Retira el parche
anterior al caminar; no mantiene un camino creciente. Las celdas propiedad del
editor se retiran al detener/desconectar; preserva bloques ajenos y ediciones
reales, incluyendo cristal puesto después de romper un auxiliar. Un cierre
forzado de Minecraft puede dejar ese pequeño parche en el mundo. No escribe
archivos ni genera un mapa fuera de los chunks que el jugador ya carga.

Este parche es un apoyo provisional para editar. No copia la geometría de RE4,
no impide colocar dentro de sus estructuras y no proporciona saltos/apoyo encima
de bloques a Leon. El desplazamiento continúa cargando los chunks normales al
seguir al jugador; puedes mantener renderizado/simulación de Minecraft bajos.
Usa una zona pequeña para esta comprobación. El margen vertical tiene límites:
un descenso acumulado mayor que ese margen puede alcanzar otra vez el terreno
original. El mapa real es la solución de esa limitación, no elevar sin fin la
referencia ni congelar a Steve.

## Actualizar y comprobar

**Cambian DLL, host y JAR.** Fabric API/Minecraft/FabricLoader, instaladores y
VIEW se mantienen. Conserva0.4.29 como recuperación.

1. Cierra juegos/host/VIEW/pruebas. Ejecuta UNINSTALL-RE4-CONTROLS del paquete
   anterior; extrae0.4.30 completo y ejecuta INSTALL-RE4-CONTROLS de0.4.30.
2. Sustituye re4bridge-0.4.26.jar por **re4bridge-0.4.30.jar**, dejando una sola
   copia del puente en mods. Mantén Fabric API.
3. RE4 en ventana, partida cargada, Minecraft creativo local y START de0.4.30.
4. controller, espera conexión; blocks on, firstperson para habilitar FP,
   blocks collisions on, arm y cierra el chat. firstperson sigue siendo un
   interruptor: no lo repitas si ya está activado. VIEW/F9 es opcional.
5. Mira el suelo temporal en Minecraft. Coloca una columna de tres piedras
   adyacente a ti. Camina un tramo corto por una pendiente y coloca otra columna.
6. Gira rápido y alterna A/D; comprueba si los bloques y el entorno se mantienen
   coherentes y si puedes seguir apilando. El HUD de salud/balas de RE4 debería
   desaparecer en FP. F8 debe devolverlo y retirar los auxiliares de Minecraft.

DLL SHA256: bd9e3f19eb213ae23e3ebbb930c9c8f97672ca592e1d83d7f12d604b0703042a.
Build430. Control v2/80, escena v6/96352 y HUD v1/32792 conservados. Para recuperar
0.4.29: desinstala con0.4.30, instala con0.4.29, repón su JAR0.4.26 y host.

## Exportar el mapa real — prueba independiente, sin arm

**EXPORT-RE4-MAP.bat** descubre los managers SAT/EAT con sus firmas únicas y lee
triángulos de colisión, sus transformaciones y clasificación suelo/pendiente/
pared. Usa exclusivamente PROCESS_VM_READ y consultas. Valida UHD1.1.0, módulo,
vtable, stride0xC0, contadores/índices, valores finitos, cambios de objeto/manager
y un presupuesto de32MiB y50000 triángulos locales. No llama funciones nativas,
no inyecta input, no activa un canal ni modifica RE4/Minecraft.

1. Abre **solo RE4** y carga una partida en la zona inicial. Host, Minecraft y
   VIEW no son necesarios. Si estabas controlando el puente, pulsa F8 antes.
2. Deja a Leon quieto y ejecuta EXPORT-RE4-MAP (mismos permisos que RE4).
3. Se generan juntos **re4-map-....json**, **.obj** y **.html**. Abre el HTML en tu
   navegador: arrastra para girar y rueda para acercar; amarillo marca a Leon.
   No descarga librerías, no accede a red ni muestra una vista en vivo.
4. Envía el JSON para validar el formato real. El radio por defecto es20 bloques
   nativos provisionales; puedes usar --radius-blocks5..60 para otro tamaño.

Es una captura local de polígonos de colisión, no de texturas ni malla visual.
Los objetos decorativos sin colisión no están representados. Una firma ausente,
objeto que cambia o layout distinto producen un informe incompleto; no se usa
para asumir que un espacio está vacío. **Todavía no importa estos triángulos a
Minecraft, no bloquea colocaciones en estructuras, ni convierte pendientes en
losas.** Esa validación es el paso previo para implementar el mapa compartido,
pruebas de ocupación/rayo y apoyo vertical/salto. Espadas, arcos y daño compartido
también quedan pendientes; esta versión no anuncia esas mecánicas como activas.

## Evidencia y límites

114 pruebas Python,25 bridge-core y30 Minecraft pasan sin fallos/omisiones;
11 fixtures C bajo ASan/UBSan, cuatro suites C portátiles e instaladores
PowerShell pasan. Se conserva la presión contra paredes con pasos tardíos,
TTL/F8 y las10000 proyecciones rápidas sin trabajo repetido. Se prueban mirada
absoluta contra el rayo transformado, vista constante con cambios de input a
mitad de escena, HUD restaurado con bits ajenos, parche acotado sin escrituras
si no cambió ni sobrescritura de construcciones, matrices SAT/EAT/índices/
cambios/límites y exportación completa de artefactos.

Referencia de interoperabilidad: nipkownix/re4_tweaks,
92c0208bd09c29c9640e13a2493b8e6bd6edd700, Game.cpp, SDK/atari.h, SDK/cManager.h,
SDK/global.h y GameFlags.h. Solo se usan firmas/layouts/nombres de bits; el
exportador, renderer del visor y código del puente se implementan aquí. No se
incluyen ejecutables del juego, capturas del usuario ni geometría extraída en
los paquetes. Las APIs Windows/D3D9 son simuladas en las pruebas y la geometría
es sintética; no se ha ejecutado RE4 ni extraído su mapa real en la nube.
