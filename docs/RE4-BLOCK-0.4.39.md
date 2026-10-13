# 0.4.39: colisiones fuera de pantalla y preparación de VIEW

El registro de38 identifica la parada: stop1 y collision_state.reason4,
sin fallo de dibujo, después de36 pases omitidos sin nuevos dibujos. El host
sigue conectado. Una escena válida fuera del campo de visión no producía
vértices; si un pase anterior había desactivado la métrica, solo un dibujo
posterior la recuperaba. La referencia de colisiones envejecía mientras tanto.
Al volver a ver los bloques, la protección de intervalo500ms detenía el control.

## Instalar y probar

1. Cierra juegos, host y VIEW. Desinstala la DLL anterior e instala la nueva
   con los BAT de este paquete. Sustituye el mod por **re4bridge-0.4.39.jar**.
   Usa DLL, mod, host y VIEW del mismo paquete; conserva Fabric API.
2. Abre una partida RE4 y un mundo local creativo de Minecraft, en ventana y
   sin minimizar. Abre START-RE4-CONTROLS y VIEW.
3. Pulsa **Preparar Minecraft**. Si Windows no permite darle foco, VIEW indica
   que pulses la ventana de Minecraft: haz un clic y la preparación continúa
   automáticamente, sin volver a pulsar el botón ni escribir comandos.
4. Espera la confirmación y pulsa **F9**. Coloca una pared pequeña nueva,
   apártala del campo de visión varios segundos caminando/girando y vuelve a
   mirarla. Repite con sprint y acércate a ella. La referencia de colisión debe
   mantenerse aunque la pared no tenga vértices visibles en el fotograma.
5. F8 vuelve a RE4 normal y también cancela una preparación pendiente de foco.

Puedes probar en ventana sin F9, con Minecraft en foco y ambos juegos visibles.
No hace falta borrar bloques antiguos para esta prueba. Los bloques de sesiones
anteriores que solo están en Minecraft se conservan: aún no se reimportan a RE4.

## Cambios

- Una escena validada, aunque quede fuera de pantalla, vuelve a marcarse activa
  para las colisiones. Esto no inventa un dibujo ni incrementa su contador.
  Se conservan los checks de cámara, ventana/dispositivo, geometría, arm/TTL,
  fallos gráficos y restauración. La protección de intervalo500ms sigue activa
  para interrupciones reales; no se resuelve relajándola otra vez.
- VIEW conserva la disposición y la parada cuando Windows rechaza el foco.
  Espera activación manual de la ventana exacta de Minecraft antes de iniciar
  la preparación autenticada. No repite cambios de tamaño ni intenta robar foco
  a otras aplicaciones durante la espera. F8 cancela y restaura las ventanas.
- Minecraft PlayerEntity.tick restablece noClip antes de mover al jugador.
  El seguidor lo había ajustado antes del tick, así que los bloques locales
  podían volver a empujar a Steve o disparar correcciones de movimiento del
  servidor integrado. El nuevo hook lo reaplica después del reset solo al
  jugador exacto del seguidor, mientras su referencia está vigente. F8,
  desconexión, cambio de jugador o caducidad lo desactivan. Los demás jugadores
  y Minecraft normal conservan su física. Las colisiones de construcciones
  sincronizadas siguen resolviéndose en RE4.

El fallo de noClip se verificó en el bytecode de Minecraft1.21.1; el log de la
parada confirma la protección nativa4, no demuestra que un bloque antiguo la
causara. Borrar las construcciones no sustituye la corrección de la referencia.

## Verificación y límites

Core36, Minecraft51 y Python139 pruebas.16 fixtures C con ASan/UBSan.
Integración de renderer y callback de colisiones reales:200 fotogramas sin
vértices visibles, pases transitorios, sprint contra pared, giro de vuelta y
cámara inválida rechazada; no hay parada, penetración ni escrituras en Y.
VIEW: denegación de foco no envía comandos, un clic manual inicia una sola
petición sin otro resize y F8 cancela la espera. Minecraft: punto de inyección
verificado antes del movimiento y leases por identidad/servidor/cliente con
caducidad, parada y sustitución. Build Fabric/JDK21 y DLL439 PE32.

Presentación, hilo Win32 y juego real siguen requiriendo prueba en Windows.
El registro del puente todavía es por sesión; no se restaura automáticamente
una construcción vieja a partir de todo el mundo Minecraft. Suelo/losas,
flechas y salto conservan los límites indicados en38.

Si vuelve a perderse la primera persona, conserva el tramo del host desde antes
del corte. Indica stop, reason, collision_state y si la consola permanece
abierta. view-events.log identifica pérdidas de foco. El HUD/colisiones del
puente se detienen después de un fallo real; no se rearma automáticamente.
