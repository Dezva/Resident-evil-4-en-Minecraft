# 0.4.17 — rutas gráficas de Windows y diagnóstico

**Resultado confirmado en Windows:** graphics-capture-20261011-142924.json
registra build417, hook_status1 instalado durante DLL_PROCESS_ATTACH, un intento,
dos fábricas correctas y un CreateDevice correcto/S_OK, sin fallos ni interfaces
desconocidas devueltas. La ruta cargada es C:\\WINDOWS\\SYSTEM32\\d3d9.dll, aceptada
como SystemDirectory. Confirma la diferencia de rutas que bloqueaba0.4.16.
El dispositivo se creó en modo ventana1366×768; no se habilitó dibujo y este
informe no acredita estabilidad prolongada, Reset ni recreación del dispositivo.
El siguiente paso es una prueba de dibujo de un cubo, antes de conectar bloques.

graphics-capture-20261011-141909.json confirma DLL0.4.16, dos intentos y el mismo
rechazo en ambos: motivo7, «ruta de d3d9 distinta del sistema». Cero llamadas a
fábrica/dispositivo. Esto localiza el bloqueo antes del hook, pero no contiene
las rutas que se compararon. La auditoría anterior sí encuentra la DLL oficial.

La comprobación anterior exigía la ruta SysWOW64 si esa API estaba disponible.
Sin embargo, GetModuleFileNameW conserva el formato utilizado al cargar la DLL:
un proceso de32bits puede cargarla mediante System32 y Windows redirigirla a
SysWOW64. Esta versión admite **ambas rutas exactas obtenidas de las APIs de
Windows**. No admite otras carpetas ni subdirectorios. Sigue exigiendo una única
importación por nombre que coincida con la exportación de la DLL del sistema.
El informe mostrará las rutas para confirmar si este era el caso del usuario.

Referencia: [GetModuleFileNameW](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulefilenamew)
y [redirección WOW64](https://learn.microsoft.com/en-us/windows/win32/winprog64/file-system-redirector).

## Actualizar y probar

1. Cierra RE4, host y VIEW. Ejecuta **UNINSTALL-RE4-CONTROLS del paquete0.4.16
   instalado**, antes de borrar o sustituir los archivos del paquete.
2. Extrae0.4.17 y ejecuta su **INSTALL-RE4-CONTROLS**. Puedes reutilizar la carpeta
   si retiraste los archivos anteriores; una carpeta nueva solo evita mezclas.
3. Abre RE4 y carga una partida. No necesitas Minecraft, START-RE4-CONTROLS,
   VIEW, controller/arm ni primera persona para esta prueba.
4. Ejecuta **CAPTURE-RE4-GRAPHICS.bat** de0.4.17 con los mismos permisos que RE4.
5. Comparte el nuevo **graphics-capture-*.json** guardado junto al BAT.

Los JAR Minecraft0.4.11/Fabric API son los mismos; no necesitas cambiarlos.
**Dibujo de bloques desactivado.** Resultado buscado: hook_status1 y
device_success mayor que0. Un hook instalado sin creaciones observadas no
demuestra captura de dispositivo ni dibujo. Comparte también el informe si
continúa fallando. No se buscan objetos existentes ni se recrean dispositivos.

El nuevo campo `paths`, dentro de `installation.initial` y `latest`, contiene
`loaded`, `wow64`, `system` y `matched`: SysWOW64, SystemDirectory o none.
Conserva motivo, RVA, valores, fase y un máximo de dos intentos. La ruta ausente
o truncada se rechaza; el informe permite distinguirla de una ruta diferente.

Si RE4 se cierra al arrancar, desinstala desde0.4.17 y reinstala0.4.14, cuyo
arranque/carga ya confirmaste. COLLECT-RE4-CRASH recoge eventos sin abrir RE4.

## Alcance y validación

No cambia la fachada gráfica de0.4.15 ni controles/cámara/correr. Durante DllMain
no carga bibliotecas, llama COM, reserva heap ni crea IPC. Las direcciones de
dispositivos son retornos históricos: no se llama su vtable ni se retienen.
Sin EndScene, sonda de dispositivos, barrido de memoria o recursos GPU.

IPCv3/3296bytes añade rutas UTF-16 por intento; lector FILE_MAP_READ con seqlock
compatible con v1/0.4.15 y v2/0.4.16. Build417. Pasan62 pruebas Python y4 suites
C. Tres fixtures de captura pasan ASan/UBSan. Pruebas del instalador real con
APIs simuladas cubren ambas rutas bajo WOW64, Windows32, mayúsculas, API ausente,
truncamiento/límites, rechazo de DLL local/subdirectorios y conservación de rutas.
El lector comprueba snapshots sin escritura, Unicode/terminadores, identidad y
limpieza de vistas. DLLPE32 compila con -Werror; respaldo/hash propio y retirada
sin sobrescribir otras DLL verificados. La nube no ejecuta RE4 ni Direct3D real.
La captura inicial de0.4.17 queda confirmada por el informe del usuario descrito
arriba; el dibujo y los cambios de modo/dispositivo siguen pendientes.
