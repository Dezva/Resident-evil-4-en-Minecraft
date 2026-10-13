# 0.4.41: vida compartida, Esc en movimiento y F5

## Actualizar

Cierra RE4, Minecraft, host y VIEW. Sustituye el JAR anterior por
**re4bridge-0.4.41.jar**, actualiza los archivos del paquete y conserva Fabric API.
Esta vez hay una **DLL nueva, build441**: usa UNINSTALL-RE4-CONTROLS e
INSTALL-RE4-CONTROLS del paquete, como en las actualizaciones anteriores.
No mezcles la DLL439 con el mod/host nuevos. Preparar comprueba el canal de vida.

Abre las dos partidas, START-RE4-CONTROLS y VIEW. Elige Supervivencia y pulsa
Preparar Minecraft; cuando termine, F9. Después **F5 alterna creativo y
supervivencia sin otra preparación**, conservando inventario, comida, vida,
referencia y construcciones. Fuera del puente, F5 conserva su función normal de
cambiar perspectiva. Mantener F5 no repite el cambio; requiere otra pulsación.
E abre inventario; Esc abre/cierra el menú de Minecraft; F8 vuelve a RE4 normal.

## Cómo funciona la vida

El hambre, la saturación y la regeneración siguen siendo las de Minecraft.
La vida de ambos juegos está vinculada por porcentaje mientras controlas RE4.
Al activar el puente, Leon adopta el porcentaje actual de corazones de Minecraft.
Por ejemplo, perder un20% de la vida máxima de Leon quita4puntos de Minecraft:
**dos corazones de una barra de diez**. La armadura de RE4 y su dificultad ya
forman parte del daño que calcula RE4; el daño transferido no recibe una segunda
reducción por la armadura de Minecraft. Daño originado en Minecraft conserva sus
propias reglas. La armadura de Minecraft no reduce golpes originados en RE4.

La regeneración por comida de Minecraft aumenta la vida de Leon. Una hierba u
otra curación de RE4 aumenta los corazones según el porcentaje recuperado.
Los golpes acumulados se confirman después de aplicar su efecto en el servidor
local: una pausa de Minecraft o varios eventos de red no los pierden ni repiten.

Si los corazones llegan a cero, se escriben cero HP en Leon y su reflejo de HP,
y se solicita su rutina nativa de muerte cuando estaba en Move/Damage.
Una muerte originada en RE4 también se conserva para matar al jugador Minecraft.
No se revive automáticamente un actor RE4 muerto. Tras morir, vuelve con F8,
recarga RE4, reaparece en Minecraft y prepara otra vez. Se necesita una nueva
sesión armada para conectar el actor recargado. Una muerte ya procesada no se
reaplica al Steve que reaparece; F8 termina el enlace.

Creativo conserva la vida/comida de Minecraft para el siguiente cambio a
supervivencia. Durante creativo el enlace repone los golpes normales no letales
sobre los HP de Leon y descarta ese daño en Minecraft. No se añaden parches para
anular ejecuciones/cinemáticas mortales de RE4; esas situaciones requieren prueba
específica. Cambiar a supervivencia vuelve a usar los corazones conservados.

Esc pausa el mundo local de Minecraft y pone movimiento0, pero **RE4 sigue su
simulación**. Si recibe golpes durante el menú, se acumulan para reflejarlos al
continuar. Este paquete no añade una tecla para saltar cinemáticas.

## Prueba breve

1. En supervivencia, deja que un ganado dé un golpe normal. Los corazones deben
   bajar y Leon quedar con el mismo porcentaje. Repite varios golpes separados.
2. Con hambre y vida incompleta, come y espera la regeneración: también debe
   aumentar la vida de Leon. La dificultad de Minecraft afecta regeneración y
   hambre; Pacífico no sirve para comprobar consumo normal de comida.
3. En una prueba donde no importe morir, comprueba la última pérdida de
   corazones y la muerte de ambos. La animación/rutina real necesita validarse
   en tu ejecutable; aquí se verificaron sus escrituras y límites.
4. Mantén W/sprint, pulsa Esc, espera y cierra con Esc/Continuar. Debe conservar
   primera persona, conexión y generación, y seguir permitiendo construir.
5. F5 → creativo → F5 → supervivencia. Comprueba la interfaz, consumo de bloques
   y que la referencia/inventario permanezcan. F8 debe seguir devolviendo RE4.

Si hay un fallo de vida, /re4bridge status muestra «Vida compartida», HP nativo,
estado y motivo; el host añade health_state con contadores y generación.
Si se corta el puente, conserva las líneas alrededor del cambio de stop/reason
además de view-events.log.

## Implementación y verificación

La pausa envía movimiento0 desde setScreen antes de que vanilla abra la GUI.
El worker admite hasta1segundo únicamente para esa instantánea de menú con
movimiento0; el movimiento normal caduca a200ms y la DLL mantiene su lease500ms.
La cámara admite el bit de menú y las colisiones no dependen del pase gráfico
mientras ese menú explícito está abierto. Un cierre de menú tolera el desfase
breve del flag isPaused. Foco, F8, TCP perdido y paradas nativas siguen parando
sin recuperación automática de foco ni de armado.

F5 se intercepta en Keyboard.onKey solo para la ventana Minecraft enfocada y el
puente activo, sin menú. Cambia GameMode en el executor del servidor local,
con los mismos UUID/dimensión/conexión/generación y espera confirmación al cliente.

IPC de vida separado de100bytes, v1/build441, dos seqlocks. La DLL actúa en el
callback pad existente y valida identidad, memoria, rutina global MainLoop,
Leon/LeonAshley, lease, generación y F8 antes de escribir. Solo HP actuales y
entrada de muerte documentados por SDK: GLOBAL_WK4FB4/4FB6, cEm.hp324,
cModel.r_noFC..FF y cEm::Routine0::Die3. HP máximos y flags de depuración quedan
bajo control de RE4. Referencia pública:
[nipkownix/re4_tweaks@92c0208](https://github.com/nipkownix/re4_tweaks/tree/92c0208bd09c29c9640e13a2493b8e6bd6edd700/dllmain/SDK).

Pruebas de daño/curación acumulados, ACK repetido/tardío, wrap, cambios de modo,
muerte, identidad, datos inválidos, seqlock, F8, lease y carreras de punteros.
Fixture TCP autentica controles/vida y reproduce movimiento+sprint → pausa
sin nuevos ticks durante350ms → movimiento con misma generación. Build Fabric
JDK21 y DLL PE32. Gameplay, animación de muerte, foco y GPU reales requieren
prueba en Windows; no se han ejecutado RE4 ni Minecraft gráficos en cloud.

Resultado final: Core42/Minecraft59, Python144 y fixtures VIEW pasan.
HealthLedger, health_tick y collision_tick pasan ASan/UBSan.

El desinstalador nuevo reconoce también el hash publicado de DLL439 (39/40),
para facilitar esta actualización sin aceptar DLL de otros mods. Se probó
retirada439 y copia/retirada441 con partidas sintéticas y backups verificados.
