# 0.4.19 — colocar y retirar un bloque desde Minecraft

Esta versión conecta la construcción de Minecraft con el dibujo del cubo probado
en RE4 0.4.18. **Coloca un bloque en Minecraft y aparece un cubo de aristas verdes
fijo delante de Leon. Rompe ese mismo bloque y desaparece.** Una nueva colocación
sustituye al cubo anterior; solo se representa un bloque a la vez.

Es una prueba de sincronización. La posición del bloque en Minecraft no define
todavía una posición equivalente en el mapa de RE4: se fija delante del personaje
usando su posición y orientación al intentar colocar. Todavía no hay textura,
superficies sólidas, ocultación por terreno, colisión, TNT ni daño cruzado.

## Instalar

1. Cierra Minecraft, RE4, host, VIEW y cualquier TEST-RE4-CUBE.
2. Ejecuta **UNINSTALL-RE4-CONTROLS del paquete 0.4.18 instalado** antes de
   borrar sus archivos. Conserva ese paquete para volver a la versión anterior.
3. Extrae el paquete 0.4.19. Puedes reutilizar la carpeta tras desinstalar y
   retirar los archivos anteriores. Ejecuta su **INSTALL-RE4-CONTROLS**.
4. En `%APPDATA%\.minecraft\mods`, retira el JAR anterior
   `re4bridge-0.4.11.jar` y copia **re4bridge-0.4.19.jar** del nuevo paquete.
   Debe quedar un solo JAR de RE4 Bridge. Conserva Fabric API 0.116.6+1.21.1,
   Minecraft 1.21.1 y Fabric Loader 0.16.10.

## Primera prueba con los dos juegos

1. Abre RE4 en ventana y carga una partida en una zona abierta. Abre Minecraft
   en ventana, sin minimizarlo, en tu mundo local superplano y **modo creativo**.
   Usa un bloque completo sencillo, por ejemplo piedra. Espectador no permite
   colocar ni romper bloques.
2. Ejecuta **START-RE4-CONTROLS del paquete 0.4.19**, con los mismos permisos
   que RE4. Cierra cualquier otro host o TEST-RE4-CUBE antes: comparten un único
   permiso de escritura. Minecraft debe haber creado `config/re4bridge.json`.
3. En Minecraft ejecuta **/re4bridge controller**, espera la conexión y después
   **/re4bridge blocks on**. El tamaño inicial es 100 unidades de RE4.
4. Ejecuta **/re4bridge arm**, cierra el chat y coloca una piedra en Minecraft.
   Debería aparecer el cubo delante de Leon. Usa las ventanas al lado para esta
   primera prueba; no actives mouse/firstperson todavía.
5. Rompe la piedra que acabas de colocar: el cubo debería desaparecer. Coloca
   otra para comprobar que vuelve a aparecer. Si colocas varias, el cubo solo
   sigue a la última; romper una anterior no lo retira.
6. Pulsa **F8**: se detienen el control y el dibujo. Para volver a probar, regresa
   a Minecraft y usa **/re4bridge arm**.

Abrir el chat, perder el foco o pausar Minecraft desarma el puente y retira
temporalmente el dibujo. Para verlo mientras Minecraft envía inputs, mantén su
foco o usa VIEW/F9 como en las pruebas anteriores; después de Alt+Tab vuelve
a armar. El cubo no se mueve con Leon una vez colocado.

**/re4bridge blocks off** desactiva la captura de nuevas colocaciones y retira
el cubo. **/re4bridge blocktest 100** sigue disponible como diagnóstico manual:
mira un bloque ya colocado, ejecuta el comando y luego arm. **blocktest off**
retira la selección actual, pero no desactiva blocks on.

Si no se ve, vuelve a Minecraft y copia **/re4bridge status**; abrir el chat
desarma el control por diseño. Los contadores de dibujos son acumulados: el
estado 1 solo indica que el canal está listo, no que el cubo esté visible ahora.
Envía también **CAPTURE-RE4-GRAPHICS** del paquete nuevo y un vídeo corto si
la posición parece incorrecta. No necesitas repetir TEST-RE4-CUBE para esta prueba.

## Cómo se retira y qué se ha verificado

El mod observa intentos de colocar BlockItem en el cliente local y espera un
cambio de estado hacia el bloque esperado durante un máximo de diez ticks.
Un clic sin colocación confirmada no crea un cubo. Se sigue la posición del
último bloque: aire, sustitución de estado o cambio de dimensión retiran la
selección. Un rechazo posterior del mundo también la retira. Losas/puertas y
otras formas especiales se representan por ahora como una sola caja; sus
formas exactas aún no se transfieren.

El host requiere una nueva arm tras parada. Su canal CubeCommand v2 de 56 bytes
lleva modo y bounds explícitos; comparte el mutex de escritor con TEST-RE4-CUBE.
Telemetría de 92 bytes se lee sin escribirla. La DLL valida límites/valores
finitos, caducidad de 500 ms y F8, y dibuja mediante el dispositivo recibido de
CreateDevice. Conserva la restauración gráfica de 0.4.18 y excluye el detector
retirado. Un fallo de restauración requiere reiniciar RE4; pérdidas/Reset durante
el dibujo desarman y requieren otra arm tras recuperar el dispositivo.

Pasan 72 pruebas Python, 17 de bridge-core, 5 de Minecraft y 4 suites C portátiles.
Seis fixtures nativos pasan ASan/UBSan con APIs simuladas. Se verificaron el
intercambio autenticado TCP, retirada, F8/rearm, exclusión de otro escritor,
bounds explícitos fijos al mover cámara/personaje y rechazo de bounds inválidos.
La DLL PE32 y el JAR se compilan en la nube. La visibilidad y posición de esta
integración nueva todavía requieren tu prueba en Windows.

Para recuperar 0.4.18: cierra los juegos, desinstala con el paquete 0.4.19,
instala con el 0.4.18 conservado y vuelve a poner su JAR 0.4.11. Los instaladores
conservan las partidas y verifican el hash de la DLL; no sustituyas DLL a mano.
