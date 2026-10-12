# Ver RE4 mientras Minecraft controla: complemento de vista 0.1.2

Actualizado con0.4.31 para colocar/romper bloques sin perder foco.
Compatible con los paquetes de controles ya publicados (IPC v1/v2). No necesita
cambiar mod, host ni DLL. Windows PowerShell 5.1 y WinForms vienen con Windows;
no requiere instalar otra biblioteca. Usa los mismos permisos que RE4/host.

## Primera prueba

1. RE4 en **modo ventana**, partida cargada. Minecraft abierto con el mod y el
   host de controles funcionando. No minimices Minecraft ni uses fullscreen
   exclusivo en ninguno de los juegos.
2. Extrae el complemento y ejecuta `VIEW-RE4.bat`, como administrador si RE4
   tambien lo esta. Aparece un panel con **Preparar Minecraft** y
   **Volver a RE4 normal (F8)**. Solo admite una ventana de cada juego.
3. Pulsa **Preparar Minecraft** primero: detiene el puente y ajusta RE4 sin bordes
   en el fondo, antes de activar controles/HUD. En Minecraft ejecuta `/re4bridge controller`, espera conexion, configura
   las opciones que quieras probar y despues `/re4bridge arm`. Si quieres raton,
   usa `/re4bridge mouse` antes de arm. Si quieres camara experimental,
   `/re4bridge firstperson` antes de arm. Cierra el chat.
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
indicadores. Usa otro `/re4bridge arm`, cierra chat y F9 para volver a RE4.
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
