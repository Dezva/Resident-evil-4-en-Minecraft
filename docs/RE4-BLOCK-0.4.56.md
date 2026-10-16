# 0.4.56: flechas de arco/ballesta y acciones nativas visibles

Versión de prueba para Windows: DLL456, mod0.4.56 y host0.4.56 deben actualizarse
juntos. Conserva las correcciones de inicio de hostfix2. El usuario confirmó
estabilidad y activación de trampas/bombas en0.4.55; esa es la base funcional.
0.4.56 está compilada/probada automáticamente, pendiente de gameplay real.

## Actualizar desde0.4.55

1. Cierra RE4, Minecraft, host y VIEW. Extrae el ZIP en una carpeta nueva.
2. Ejecuta `UNINSTALL-RE4-CONTROLS.bat` de esa carpeta: reconoce por hash la
   DLL455 publicada. Después `INSTALL-RE4-CONTROLS.bat` instala DLL456 y verifica
   una copia de partidas. Una DLL ajena se rechaza; no se sobrescribe bio4.exe.
3. Sustituye el JAR anterior por `mods/re4bridge-0.4.56.jar`, dejando una sola
   versión y Fabric API. No instales el JAR de fuentes.
4. Usa START-RE4-CONTROLS y VIEW de la carpeta nueva. RE4 y host se abren
   normalmente, sin restaurar RUNASADMIN. Preparar Minecraft, F9 para jugar,
   F8 para parar. Usa exclusivamente una partida offline.

El agente no instala este paquete ni modifica la instalación real del juego.
Para volver a0.4.55, retira primero DLL456 con su desinstalador y restaura
DLL/JAR/host del paquete0.4.55-hostfix2. Los ajustes de administrador son
independientes y no deben restaurarse para cambiar de versión.

## Combate con flechas

Minecraft conserva carga del arco, carga/disparo de ballesta, inventario,
munición, durabilidad, gravedad y arrastre de sus entidades vanilla. Solo se
conectan flechas propias nacidas durante la sesión de control. El servidor
integrado comprueba su segmento de movimiento frente al catálogo nativo, al
terreno conocido y a los bloques colocados. No convierte la mira en un disparo
instantáneo: una flecha que aún no ha alcanzado al Ganado no solicita daño.

Daño: ceil(velocidad float × daño base), más el bono crítico vanilla y Poder
del arma guardada en la propia flecha, convertido a50HP nativos por punto.
Se usa la fórmula de Poder de Minecraft1.21.1. No depende del objeto que tengas
en la mano después de disparar. Munición y desgaste ya se pagan en el disparo
vanilla y no se repiten al recibir la confirmación de RE4.

La DLL conserva identidad GUID/tipo/vtable, hueso/parte y raycast nativo,
barreras colocadas, generación, TTL500ms y F8. Cada flecha puede solicitar un
solo impacto nativo; ni cambiar el ID del comando ni llenar el historial de128
entradas permite reutilizarla. Se retira al confirmar, rechazar o caducar su
impacto, evitando impactos tardíos sobre otro actor. La ventana de daño500ms
por actor continúa aplicándose también entre flechas y golpes.

Los modelos de flecha/spectral se dibujan con el renderer vanilla instalado,
en el overlay existente y con el FOV del puente: máximo16 flechas visibles,
sin otra ventana, framebuffer ni copias de recursos originales del juego.

Límites de esta primera etapa:

- La flecha se pausa al salir de la cobertura de terreno completa y reciente
  (consulta local de12000 unidades nativas, aproximadamente12 bloques a escala1000).
  Puede continuar al volver a existir cobertura. Las pausadas/incrustadas se
  retiran tras1200 ticks retenidos; no se supone que el espacio desconocido esté vacío.
- El overlay no comparte el búfer Z de RE4: puede mostrar una flecha por encima
  de un objeto nativo. La autorización de daño sigue comprobando colisiones.
- Un impacto por flecha: Perforación contra varios Ganados queda pendiente.
  Multidisparo genera flechas vanilla independientes, con la misma ventana de
  daño por actor. No se ha validado todavía en una partida real.
- Fuego, Empuje, efectos de pociones/espectral, encantamientos condicionales
  personalizados y cohetes de ballesta no se trasladan a los actores de RE4.
  Las flechas especiales pueden causar el daño base, pero no esos estados.
- No se agregan tipos de enemigos fuera de la whitelist ya comprobada.

## Patada y modelo de Minecraft

STATUS[2]/STA_PL_EM_ACTION (0x40000000), definido en el SDK fijado, cede el control
a la cámara/animación nativa, restaura el modelo de Leon y oculta mano/HUD del
puente. Al terminar devuelve la primera persona sin exigir otra generación.
STATUS[0]/STA_PL_ACTION por sí solo sigue sin considerarse una escena: un aviso
contextual no basta para ocultar el puente. Los fixtures prueban restauración
y retorno; falta comprobar qué fases exactas marca la patada en el ejecutable real.
El host expone `native_flags2` para distinguirlas sin escribir memoria exploratoria.

Usar tu skin/modelo de Minecraft es una etapa distinta, técnicamente plausible:
leer la pose nativa de los huesos, mapear brazos/piernas/cabeza al modelo cúbico,
transferir la skin local y dibujar respetando profundidad y restauración del
renderer. Cambiar únicamente una textura no adapta las proporciones ni la
animación. Esta entrega conserva a Leon; no incluye todavía esa adaptación.

## Prueba breve en una partida offline

1. En una zona abierta, sitúa un Ganado a3–6 bloques aproximados. Dispara una
   flecha con arco poco tensado y otra completamente tensada. Comprueba que
   vuelan, reaccionan al llegar y consumen una flecha cada una en Supervivencia.
2. Repite con ballesta: carga, suelta el botón y dispara. Cambia de objeto tras
   disparar; el daño debe seguir perteneciendo al proyectil, sin otro desgaste.
3. Coloca una pared de dos bloques entre tú y el Ganado y dispara contra ella.
   El Ganado no debe recibir ese impacto. Una flecha visualmente sobrepuesta
   al escenario no demuestra daño: observa su reacción/estado.
4. Aturde un Ganado como de costumbre y activa la patada contextual con R.
   Deben verse Leon y su patada, y después volver mano/HUD/primera persona.
5. Pulsa F8 con una flecha en vuelo: el control debe parar y no generar daño
   nuevo después de la parada. No guardes la partida para esta comprobación.

No es necesario repetir todos los diagnósticos anteriores. Si falla algún paso,
indica el número; los logs se leen directamente en el equipo.
