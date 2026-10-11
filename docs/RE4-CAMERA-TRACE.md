# Diagnóstico de cámara al detenerse

Este complemento registra durante 45 segundos la posición y orientación de León,
las etapas de cámara y el estado del puente. Solo lee: no instala DLL, no cambia
mods, no activa controles ni modifica el inventario. Usa el Python 3.10 o posterior
que ya requiere el host.

1. Extrae este ZIP en una carpeta nueva. Mantén tu instalación 0.4.3 y abre RE4 con
   una partida cargada, Minecraft y el host de controles como de costumbre.
2. Ejecuta **TRACE-RE4-CAMERA.bat como administrador** si RE4 también requiere ese
   nivel de permisos. Comenzará el registro inmediatamente.
3. Vuelve a Minecraft. Activa controller y primera persona con tus comandos
   habituales, ejecuta `/re4bridge arm` y cierra el chat. Cambiar de ventana puede
   desarmar el puente; por eso se arma después de iniciar el registro.
4. Camina con W durante unos 5 segundos. Suelta W y permanece quieto unos
   10 segundos, sin mover el ratón. Después mueve únicamente el ratón y repite
   caminar y detenerte. Mantén Minecraft enfocado durante la prueba; puedes usar
   VIEW-RE4 y F9 para ver RE4. F8 sigue siendo tu retorno a RE4 normal.
5. Al finalizar, envía el archivo **camera-trace-....jsonl** que se creó en la
   carpeta de este diagnóstico. Si aparece un error, envía su texto también.

Los valores permiten comparar movimiento y reposo sin depender de capturas. Las
lecturas de diferentes estructuras no son un único fotograma atómico. El hash
indica qué DLL está instalada en disco; no demuestra que RE4 se reinició después
de instalarla. Este diagnóstico todavía no corrige el desvío.
