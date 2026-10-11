# 0.4.6: validación de múltiples llamadas de proyección

Paquete completo: DLL 0.4.6, mod/host 0.4.3, START-RE4-CONTROLS y VIEW-RE4.
Conserva tus mods actuales. El movimiento relativo a cámara de 0.4.5 sigue
disponible en primera persona: WASD lateral/diagonal con mirada independiente,
mediante locomoción y colisiones nativas. Salto/sprint/agachado no implementados.

El PROBE enviado muestra projection_matches=7. 0.4.5 exigía exactamente uno
y por ello nunca llegaba a habilitar FP. El informe anterior no mostraba los
destinos de esas siete llamadas: no demuestra que pertenezcan a la misma función.

0.4.6 recoge todos los sitios de llamada de esa firma, hasta 32, y exige que
cada CALL rel32 apunte al mismo destino dentro del módulo y ejecutable. Solo
entonces instala los hooks en esas llamadas y anuncia FP. Cero coincidencias,
exceso, destinos distintos o fuera del módulo mantienen FP apagado. No escoge
la primera coincidencia ni acepta una mayoría. Las demás firmas siguen únicas.
Si las siete llamadas de tu ejecutable no comparten destino, aún habrá que
identificar la función correcta con el nuevo diagnóstico; no se da por resuelto.

## Instalación y prueba

1. Cierra RE4 y el host. Ejecuta UNINSTALL-RE4-CONTROLS.bat de tu carpeta 0.4.5.
2. Extrae este ZIP en una carpeta nueva y ejecuta su INSTALL-RE4-CONTROLS.bat.
   Conserva los mods 0.4.3 y verifica el mensaje de copia de partidas.
3. Abre RE4 con partida cargada y Minecraft. Ejecuta el START-RE4-CONTROLS.bat
   nuevo con los mismos permisos de RE4. Usa el VIEW-RE4 de esta carpeta.
4. Ejecuta `/re4bridge controller`, `/re4bridge firstperson` si estaba apagado
   y `/re4bridge arm`. Cierra el chat. F8 conserva el retorno a RE4 normal.
5. Prueba W/S y A/D sin mover el ratón, diagonales y quedarte quieto. El
   comportamiento real de cámara/movimiento continúa pendiente de Windows.

Si sigue indicando cámara no disponible, ejecuta **PROBE-RE4-NATIVE.bat de
0.4.6 como administrador** y envía la salida completa. Incluye
projection_unique_targets, projection_shared_target y projection_calls con
los RVA/destinos/bytes de cada llamada. El script solo lee.

Con DLL activa, una rama instalada puede apuntar fuera de bio4 hacia nuestra
DLL. Por sí solo target_in_module=false en ese caso no significa fallo.

Para volver atrás: cierra RE4, usa el UNINSTALL nuevo y el INSTALL del paquete
anterior. Los instaladores comprueban el hash propio; no reemplazan DLL ajenas.

Verificación Linux: tres suites C, 30 pruebas Python y fixtures de instalación/
diagnóstico. Las regresiones prueban siete llamadas con destino común y rechazo
de un destino discrepante, inválido, opcode distinto o lista vacía. No se ha
probado la activación de FP en tu RE4 Windows con esta versión.
