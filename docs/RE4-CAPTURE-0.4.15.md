# 0.4.15 — captura de creación gráfica, sin dibujo

**Resultado del usuario:** hook_status3 ("incompatible o modificada"), cero
creaciones correctas. La causa concreta aún no se conoce. Se publica el
[diagnóstico externo de importaciones](RE4-IMPORT-AUDIT.md) para continuar
sin cambiar la DLL instalada. El ZIP0.4.15 anterior se conserva sin sobrescribir.

Prueba de la nueva captura tras el arranque correcto de0.4.14. **Todavía no
aparecerán bloques.** Conserva controles, ratón, cámara, correr y F8; el mod
Minecraft0.4.11/API no cambia. No reactiva el detector retirado de0.4.13.

## Instalar y comprobar

1. Cierra RE4, host y VIEW. Ejecuta **UNINSTALL del paquete0.4.14** instalado.
2. Extrae0.4.15 en otra carpeta y ejecuta su INSTALL-RE4-CONTROLS.
3. Abre RE4 y carga una partida. Esta prueba **no necesita Minecraft, host,
   controller/arm, primera persona ni VIEW**.
4. Ejecuta **CAPTURE-RE4-GRAPHICS.bat** del paquete0.4.15. Si RE4 tiene permisos
   de administrador, ejecuta el diagnóstico con los mismos permisos.
5. Comparte **graphics-capture-*.json**, guardado junto al BAT. Se guarda también
   si falta el canal; no tienes que provocar un fallo ni activar bloques.

Resultado buscado: hook_status=1 y device_success mayor que0. Esto demuestra que
una llamada real CreateDevice devolvió un dispositivo, no que siga vivo ahora
ni que el dibujo funcione. Si no captura, comparte igualmente el informe: no
se buscarán dispositivos por offsets ni se llamarán métodos sobre candidatos.

Si RE4 vuelve a cerrarse: detén esta prueba, desinstala desde0.4.15 y reinstala
el paquete0.4.14 confirmado. COLLECT-RE4-CRASH recoge eventos anteriores sin abrir
RE4. No sobrescribas DLL de otros mods.

## Qué cambia

La DLL busca una importación PE32 única de Direct3DCreate9 en el ejecutable
principal. Debe apuntar exactamente a la exportación de d3d9.dll del directorio
de sistema (SysWOW64 en Windows de64bits). Durante DllMain se cambia únicamente
esa entrada de importación, conservando protección y comparando su valor al
escribir. No carga bibliotecas, reserva memoria ni llama COM en DllMain. Ausencia,
tabla de nombres no disponible, cambios previos, ambigüedad o biblioteca externa
rechazan la captura. La carga tardía del puente puede perder la creación inicial;
no se fuerza recrear el dispositivo ni se busca uno existente.

La llamada original Direct3DCreate9 se conserva. Solo si devuelve una interfaz
real se crea una fachada externa de IDirect3D9 con sus17 métodos. Delega los
argumentos y resultados, mantiene identidad para IUnknown/IDirect3D9 y balancea
referencias propias. Si falla la reserva, devuelve la interfaz original. Las
interfaces desconocidas se delegan y se cuentan: pueden evitar nuestra captura,
por lo que no se declara una cobertura completa ni compatibilidad con wrappers.

CreateDevice se ejecuta en la interfaz original. El dispositivo devuelto se
entrega al juego **sin modificarlo**, sin retenerlo ni llamar ninguno de sus
métodos. Solo se guardan valores de retorno históricos. No hay hook de EndScene,
copias de tablas del dispositivo, sonda auxiliar, barrido de heap ni recursos GPU
adicionales. GetDirect3D en el dispositivo seguirá devolviendo la fábrica nativa;
la identidad entre esa fábrica y nuestra fachada no es idéntica. Esta interacción
necesita validación en el juego antes de extender la integración.

IPC diagnóstico independiente96bytes/version1 y lectura de seqlock. El lector
abre el canal con FILE_MAP_READ, no escribe en RE4 ni necesita TCP/config/token.
No modifica partidas, archivos del juego ni parámetros de creación del dispositivo.

## Evidencia y límites

Compilación MinGW x86 con warnings como errores. Pasan55 pruebas Python y4 suites
C existentes; los3 fixtures de captura nativa también pasan con ASan/UBSan. Las
pruebas ejecutan el parser
PE, el instalador de importación y la fachada reales contra APIs simuladas:
límites, importación cambiada/ambigua, protección/rollback,17 métodos, identidad,
vida de la fábrica, fallo de reserva y salidas inválidas al fallar CreateDevice.
El lector comprueba permisos de lectura, cabeceras/build/PID, snapshots cambiantes
y cierre de handles. Son pruebas cloud: el arranque y la creación real del equipo
Windows siguen pendientes. Una captura correcta será la base para añadir dibujo.
