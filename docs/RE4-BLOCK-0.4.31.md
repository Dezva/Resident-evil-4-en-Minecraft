# 0.4.31 — VIEW para construir y exportación de referencias del mapa

El usuario confirma0.4.30: los bloques ahora se ven/colocan bien y el suelo
provisional del editor funciona. Los iconos del HUD son correctos; se cortan
porque la ventana de RE4 queda parcialmente fuera de pantalla. VIEW pierde foco
al colocar/romper. Se corrige el helper, manteniendo los componentes confirmados.

## Actualizar sin reinstalar controles

**DLL, mod, Fabric API y host de controles son idénticos a0.4.30.** El paquete
completo conserva re4bridge-0.4.30.jar y DLL/build430. No hace falta ejecutar
UNINSTALL/INSTALL ni cambiar mods. Cambian VIEW-RE4.ps1, tools/RE4View.cs y
el exportador del mapa; VIEW-RE4.bat conserva su lanzador STA habitual.

1. Si tienes el VIEW antiguo abierto, pulsa F8 y ciérralo, para restaurar su
   ventana y liberar el panel único. Puedes conservar los juegos y el host.
2. Extrae este paquete y abre **su VIEW-RE4.bat**. Usa los mismos permisos que
   RE4. Ambos juegos en ventana, Minecraft abierto sin minimizar.
3. Pulsa **Preparar Minecraft** ANTES de arm. RE4 se ajusta al monitor en el
   fondo, mientras el puente está detenido. Esto evita redimensionar/render
   Reset con HUD/bloques activos al pasar luego a la vista de RE4.
4. Conecta/configura solo si hace falta; si ya estaba preparado, repite únicamente
   **/re4bridge arm**, cierra el chat y pulsa **F9** desde Minecraft.
5. Prueba colocar/romper repetidamente y cambiar ranura con rueda. La vista debe
   permanecer activa, con el HUD completo dentro de RE4. Pulsa F8 para regresar.

F9 activo solo eleva la ventana ya ajustada, sin activar/redimensionar. Los
clics/rueda reales sobre RE4 se reenvían a Minecraft, evitando que el clic active
el juego que está tapando a Minecraft. La presentación usa WS_EX_NOACTIVATE.
No hay recuperación automática de foco ni rearme al cambiar de aplicación.
F8/desconexión de ventanas/pérdida de foco/cerrar VIEW liberan los botones
reenviados y retiran el observador. Se siguen restaurando estilos/bordes/tamaño/
posición/topmost y se escribe únicamente stop del IPC existente.

El observador de ratón solo vive durante la presentación activa, exige ambas
ventanas/PID originales y foreground Minecraft. Preserva botones simultáneos,
modificadores, rueda y coordenadas; no genera movimiento ni teclado, no duplica
raw input y no reenvía entradas inyectadas o clics en otras aplicaciones. Fallos
de envío paran la vista; no se sigue capturando a ciegas. La limpieza de ventana
se intenta incluso ante un fallo de retirada del observador. F9 exige haber
preparado el tamaño primero; si omites ese paso, el panel lo indica.

## La exportación enviada todavía está incompleta

Los tres archivos entregados son coherentes entre sí. EAT produjo220 triángulos
válidos (59 suelo,85 pendientes y76 paredes), con11 objetos habilitados y sin
errores. SAT encontró18 coincidencias de firma y0.4.30 las rechazó por exigir
una única ubicación. **No conocemos todavía los operandos de esas18 referencias**;
no se ha confirmado que sean alias del mismo manager.

El exportador nuevo enumera1..64 referencias y exige que **todas** apunten al
mismo manager alineado/dentro de bio4, con el layout/vtable/stride/contenido
validado. Dos destinos distintos se rechazan, sin primera coincidencia ni
mayoría. Registra cada RVA/destino, de modo que un informe incompleto también
permita diagnosticar la diferencia. Conserva límites de lectura/triángulos,
versionado, matrices, índices y comprobaciones de cambios de objeto/manager.

Repite **EXPORT-RE4-MAP.bat del paquete nuevo** con RE4 en la zona inicial,
Leon quieto, F8/puente detenido. No necesita Minecraft/host/VIEW. Envía el JSON
nuevo; OBJ y HTML son útiles para verlo pero no necesitas adjuntarlos otra vez.
El mapa aún no se importa al editor ni impide colocar dentro de obstáculos.
Es necesario completar SAT antes de preparar las losas y ocupación del mapa.
Salto y combate permanecen pendientes; esta entrega conserva los bloques,
cámara, suelo provisional y locomoción ya probados por el usuario.

## Validación y recuperación

115 pruebas Python pasan, incluidos11 fixtures nativos. La prueba de alias usa
18 referencias sintéticas: todas iguales se aceptan, una diferente se rechaza.
El escritorio simulado compila el C# Win32 real con PowerShell, comprueba400
clics de construcción/destrucción sin cambios de foco, rueda, botones/cleanup,
fallos, orden de tamaño antes de arm y restauración. El script WinForms se
analiza y se ejecuta su función real de retorno sobre el escritorio simulado.
No se ejecutan Windows, GLFW ni RE4 aquí; falta comprobar la presentación real.

No hay rebuild de Minecraft/DLL: se verifican byte a byte contra0.4.30 junto
con Fabric API, host e instaladores. DLL SHA256:
bd9e3f19eb213ae23e3ebbb930c9c8f97672ca592e1d83d7f12d604b0703042a.
Mod0.4.30 SHA256:1404fd36b863e195f3c3db3c5a97dba5f5513d8aa61bda49212e8a7789c746b2.
Control v2/80, escena v6/96352 y HUD v1/32792 intactos. Para volver al helper
anterior: F8, cierra VIEW nuevo y abre VIEW de0.4.30; no reinstales DLL/mod.
