# 0.4.21 — cuadrícula visual de varios bloques

En 0.4.20 confirmaste en Windows que colocar y romper la piedra añade y retira
el cubo. Esta versión permite conservar **hasta 64 bloques, con un máximo total
de 128 cajas**. La primera colocación fija la referencia delante de Leon. Los
siguientes mantienen entre sí las posiciones, alturas y orientación de Minecraft.
Romper uno retira solo ese bloque, incluso si era el que fijó la referencia.

Se extraen las formas de Minecraft: una losa inferior ocupa la mitad inferior;
una superior, la superior; una doble, el bloque completo. Las escaleras y vallas
pueden tener varias cajas. Solo se transfieren bloques que colocas durante esta
sesión, nunca el suelo completo del superplano, estructuras ni mobs.

**Continúan siendo aristas verdes sin superficies, textura ni colisión física
dentro de RE4.** Se usan las formas de colisión de Minecraft como geometría
visual; eso no las instala en el motor físico de RE4. El relieve de RE4 aún no
se importa a Minecraft y la altura inicial se aproxima con la posición de Leon.
El vídeo de referencia guía el objetivo de interfaz, objetos y combate, pero no
establece cómo lo implementó su autor ni acredita equivalencia de este prototipo.

## Actualizar desde 0.4.20

Esta vez **sí cambia la DLL**: el canal nuevo contiene la escena completa.

1. Conserva tu paquete 0.4.20 como recuperación. Cierra Minecraft, RE4, host,
   VIEW y cualquier TEST-RE4-CUBE.
2. Ejecuta **UNINSTALL-RE4-CONTROLS del paquete 0.4.20**. La DLL de ese paquete
   es la misma de 0.4.19, por lo que ambos desinstaladores reconocen la instalada.
3. Extrae 0.4.21 en una carpeta vacía. También puedes retirar los archivos
   anteriores y reutilizarla, después de desinstalar y conservar la recuperación.
   Ejecuta **INSTALL-RE4-CONTROLS del paquete 0.4.21**. El instalador crea y
   verifica una copia de tus partidas antes de instalar, como las versiones anteriores.
   No reemplaces DLL manualmente ni uses el instalador nuevo para desinstalar
   la DLL anterior: los hashes son distintos.
4. En `%APPDATA%\.minecraft\mods`, retira el JAR anterior de RE4 Bridge y copia
   **re4bridge-0.4.21.jar**. Deja un solo JAR del puente. Conserva Fabric API
   0.116.6+1.21.1, Fabric Loader 0.16.10 y Minecraft 1.21.1.

## Prueba de pared y losas

1. Abre RE4 en ventana y carga una zona abierta. Abre tu mundo local de Minecraft
   en ventana, modo creativo. Mantén el foco de Minecraft, sin minimizarlo.
   VIEW es opcional. Para comparar, comienza sin mouse/firstperson.
2. Inicia **START-RE4-CONTROLS del paquete 0.4.21**, con los mismos permisos
   que RE4; cierra otros hosts/pruebas que ocupen el escritor del canal.
3. En Minecraft usa **/re4bridge controller**, espera conexión, después
   **/re4bridge blocks on** y **/re4bridge arm**. Cierra el chat y espera `activo`.
4. Coloca una piedra para fijar la referencia. Coloca otras dos pegadas a sus
   lados y una encima. Deberías ver la disposición completa en RE4. El tamaño
   inicial sigue siendo 1000 unidades RE4 por bloque, todavía provisional.
5. Rompe la piedra inicial: deben permanecer las otras, sin desplazarse. Camina
   o gira para comprobar que el conjunto sigue fijo en ese lugar.
6. Coloca una losa inferior al lado y después una superior; comprueba que las
   mitades corresponden. Completa una losa doble para ver su altura completa.
7. Pulsa **F8**: control y dibujo se detienen. Otra `arm` vuelve a mostrar la
   escena conservada. `blocks off` la retira y borra su referencia.

Envía un vídeo corto que muestre la pared, la retirada del primer bloque y las
losas. Si algo falla, captura el indicador antes de abrir el chat y copia
`/re4bridge status`: incluye bloques, cajas enviadas, solicitud y dibujos.

Si la cuadrícula queda debajo del terreno, prueba **/re4bridge blocks height 250**
y después **/re4bridge arm**; `height -250` la baja y `height 0` restablece la base.
Se mueve todo el conjunto, conservando sus distancias. Este ajuste visual no
detecta pendientes ni mejora colisiones. El rango del ajuste es -2500..2500.

`blocks on 800` inicia otra cuadrícula con escala 800; `blocks on` borra la
anterior y empieza una referencia nueva. Haz esto al cambiar de zona en RE4.
Cambiar dimensión en Minecraft, desconectar o reiniciar borra la escena. Los
chunks sin cargar conservan las cajas hasta poder comprobar sus bloques otra
vez; romper/sustituir un bloque cargado lo retira. No se guardan escenas en disco.

El límite rechaza nuevas colocaciones sin borrar las anteriores. Si un cambio
de vecinos eleva la complejidad total por encima de 128 cajas, se retira el dibujo
y aparece un aviso. Plantas sin forma de colisión no se representan; objetos de
varias celdas, como puertas, todavía necesitan sincronización adicional. La
prueba manual `blocktest` permanece como diagnóstico de un solo cubo y desactiva
la captura automática para evitar mezclar ambas referencias.

## Validación y recuperación

Pasan **79 pruebas Python, 19 bridge-core, 15 Minecraft y 4 suites C portátiles**.
Seis fixtures nativos pasan ASan/UBSan. Se verifican TCP autenticado, escena
completa, rotación rígida, eliminación individual, formas y límites, snapshot
sin publicaciones partidas, F8, caducidad de 500 ms y retirada al desconectar.
El dibujo agrupa hasta 128 cajas en un solo DrawPrimitiveUP y restaura los
estados originales; fallos y referencias COM mantienen su cobertura anterior.
DLL PE32 y JAR compilados. Los instaladores/verificación de partidas pasan con
APIs simuladas. El arranque, dibujo y rendimiento de esta versión nueva requieren
tu prueba en Windows: la nube no ejecuta RE4 ni un driver D3D9 real.

Para volver a 0.4.20: cierra ambos juegos y host, desinstala con el paquete
**0.4.21**, instala con el paquete **0.4.20** conservado y vuelve a poner su JAR
0.4.20. F8 sirve para volver al control normal sin desinstalar.

Una vez confirmadas disposición y escala, el siguiente avance visual serán
superficies de los bloques. La física requerirá además obtener el terreno y
conectar estas formas con las colisiones de RE4, antes de TNT o daño cruzado.
