# Diagnóstico de importaciones — conservar 0.4.15 instalada

La salida del usuario muestra "incompatible o modificada", cero creaciones y
dibujo desactivado. hook_status3 agrupa varias rutas; esa línea no identifica
la causa concreta ni demuestra que exista un mod ajeno o un problema del driver.

Extrae el ZIP de diagnóstico en una carpeta nueva. **No tiene INSTALL, DLL ni
mods de Minecraft**, y no sustituye el paquete0.4.15 instalado.

1. Mantén RE4 abierto en una partida; no necesitas Minecraft, host ni VIEW.
2. Ejecuta AUDIT-RE4-IMPORTS.bat. Usa los mismos permisos que RE4 si se ejecuta
   como administrador.
3. Comparte imports-audit-*.json, guardado junto al BAT. También se escribe un
   informe si una parte de la lectura falla.

El diagnóstico lee cabeceras PE32, las importaciones de bio4.exe, la exportación
Direct3DCreate9 de d3d9.dll, rutas de esos módulos y el IPC de captura existente.
Informa tablas de nombres ausentes, importaciones ordinales, entradas cambiadas,
duplicadas, límites o lecturas incompletas. Una tabla de nombres ausente puede
tener una IAT ya resuelta; solo se describe, no se usa para instalar un hook.

No escribe en el proceso, no llama COM, no busca objetos/dispositivos, no cambia
importaciones, partidas ni configuración. OpenProcess solo pide permisos de
lectura/consulta y el IPC se abre FILE_MAP_READ. Límites:2MiB,20.000 lecturas,
10segundos,256descriptores/4.096entradas por tabla. Resultados incompletos se
identifican; no equivalen a un rechazo nativo reproducido.

**Lee el estado actual después de abrir el juego.** Si las importaciones actuales
son válidas pero el canal conserva estado3, el motivo del rechazo temprano sigue
sin conocerse; no se presenta un problema de tiempo de carga como confirmado.
No activa bloques ni obliga a reinstalar. La reparación depende de este informe.

Cinco pruebas sintéticas pasan: importación válida sin cambiar bytes, tabla
OriginalFirstThunk ausente con IAT resuelta, destino distinto, duplicación/ordinal,
exportación reenviada, rutas/límites/lecturas fallidas y limpieza de handles/reporte.
La ejecución Win32 real requiere el equipo del usuario.
