# Ver RE4 mientras Minecraft controla: complemento de vista 0.1.1

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
3. Pulsa **Preparar Minecraft**. En Minecraft ejecuta `/re4bridge controller`, espera conexion, configura
   las opciones que quieras probar y despues `/re4bridge arm`. Si quieres raton,
   usa `/re4bridge mouse` antes de arm. Si quieres camara experimental,
   `/re4bridge firstperson` antes de arm. Cierra el chat.
4. Pulsa **F9 desde Minecraft**. RE4 se muestra sin bordes a pantalla completa
   en el monitor donde estaba, pero Minecraft conserva el foco y recibe W/S,
   A/D y raton. Su ventana queda detras de RE4, sin minimizarla. El panel se oculta por
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

Cuando se agreguen bloques/entidades sincronizados, el retorno debera retirar
tambien esos objetos temporales. Todavia no existen en RE4; este boton detiene
las funciones actuales. No promete revertir contenido futuro no implementado.

## Validacion

El backend Win32 real se compila con PowerShell. Un escritorio sintetico prueba
foco conservado al presentar, orden parada/restauracion/foco, rechazo si no
estas en Minecraft, proteccion de HWND cerrado/reutilizado y limpieza tras
fallos. Memoria sintetica comprueba IPC v1/v2: cambia solo stop (offset 24),
preserva secuencia/inputs y rechaza identificador/version/estado desconocidos.
La interfaz WinForms y el render de los juegos requieren prueba en Windows.
