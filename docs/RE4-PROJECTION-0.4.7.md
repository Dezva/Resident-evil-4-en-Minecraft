# 0.4.7: selección del grupo de proyección observado

ZIP completo con DLL 0.4.7, mod/host 0.4.3 y todos los START/VIEW/instaladores.
Conserva tus mods 0.4.3 y Fabric API. Movimiento/cámara siguen siendo prototipos
pendientes de validación real; esta entrega cambia la selección de proyección.

El nuevo PROBE confirmó dos destinos entre las siete coincidencias:
seis llaman al thunk RVA0xE395, que salta a RVA0x19F3A0; la séptima, en
RVA0x1F7BAD, llama a RVA0x344F y salta a RVA0x202F60. La referencia pública
re4_tweaks Game.cpp, revisión92c0208bd09c29c9640e13a2493b8e6bd6edd700, identifica
CameraCurrentProjection en VA0x59F3A0 (RVA0x19F3A0, base preferida0x400000).
Esto identifica el grupo por la función, no por cuántas coincidencias tiene.

0.4.7 conserva el caso genérico de destino común y añade un perfil **estricto
del layout observado**: exige exactamente los siete RVA de llamadas, ambos
thunks E9 y sus destinos calculados, dentro del módulo y ejecutables. Solo
en ese perfil instala las seis llamadas de proyección y excluye la otra.
No aplica el perfil a ejecutables con otras direcciones, llamadas ausentes,
destinos desconocidos o un hook previo. Otros layouts quedan rechazados.
No es un buscador general para todas las ediciones RE4 UHD.

## Actualización desde 0.4.6

1. Cierra RE4 y el host. Ejecuta **UNINSTALL-RE4-CONTROLS.bat de 0.4.6**.
2. Extrae este ZIP en una carpeta nueva y ejecuta su **INSTALL-RE4-CONTROLS.bat**.
   Comprueba el mensaje de copia de partidas. Conserva los mods actuales.
3. Abre RE4 con partida y Minecraft, y usa el **START-RE4-CONTROLS.bat nuevo**
   con los mismos permisos de RE4. VIEW-RE4 también viene en esta carpeta.
4. En Minecraft: `/re4bridge controller`, `/re4bridge firstperson` si estaba
   apagado y `/re4bridge arm`. Cierra el chat. **F8** vuelve a RE4 normal.

Prueba primero la activación FP. Luego W/S, A/D lateral sin mover el ratón,
diagonales y detenerte. No implementa salto/sprint/agachado ni física Minecraft;
el avance usa locomoción/colisión RE4 con rumbo separado de la mirada.

Si FP sigue apagado, envía el **PROBE-RE4-NATIVE.bat nuevo**: muestra destinos
resueltos y projection_observed_profile. Si se activa pero tiene saltos o
movimiento incorrecto, envía TRACE-RE4-CAMERA y un vídeo. Un hook instalado puede
mostrar destino fuera de bio4 porque está en nuestra DLL; no es fallo por sí solo.

Para volver atrás, cierra RE4, usa el UNINSTALL nuevo y el INSTALL anterior.
Instaladores verifican el hash propio y no reemplazan DLL ajenas.

Verificación Linux: tres suites C y31 pruebas Python pasan, incluyendo el perfil
de siete llamadas proporcionado y el rechazo de perfiles incompletos/alterados.
La activación y el resultado visual con 0.4.7 aún no se probaron en Windows.
