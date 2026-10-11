# 0.4.16 — motivo inicial y un reintento de captura

Los informes graphics-capture-20261011-135432.json e
imports-audit-20261011-135454.json muestran dos resultados distintos: la DLL0.4.15
rechazó la captura al iniciar (estado3, cero llamadas), pero la auditoría con el
juego abierto encuentra una sola importación Direct3DCreate9 que apunta a la
exportación oficial de C:\\WINDOWS\\SysWOW64\\d3d9.dll. No identifican qué condición
falló al principio ni demuestran que se haya capturado un dispositivo.

Esta versión registra el motivo exacto del intento inicial y permite un único
reintento al solicitar DirectInput, con las mismas comprobaciones. **Dibujo de
bloques desactivado.** No cambian controles, cámara, ratón, correr ni F8. Se
incluyen los mismos JAR del mod Minecraft0.4.11 y Fabric API; no los reemplaces.

## Instalar y probar solo con RE4

1. Cierra RE4, host y VIEW. Ejecuta **UNINSTALL-RE4-CONTROLS.bat del paquete0.4.15
   que tienes instalado**. Su desinstalador reconoce el hash de esa DLL.
2. Extrae0.4.16 en otra carpeta. Ejecuta su **INSTALL-RE4-CONTROLS.bat**.
3. Abre RE4 y carga una partida. No necesitas Minecraft, START-RE4-CONTROLS,
   VIEW, controller/arm ni primera persona para esta prueba.
4. Ejecuta **CAPTURE-RE4-GRAPHICS.bat de0.4.16**, con los mismos permisos que RE4.
5. Comparte el archivo **graphics-capture-*.json** guardado junto al BAT.

El informe contiene `installation.initial` y `installation.latest`: estado,
motivo, error de Windows, RVA de importación y valores observado/esperado.
`phase` distingue DLL_PROCESS_ATTACH de DirectInput8Create y `attempts` es como
máximo2. Si el primer intento funciona, se conserva y no hay reintento.

Resultado buscado: hook_status=1 **y device_success mayor que0**. Estado1 sin
creaciones significa que se instaló la captura pero no se observó un CreateDevice
correcto. Puede haber ocurrido antes del reintento; no se recrea el dispositivo
ni se buscan objetos existentes. Comparte también el informe si continúa el
rechazo. Las direcciones registradas son retornos históricos, no objetos cuya
vida actual esté garantizada.

Si RE4 se cierra antes del menú, retira la DLL con UNINSTALL del paquete0.4.16 y
reinstala0.4.14, cuyo arranque y carga de partida ya confirmaste. Puedes recoger
eventos con COLLECT-RE4-CRASH sin volver a abrir RE4. No sobrescribas otras DLL.

## Alcance del cambio

El intento temprano valida las cabeceras PE32 y una importación por nombre única
del ejecutable principal que debe apuntar exactamente a la exportación de D3D9
del directorio de sistema. Mantiene comparación atómica y protección de página;
si falla la restauración intenta revertir el cambio. Durante DllMain no carga
DLL, llama COM, reserva heap ni crea el canal diagnóstico.

El único reintento ocurre antes de delegar la primera llamada DirectInput8Create
al sistema, si el resultado previo fue importación ausente/incompatible. Repite
los filtros íntegros. No reintenta un hook listo ni un fallo de protección. No
sondea cada frame ni fuerza la creación de dispositivos. El original DirectInput
se llama con los mismos argumentos y resultado.

La fachada IDirect3D9 y sus17 métodos se conservan de0.4.15. CreateDevice se
delega al original y el dispositivo se entrega sin modificar, retener ni llamar
su vtable. Interfaces desconocidas se delegan y cuentan; GetDirect3D de un
dispositivo devuelve la fábrica nativa. Compatibilidad real y cobertura necesitan
verificación en Windows. No hay EndScene, renderer, recursos GPU ni colisiones
de bloques.

IPC diagnóstico v2 de168bytes; lectura solo FILE_MAP_READ con seqlock y validación
de versión/build/PID. El lector acepta también v1/96bytes de0.4.15. Incluye el
auditor externo AUDIT-RE4-IMPORTS para comprobaciones adicionales de solo lectura.

## Validación en la nube

Pasan61 pruebas Python y4 suites C; DLLPE32 compilada con warnings como errores.
Tres fixtures de captura pasan ASan/UBSan. Se comprueban el parser y el instalador
reales con APIs simuladas: rechazo inicial, import resuelta después, un solo
reintento, motivo inicial conservado, exclusión de hooks listos/fallo de protección,
rollback y LastError. La fachada conserva forwarding/referencias; el lector valida
los dos formatos, snapshots completos y limpieza al fallar el remapeo extendido.
Instalación/desinstalación verifican respaldo/hash y rechazo de sobrescrituras.
Estas pruebas no ejecutan RE4 ni un controlador Direct3D real; arranque y captura
de0.4.16 quedan pendientes de tu prueba.
