# 0.4.44: selección por caras, salto y rotura de bloques

Esta entrega prepara la construcción en creativo y supervivencia: mira más
precisa al girar, salto del controlador, apoyo sobre bloques, grietas de minería
y fragmentos al romper. El fallo de colisiones con ganados reportado por el
usuario sigue pendiente; el combate Minecraft contra ellos se abordará después.

## Actualizar

Cierra ambos juegos, host y VIEW. Sustituye los archivos del paquete y el JAR
por **re4bridge-0.4.44.jar**, dejando un solo JAR de RE4 Bridge. Conserva Fabric API.
Usa **UNINSTALL-RE4-CONTROLS → INSTALL-RE4-CONTROLS** del paquete nuevo:
incluye **DLL444 nueva** y un canal Runtime actualizado. El desinstalador reconoce
las DLL publicadas de 0.4.43/42/41/39/40 y conserva DLL ajenas.

Abre las partidas y START-RE4-CONTROLS, después VIEW: Preparar Minecraft →
confirmación → F9. También sirve con ambos juegos en ventana, sin VIEW.

| Tecla | Función |
|---|---|
| Espacio, o tu tecla de salto de Minecraft | Saltar con el controlador |
| Clic derecho | Colocar en la cara seleccionada |
| Clic izquierdo | Romper; mantenerlo para minar en supervivencia |
| E / Esc | Inventario / menú de Minecraft |
| F5 | Alternar creativo y supervivencia |
| R | Acción contextual de RE4 |
| F8 | Terminar el puente y volver a RE4 normal |

## Colocación y salto

La selección recalcula el rayo con la mirada actual, también justo antes de
procesar los clics. El control refresca mirada entre fotogramas sin extender la
vigencia del movimiento. Mirar arriba/abajo llega a90° en ambos juegos: antes
RE4 se limitaba a85°, lo que desplazaba la cara apuntada al mirar al suelo.
Los bloques conocidos siguen siendo seleccionables durante una demora del
mapa; la colocación continúa validándose contra la geometría nativa disponible.
Se rechaza colocar una forma dentro del cuerpo del jugador, incluso con noclip
temporal del seguidor.

El salto usa impulso/gravedad de Minecraft a20ticks/s, altura aproximada1.25
bloques, apoyo en las formas colocadas y contacto con techos. Mirando hacia
abajo, salta y coloca cuando tus pies superen la parte superior del bloque
nuevo. Al caer quedas apoyado encima: permite repetir para construir una columna.
Mantener salto vuelve a saltar cuando aterrizas, como Minecraft.

El suelo temporal de cristal permanece a la altura del terreno de RE4 mientras
saltas; no sube para ocupar la celda donde intentas construir. Al salir de una
construcción, o romper su apoyo, la gravedad te devuelve al suelo.
En supervivencia, saltar consume extenuación, las caídas de más de3bloques causan
daño Minecraft y minar en el aire conserva su penalización. Creativo conserva
sus reglas de objetos/daño. El menú congela la física vertical del controlador.

Este salto mueve **Steve y la vista de RE4** y usa pies virtuales para colisiones
de construcción. La Y del cuerpo nativo de Leon sigue bajo control de RE4:
todavía no tiene animación nativa de salto ni permite saltar sus barreras del
terreno. El vuelo libre de creativo queda pendiente. Estos límites se deben
tener en cuenta antes de conectar el combate de enemigos al salto.

## Rotura

La minería de supervivencia muestra los diez estados de grietas sobre la
textura del bloque. Al soltar el botón, cambiar de objetivo o terminar de romper,
se restaura el material normal. Herramientas, dureza, tiempo, inventario y drops
siguen siendo los de Minecraft.

Cada rotura confirmada por el controlador Minecraft genera fragmentos con la
textura/tinte del bloque; la observación del mundo conserva el efecto si llega
por actualización del servidor. Se evita duplicarlo. Se dibujan en el mundo de
RE4 con el mismo pase de profundidad que los bloques, también al romper el
último bloque de una construcción. Hasta24fragmentos activos, vida0.8s; las
formas visuales se excluyen del barrido físico. El límite global de128formas y
64texturas continúa aplicándose. Los objetos sueltos de Minecraft aún no tienen
representación propia en RE4.

## Prueba breve

1. Gira y apunta a distintas caras de un bloque; coloca a sus lados y encima.
2. Mira hacia abajo, salta y coloca debajo. Repite hasta construir una columna,
   comprueba que aterrizas sobre ella y sal de ella para caer.
3. En supervivencia, mina piedra: deben aparecer grietas y desaparecer al
   soltar o cambiar de bloque. Comprueba herramienta, tiempo e inventario.
4. Rompe varios bloques y el último: deben aparecer fragmentos sin impedir
   caminar. Comprueba también uno detrás de un objeto del escenario.
5. Durante un salto abre Esc y continúa. Comprueba F5 y F8 como antes.

Si hay un fallo, conserva el estado del host y una captura/video del objetivo y
de los pies de Steve; no hace falta borrar tus construcciones para actualizar.
La escala habitual de1000unidades por bloque es la referencia de esta prueba.

## Verificación

Build Fabric/JDK21 y DLL PE32, pruebas de rayo vertical/caras, física de salto,
columna de seis bloques, techos, pausa, consumo/caída y fragmentos acotados sin
colisión. TCP comprueba altura y mirada fresca sin prolongar el lease de input.
Fixtures nativos verifican cámara elevada sin escritura Y, pies virtuales sin
falsa parada por teletransporte, máscara física de formas, seqlock/F8/TTL y
retorno de cinemáticas. No se ejecutó RE4 ni un driver D3D9 real en cloud: la
apariencia final y la colocación necesitan esta prueba en tu Windows.
