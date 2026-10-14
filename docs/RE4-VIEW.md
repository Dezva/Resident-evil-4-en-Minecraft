# Ver RE4 mientras Minecraft controla: complemento de vista 0.1.2

Actualizado con 0.4.35: el botón prepara el mod completo sin comandos.
Requiere mod, host y DLL del paquete nuevo. Windows PowerShell 5.1 y WinForms
vienen con Windows. Usa los mismos permisos que RE4/host.

## Primera prueba

1. RE4 en **modo ventana**, partida cargada. Minecraft abierto con el mod y el
   host de controles funcionando. No minimices Minecraft ni uses fullscreen
   exclusivo en ninguno de los juegos.
2. Extrae el complemento y ejecuta `VIEW-RE4.bat`, como administrador si RE4
   tambien lo esta. Aparece un panel con **Preparar Minecraft** y
   **Volver a RE4 normal (F8)**. Solo admite una ventana de cada juego.
3. Pulsa **Preparar Minecraft**: ajusta las ventanas antes de armar y pide al
   mod que conecte controller, active primera persona/ratón/HUD/construcción y
   arme el puente. Mundo local creativo, chat/inventario cerrados. Espera el
   mensaje de preparación completada. F8 cancela también durante la espera.
4. Pulsa **F9 desde Minecraft**. RE4 se muestra sin bordes a pantalla completa
   en el monitor donde estaba, pero Minecraft conserva el foco y recibe W/S,
   A/D y raton. Los clics reales izquierdo/derecho/medio y la rueda que caen sobre
   RE4 se dirigen a Minecraft. La vista no vuelve a redimensionar RE4 con arm
   activo; evita provocar un Reset grafico de HUD/bloques al entrar. Su ventana queda detras de RE4, sin minimizarla. El panel se oculta por
   completo; F8/F9 siguen funcionando mediante el timer del helper.
5. **F8** detiene el puente, restaura bordes/tamano/posicion de RE4 y le da el
   foco para jugar directamente. El boton hace lo mismo. Si el raton esta
   capturado, usa F8; para pulsar el boton, Esc libera el cursor y ya desarma.

F9 de nuevo muestra el panel, devuelve la vista a Minecraft y desarma, para ver comandos e
indicadores. El panel vuelve a preparar el mod; espera confirmación y pulsa F9 para volver a RE4.
Alt-tab/perder foco desarma y restaura la ventana; no se rearma solo. Cerrar
el panel normalmente tambien detiene y restaura. Si Windows rechaza dar el
foco, pulsa la ventana de RE4; el panel informa el fallo.

## Que significa volver a normalidad

El boton/F8 escribe solo la parada persistente de la DLL, tras comprobar
identificador/version/estado del IPC; no publica inputs ni modifica la memoria
del proceso. La DLL deja de sustituir teclas/camara. Desde 0.4.2 restaura tambien
los bits de dibujo del cuerpo; 0.4.3 agrega los modelos de arma. La restauracion del juego sucede cuando RE4
ejecuta su siguiente update; si esta pausado, primero hay que reanudarlo.
El helper conserva/restaura estilos, posicion, tamano y estado topmost de RE4.

No desinstala mods, no vuelve atras una partida y no deshace acciones que ya
ocurrieron en el juego. Tampoco recupera un proceso colgado: en ese caso
cierra host/helper y reinicia RE4. Forzar el cierre del helper desde el
Administrador de tareas puede impedir restaurar sus cambios de ventana;
cerrar/reabrir RE4 recupera su configuracion habitual.

El renderer de bloques/HUD y la barrera existentes dejan de actuar cuando la
DLL observa stop. La plataforma temporal de Minecraft se retira al desarmarse
el cliente. Las construcciones que colocaste en Minecraft no se borran.

## Validacion

El backend Win32 real se compila con PowerShell. Un escritorio sintetico prueba
foco conservado al presentar, orden parada/restauracion/foco, rechazo si no
estas en Minecraft, proteccion de HWND cerrado/reutilizado y limpieza tras
fallos. Memoria sintetica comprueba IPC v1/v2: cambia solo stop (offset 24),
preserva secuencia/inputs y rechaza identificador/version/estado desconocidos.
La interfaz WinForms y el render de los juegos requieren prueba en Windows.

En0.4.12, si Windows rechaza el foco de RE4 al volver, la sesión conserva la
parada y la restauración de ventana, y el panel pide pulsar RE4 manualmente.
No convierte ese rechazo en una excepción de retorno. Errores reales al restaurar
la ventana conservan el estado pendiente para poder reintentar la limpieza.

## Clics en0.1.2

Un observador WH_MOUSE_LL se instala solo durante F9 y consume/reenvia los
botones/rueda reales que caen sobre la ventana RE4 verificada, con Minecraft
como foreground y ambas ventanas pertenecientes a sus PID originales. No envia
teclas, movimiento del raton ni clics periodicos, no lee otros procesos y no
intenta recuperar el foco despues de Alt-tab. Eventos de movimiento se ignoran
antes de consultar ventanas; no duplican raw input ni cargan el helper a cada
muestra de raton. Eventos inyectados/ajenos no se reenvian.

RE4 usa WS_EX_NOACTIVATE durante la vista. El helper conserva modificadores,
rueda y coordenadas, y libera botones reenviados al salir aunque Minecraft haya
capturado el raton. Un fallo de envio detiene la vista; F8, salir, fallo de
preparacion y perdida de foco desmontan el observador. Restaurar la ventana se
intenta incluso si falla la retirada del observador. No cambia DLL/JAR/host.

El fixture compila el backend real y prueba400 eventos de colocar/romper,
botones simultaneos, captura/cambios de destino, modificadores, rueda negativa,
limpieza de pulsaciones, rechazo de entradas ajenas, ventanas/PID reutilizados,
fallos de entrega/preparacion/limpieza y ajuste de tamano antes de arm. Sigue
siendo una simulacion del escritorio: la respuesta real de Windows/GLFW y los
juegos necesita comprobacion en tu equipo.


## 0.4.33 — inventario con E y ventanas alineadas

Preparar Minecraft ajusta ahora también su ventana al monitor, detrás de RE4,
para que interfaz/cursor coincidan con la presentación. F8/cierre/fallo restauran
los estilos/tamaños originales de ambos juegos. Usa E para el inventario vanilla:
W/A/S/D quedan suspendidos mientras está abierto; E/Esc lo cierran conservando
arm. Esc desde juego abre pausa y sí desarma. F8 siempre devuelve RE4 normal.

El observador continúa reenviando solo botones/rueda de la sesión verificada.
Añade movimiento de cursor únicamente si el canal OverlayCommand indica menú
enabled con TTL<500ms; ese movimiento no se consume ni se duplica como raw input
jugando. No hay cambio de foco en abrir/cerrar inventario, ni rearme por alt-tab.
La calidad final también depende de la resolución/GUI scale configuradas en los
juegos. La capa se captura hasta1600×900; resolución1366×768 cabe sin reducción.
Comprobación visual y GLFW/D3D reales pendientes en Windows.

## Diagnóstico0.4.34

Los movimientos del cursor del inventario se agrupan hasta la siguiente
actualización de presentación (timer16ms), conservando el último punto. Antes
de cada clic se vacía esa posición pendiente. Los movimientos raw del modo de
juego no se reenvían. PostMessage fallido, excepción del hook, cambio/cierre
de ventana y pérdida de foco tienen causas separadas. VIEW escribe
`view-events.log` con hora local después de detener/restaurar. Un error de
log no impide detener. No se rearma ni se recupera el foco automáticamente.

## 0.4.38

El observador WH_MOUSE_LL utiliza un hilo propio con cola de mensajes. Una pausa
del timer/interfaz PowerShell ya no detiene su bombeo. Al salir se envía WM_QUIT,
se espera la retirada del hook y solo entonces se liberan botones/mappings.
La posición de menú continúa agrupada y protegida entre los dos hilos.
La pérdida de foco incluye HWND, proceso y clase en `view-events.log`.
Una salida de primera persona puede ser una parada de la DLL antes de hacer
Alt+Tab: revisa también `collision_state.reason` del host. No se recupera foco
ni se rearma automáticamente. Win32/hilo real pendientes de prueba en Windows.

## 0.4.39 — Windows rechaza el foco al preparar

La denegación de foco ya no provoca una excepción/restauración que haga inútil
la instrucción de pulsar Minecraft. Se conserva la disposición preparada con
stop latched y sin iniciar ninguna petición al mod. Tras un clic en la ventana
verificada de Minecraft, el timer inicia preparación una sola vez. F9 espera
la confirmación y F8 cancela tanto la espera de foco como la petición en curso.
No hay cambio automático de foco durante esa espera ni pérdida de restauración.

## 0.4.40 — selector de modo y Esc

El panel añade Mantener modo del mundo / Supervivencia / Creativo. La petición
de preparación autentica ese valor fijo; se rechazan nombres/valores distintos
y se espera confirmación del mod antes de jugar. Una versión antigua del mod
que no confirme el modo solicitado produce un mensaje de actualización y corte.
F9 continúa presentando RE4; Esc/E reciben las interfaces de Minecraft en el
overlay con botones de movimiento0 y el mismo armado. F8 conserva retorno normal.
El menú pausa Minecraft, pero esta versión no congela la simulación de RE4.

## 0.4.41 — Esc en movimiento y F5

Preparar exige host/DLL441 por el nuevo canal de vida. F9 sigue siendo presentación.
Esc publica movimiento0 antes de abrir la GUI y conserva armado al cerrar.
F5 alterna creativo/supervivencia durante control activo sin repetir Preparar;
funciona también con ambos juegos en ventana, sin VIEW. Fuera del puente no
intercepta F5. F8 conserva retorno normal. La vida se vincula por porcentaje
mientras se controla RE4; hambre/regeneración son Minecraft. Menú Minecraft
no congela RE4. Guía y prueba: RE4-BLOCK-0.4.41.md.

## 0.4.42 — escenas y acción R

Preparar exige host/DLL442 por el canal de escenas/acción. R envía la acción
contextual de RE4 desde Minecraft sin mover foco; E conserva inventario. F5/Esc/
F8 mantienen sus funciones anteriores. En escenas de RE4 se cede la cámara y
se suprime el overlay de juego; al acabar se retoma la misma sesión. Los menús
Minecraft pueden seguir usándose, pero no se superponen a la muerte nativa.
No se añade atajo de salto de cinemáticas. Pacífico pasa a Normal al preparar o
entrar en supervivencia, otras dificultades se conservan. Estas funciones también
sirven en ventana sin VIEW. Instrucciones: RE4-BLOCK-0.4.42.md.

## 0.4.43 — Preparar no termina en 0.4.42

El fallo provenía de la clasificación nativa de escenas: CINESCO estaba activo
en una partida normal y anulaba cámara/control, impidiendo confirmar preparación.
DLL443 corrige la máscara y refresca escena aun desarmado. Actualizar paquete
completo y reinstalar DLL; el host rechaza442 con mensaje claro. VIEW conserva
su preparación autenticada y no omite sus checks de foco ni recupera un armado
tras parada. Guía: RE4-BLOCK-0.4.43.md.

## 0.4.44 — construcción y salto

Preparar sigue igual, con paquete/mod/host/DLL444 completos por Runtimev2.
La tecla de salto de Minecraft mueve Steve y la vista de RE4, permite apoyo
en construcciones y colocar debajo; no fuerza una animación nativa de Leon.
E/Esc/F5/R/F8 conservan sus funciones, en VIEW y en ventana. Mira90°, grietas y
fragmentos se añaden en el controlador/renderer, sin nuevos mensajes de ratón
ni cambios de foco de VIEW. Guía: RE4-BLOCK-0.4.44.md.
