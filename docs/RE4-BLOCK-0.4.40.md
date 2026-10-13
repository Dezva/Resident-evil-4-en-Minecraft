# 0.4.40: Esc, supervivencia e interfaz de Minecraft

## Actualizar y empezar

La DLL es la misma build439 de0.4.39, con las colisiones fuera de pantalla que
ya comprobaste. Puedes conservarla instalada. Cierra Minecraft, host y VIEW,
sustituye el JAR anterior por **re4bridge-0.4.40.jar** y actualiza los archivos
del paquete, incluidos VIEW y tools. Conserva Fabric API.

1. Abre una partida RE4 y un mundo local Minecraft, con ambos en ventana y sin
   minimizar. Abre START-RE4-CONTROLS y VIEW.
2. En VIEW elige **Supervivencia**, **Creativo** o **Mantener modo del mundo**.
   Pulsa **Preparar Minecraft** y espera la confirmación del modo elegido.
   Si Windows no concede foco, pulsa Minecraft una vez como en39.
3. **F9** muestra RE4. **E** abre el inventario real; **Esc** abre el menú de
   Minecraft. Esc/Continuar lo cierran y retoman el puente sin otro arm ni F9.
   **F8** devuelve RE4 normal y restaura las capacidades de movimiento prestadas.

La elección de modo cambia el jugador local del servidor integrado. Se conserva
su inventario, vida, comida, dificultad y construcción; no se conceden objetos
ni se rellena la vida/comida. Mantener modo no cambia el modo existente.
Si el mundo estaba en Pacífico, el comportamiento de comida/vida es el de esa
dificultad; puedes cambiarlo desde las opciones de Minecraft.

## Prueba breve

- Esc → espera unos segundos → Continuar/Esc; después camina, coloca y rompe.
  El host debe seguir conectado y no exigir otra preparación.
- Supervivencia: comprueba corazones, comida, armadura si la llevas, barra/nivel
  de experiencia si tienes XP, hotbar y mano. Son los renderizadores y valores
  del Minecraft instalado, con sus recursos y escala de GUI.
- Lleva unas piedras, un pico adecuado y comida en el inventario. Colocar
  consume la pila; romper respeta herramientas/tiempo de minería. Los drops
  se crean en Minecraft y puedes recogerlos al acercar a Steve mediante Leon.
- Corre durante un tramo largo para consumir saturación/comida; come cuando
  tengas hambre. Correr contra una pared sin avanzar no añade gasto de sprint.
- E permite inventario de supervivencia y recetas. Las interfaces de mesas,
  cofres y otros contenedores también suspenden movimiento sin cortar el puente.
- F8 y Preparar con Creativo permiten volver a construcción creativa.

## Qué conserva el puente al abrir menús

Durante inventario, pausa/opciones y pantalla de muerte se siguen enviando
frames de control con botones0 y el mismo armado/generación. La captura GUI
continúa en el render thread; cerrar un menú no requiere rearme. Los botones,
rueda y cursor usan el router ya verificado de VIEW. Pérdida real de foco,
F8, muestra nativa caducada, fallo real o salir del mundo siguen deteniendo
control. Salir al título termina la sesión; no implica retomar sin un mundo.

Esc usa el menú de Minecraft. Ese menú pausa el mundo local de Minecraft y
suspende los inputs del puente; **la simulación de RE4 continúa**. Esta versión
no añade una pausa nativa ni un comando para saltar cinematográficas.

## Supervivencia y límites actuales

Vida y hambre son las de Minecraft, según lo elegido por el usuario. Los
corazones no leen ni escriben HP de Leon. El daño de enemigos de RE4 todavía
sigue las reglas de RE4; no se convierte en daño de Minecraft. Tampoco se
dibujan aún entidades/drops/flechas de Minecraft en RE4. El puente de combate,
salto/caída y restauración de construcciones entre sesiones siguen pendientes.

El seguidor admite creativo y supervivencia. Presta únicamente vuelo/noClip
para que la física de Minecraft no dispute la posición a RE4 y restaura esos
valores al soltar control, sin cambiar invulnerabilidad ni convertir el jugador
en creativo. El reset vanilla del tick se corrige también en supervivencia.
La minería ignora únicamente la penalización aérea causada por ese seguimiento;
herramientas, dureza, efectos y cosecha siguen siendo vanilla. La extenuación de
sprint usa distancia horizontal real×0.1, descartando pasos caducados/saltos de
posición, con vida, comida y regeneración gestionadas por Minecraft.

La pantalla de muerte se puede capturar y usar para reaparecer. La continuidad
del puente depende de que RE4 mantenga personaje válido y el foco en Minecraft.

## Verificación

Core38 y Minecraft57 pruebas; Fabric/JDK21 compilado. Python139 y fixtures
VIEW de autenticación, modo elegido/confirmación, espera de foco y F8. Pausa
simulada10segundos conserva generación; foco/parada siguen sin rearme automático.
HUD vanilla confirma rutas de corazones/comida/armadura/aire/XP; extenuación y
comida usan HungerManager real. Se verifica el único check de penalización de
minería de1.21.1 y restauración de capacidades al cambiar modo/soltar seguimiento.
La DLL build439 y su SHA256 se conservan; sus16 fixtures ASan/UBSan ya pasaron
en39. Interacción/GPU/menús en los juegos reales requiere prueba en Windows.
