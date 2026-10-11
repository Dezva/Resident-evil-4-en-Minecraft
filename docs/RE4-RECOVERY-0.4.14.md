# 0.4.14 — recuperación tras el cierre de 0.4.13

0.4.13 está retirada: el usuario informa tres cierres antes del menú principal.
El detector llamaba métodos sobre candidatos encontrados en memoria y sustituía
su tabla de métodos. Una tabla compatible no demuestra que el objeto siga vivo.
Es un riesgo identificado, pero falta el registro de Windows para confirmar la
causa exacta del cierre.

0.4.14 excluye completamente el renderer de la DLL: no crea su IPC, no busca
dispositivos, no llama métodos COM y no instala hooks gráficos. Conserva los
controles, ratón, cámara experimental, correr y retorno F8 anteriores. No cambia
la cámara ni ofrece bloques visuales. El archivo histórico block_render.h no
se compila y contiene una barrera de compilación para evitar reactivarlo.

## Instalar la recuperación

1. Cierra RE4, host y VIEW.
2. Ejecuta UNINSTALL-RE4-CONTROLS del paquete **0.4.13** que instalaste.
   Comprueba el hash antes de retirar únicamente esa DLL. No borra partidas.
3. Extrae 0.4.14 en una carpeta nueva y ejecuta su INSTALL-RE4-CONTROLS.
4. Abre RE4 primero, sin host ni VIEW, y comprueba si llega al menú y carga partida.
5. Si abre, puedes usar START-RE4-CONTROLS y VIEW como antes. No uses blocktest:
   el dibujo de bloques está desactivado en esta recuperación.

**Conserva los mods Minecraft 0.4.11 y Fabric API existentes**, incluidos sin
cambios en el ZIP completo. No instales una segunda copia del mismo mod.
El status antiguo del mod puede mostrar estado visual -1: no hay renderer.

## Recoger el cierre anterior

Ejecuta COLLECT-RE4-CRASH.bat y comparte re4-crash-*.json. Lee eventos técnicos
de bio4.exe de los últimos siete días; no abre RE4, no necesita Minecraft/host
y no requiere provocar otro cierre. Solo escribe el informe junto al colector.
No exporta volcados completos ni modifica antivirus, registro o archivos del juego.
Si no hay eventos, el informe lo indica; eso no prueba que no haya habido fallo.

Alternativa para volver al paquete anterior: desinstala desde el paquete cuya DLL
está instalada y reinstala 0.4.12. Esa versión abría en la prueba del usuario,
aunque su detector no encontraba el dispositivo y no aparecían bloques.

Validación cloud: la compilación y los checks del protocolo no equivalen a
ejecutar el juego en Windows. El arranque de esta recuperación requiere prueba
en el equipo del usuario. La integración gráfica necesitará capturar un
dispositivo vivo desde su creación, antes de volver a intentar dibujar.
