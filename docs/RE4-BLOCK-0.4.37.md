# 0.4.37: proporción, ojos y colocación coherentes

La captura36 confirma5068 dibujos con profundidad y sin fallos de restauración.
Las fotos muestran bloques demasiado anchos y dificultad para apuntar/apilar.
En36 se usó el focalX de la matriz virtual GX4:3 directamente sobre un viewport
1366×768: ampliaba X alrededor de un33% respecto a Y. Esa matriz no debe usarse
para deformar los píxeles de salida. Ahora el focalX efectivo de GX se adapta al
aspecto de salida y coincide con el focalY en píxeles, recuperando proporciones
cuadradas como el dibujo anterior a35. Se conservan los planos Z nativos y la
profundidad validada; no se altera la matriz de proyección del juego.

## Instalar y probar

1. Cierra juegos, host y VIEW. Desinstala la DLL anterior e instala la nueva con
   los BAT habituales. Sustituye el JAR por **re4bridge-0.4.37.jar**; conserva
   Fabric API. Usa mod, host, VIEW y DLL del mismo paquete.
2. Abre RE4 con partida cargada y Minecraft en mundo local creativo, ambos en
   ventana y sin minimizar. Abre START-RE4-CONTROLS y VIEW.
3. **Preparar Minecraft → esperar confirmación → F9**.
4. Coloca un bloque nuevo cerca del suelo, míralo de frente y coloca otro sobre
   su cara superior. Rodea la pareja y comprueba contorno/cara seleccionada;
   rompe solo el superior. Después prueba tres bloques en fila y una losa.
5. F8 vuelve a RE4 normal. Preparar → F9 permite volver al puente.

Una cara frontal debería verse cuadrada; una vista oblicua conserva su perspectiva.
No utilices una construcción anterior a un reinicio para esta primera prueba:
el registro de bloques del puente y su referencia siguen siendo por sesión.

## Cambios en interacción

- Preparar usa la altura de ojos real de Steve y la escala: normalmente1.62
  bloques de pie, y la altura del pose al agacharse. Pies, cámara y rayos usan
  el mismo origen. `eyeheight` manual sigue disponible, y Preparar restaura auto.
  El rango nativo es40..2500unidades; escalas que no permitan esa altura se
  rechazan al armar. La escala habitual es1000unidades/bloque.
- El rayo utiliza la cabeza mapeada desde RE4 y la orientación del tick de
  control. Se evita mezclar posiciones/ángulos interpolados de Minecraft con
  la cámara de RE4. La selección se calcula antes de enviar el HUD de ese tick.
- Altura de cuadrícula, malla, bloques, contorno, colocación y flechas utilizan
  el mismo desplazamiento. Un ajuste de altura no puede separar el contorno
  del volumen renderizado.
- Contacto de apoyo de hasta0.02bloques, limitado al20% de la altura de una forma
  pequeña. Una discrepancia de milímetros con el suelo no fuerza a elevar la
  propuesta un voxel entero. Los laterales y la parte superior se conservan:
  muros y objetos que intersecten el volumen siguen impidiendo colocar.
- Sigue la validación del hit del paquete corregida en36 y el alcance de Minecraft.
  Se seleccionan las formas reales de los bloques construidos para apilar y romper.

El mapa nativo todavía no se transforma en una malla de losas de Minecraft.
Pendientes grandes pueden requerir una celda más alta; no se promete ajuste
continuo de todo el terreno. Oclusión de transparencias y objetos sin depth
write también conserva límites. Arcos visibles en RE4 y daño nativo pendientes.

## Verificación

Pruebas: cara frontal cuadrada con GX4:3 en salida1366×768, clipping y profundidad,
ojos/pies con distintas poses/escalas/alturas, todos los vértices de dos bloques
apilados frente al contorno con rotaciones, rayo desde ojos de Steve a cara
superior, contacto pequeño de suelo aceptado y muro rechazado. Build Fabric,
DLL PE32, suites Python y fixtures C con ASan/UBSan. La interacción visual y
fluidez real se deben comprobar con los juegos en Windows.

Si todavía se separan bloque y contorno, captura la escena y usa
CAPTURE-RE4-GRAPHICS después de intentar colocar; el informe no abre memoria
del proceso y conserva matrices/contadores. Indica si falló apuntar al suelo,
la cara superior de un bloque ya puesto o ambas.
