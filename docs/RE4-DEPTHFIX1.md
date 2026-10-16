# 0.4.57-depthfix1 — interrupción al mirar bloques

Parche nativo de la0.4.57. Mantiene DLL/IPC build457 y el mismo mod/core0.4.57.
El JAR, Fabric API, host de controles, combate, trampas y cámara de patadas
conservan sus bytes de la entrega0.4.57. No requiere compilar/reinstalar Minecraft.

## Qué se corrige

El usuario reporta interrupción al ocultar bloques detrás de un árbol y al
preparar VIEW con construcciones existentes. Los logs muestran parada3 por
referencia de colisión sin actualizar500ms; la telemetría real muestra47973
intentos de profundidad y594 dibujos, proyección rechazada, cero errores de
Direct3D/restauración. RE4 y el host seguían abiertos; no se interpreta como
un crash del juego ni como error de permisos.

El selector de pase0.4.57 fijaba depth_seen antes de validar cámara, viewport y
proyección; admitía comparaciones ALWAYS/NEVER/EQUAL/NOTEQUAL, que no permiten
obtener la proyección de profundidad del mundo. Una primera muestra inválida
bloqueaba muestras posteriores y provocaba reintentos durante cada draw.
El fixture reproduce el rechazo con el código anterior.

Ahora espera un pase compatible y una cámara/viewport/proyección validados
antes de fijarlos; permite recuperarse dentro del mismo fotograma. Escaneo GPU
máximo una vez por intervalo de profundidad y un intento de geometría por
fotograma/pase. La parada500ms, F8, límites, generación, colisiones y rechazo de
escrituras desconocidas siguen intactos. No se vuelve al overlay sin profundidad.
Las mediciones reales no registraron la comparación del primer pase fallido;
la relación con ese pase se ha reproducido en un fixture, no en una captura GPU.

## Actualizar desde0.4.57

1. Cierra RE4 y el host. Extrae el paquete en una carpeta nueva.
2. Ejecuta UNINSTALL-RE4-CONTROLS.bat del paquete nuevo para retirar solo una
   DLL propia reconocida; después INSTALL-RE4-CONTROLS.bat instala el parche
   y verifica la copia de seguridad de partidas/configuración.
3. Conserva el mod0.4.57 y Fabric API existentes. Usa los lanzadores del paquete
   nuevo para disponer también de la lectura de diagnóstico actualizada.

El instalador rechaza sobrescribir una DLL o retirar una ajena. No modifica
bio4.exe ni requiere restaurar el modo administrador. Codex no ejecutó estos
pasos contra la instalación real; las pruebas usan archivos sintéticos.

## Comprobación breve

Conserva los bloques que ya tienes. Prepara VIEW y mantén la sesión10 segundos
sin moverte. Luego rodea el mismo árbol mirando hacia el bloque otros10 segundos.
Debe seguir armado. Comprueba que el tronco tape el bloque y que al volver al
lado descubierto el bloque conserve colisión. F8 debe detener el puente.
Trabaja offline. La oclusión real y los FPS siguen pendientes de esa prueba;
las pruebas COM no contienen el depth buffer real de la GTX1050.

No se amplía el alcance del terreno ni se cambia la pausa de flechas fuera de
cobertura. Modelo/skin Minecraft sobre Leon permanece pendiente.
