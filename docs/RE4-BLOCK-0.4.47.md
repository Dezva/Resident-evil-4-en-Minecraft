# 0.4.47: interacción medida, detector de ganados y continuidad de partículas

Esta versión corrige fallos que la prueba en Windows confirmó en0.4.46.
La captura de entrada y los administradores proceden del diagnóstico de lectura
aportado por el usuario; las pruebas de regresión reproducen esos datos.

## Actualizar desde0.4.46

1. Cierra Minecraft, RE4, el host y VIEW.
2. Extrae el paquete completo en una carpeta nueva.
3. En los mods de tu instancia, sustituye el JAR anterior por
   `re4bridge-0.4.47.jar`; conserva Fabric API1.21.1.
4. Ejecuta `UNINSTALL-RE4-CONTROLS.bat` y después
   `INSTALL-RE4-CONTROLS.bat` del paquete nuevo. El desinstalador reconoce la
   DLL446 por su checksum. El instalador respalda y verifica las partidas.
5. Abre ambos juegos y el host nuevo, prepara Minecraft y pulsa F9 en VIEW.
   Mod, host y DLL447 deben actualizarse juntos.

## Correcciones

- **R para interactuar:** la E real produjo `Key_btn_on/trg=0x80400`, es decir,
  `KEY_CK|KEY_A`. La señal anterior omitía `KEY_A` e inyectaba otros bits que
  no aparecían en esa captura. R ahora usa la combinación observada, con un
  disparo inicial y una pulsación limitada a120ms. Funciona con el foco en
  Minecraft y el menú cerrado; E conserva su función de inventario en Minecraft.
- **Detector de ganados:** había19 coincidencias de firma para tres gestores.
  El diagnóstico encontró un pool compatible con cEm:60 entradas de3592bytes,
  con11 actores de tipo ganado0x12. Los otros gestores usan468 y988bytes,
  menores que la estructura cEm del SDK (1032bytes). La DLL reúne los destinos
  de la firma conocida y selecciona el único pool compatible y legible. No
  utiliza direcciones fijas ni el primer resultado; si hay dos candidatos
  compatibles, rechaza la ambigüedad. Reintenta después de inicializarse el juego.
- **Colisión de Leon:** también separa al jugador cuando un bloque ya contiene
  su posición al iniciar la referencia o mientras está quieto. El barrido de
  movimiento por sí solo no resolvía ese caso. La separación busca una salida
  del conjunto de bloques conectados, evitando alternar entre cajas vecinas.
  Ganados y Leon usan esa resolución horizontal; no se escriben Y, velocidad,
  salud, animación ni rutinas de los enemigos.
- **Partículas:** mover o retirar fragmentos visuales ya no cambia la época de
  la escena. Los paquetes duplicados a60Hz tampoco reinician la velocidad de
  partículas que avanzan a20Hz. Se conservan los tiempos de cambio de geometría
  y se anima en cada fotograma de RE4, con predicción limitada a50ms. El sufijo
  visual no modifica el prefijo de colisionadores ni prolonga las leases.

## Comprobar en tu partida

1. Ante un aviso como `E TALK`, pulsa R con el foco en Minecraft y sin chat abierto.
2. Coloca una pared de varios bloques entre Leon y un ganado. Comprueba que ambos
   se detienen. `/re4bridge status` debe mostrar el detector disponible, actores
   revisados y contactos/correcciones al tocar la pared.
3. Rompe un bloque y observa si los fragmentos avanzan entre ticks, sin saltos.

La compilación y los tests en Linux no sustituyen estas comprobaciones en RE4.
Los golpes desde abajo con elevación virtual siguen pendientes; esta versión
no cambia la Y del actor ni desactiva el daño.
