# Actualización nativa de cámara 0.4.4

Conserva el mod Minecraft **0.4.3**, Fabric API y el host actual. Este ZIP
actualiza solamente la DLL del puente e incluye el diagnóstico ampliado.

El registro recibido confirma DLL 0.4.3 en disco, 223 muestras y control activo.
Entre el inicio del control y aproximadamente el segundo 41, el yaw de León
permanece -2.297626 radianes, sin movimiento del ratón. La dirección de
CameraControl y QuasiFPS también permanece estable al soltar W. Por tanto,
el registro no demuestra que el desvío visible venga de un giro de León.
No contenía las matrices de vista finales; el origen exacto sigue sin confirmar.

0.4.4 corrige dos problemas del punto de entrada existente, sin nuevos hooks:

- Sustituye hitCheck durante primera persona activa, evitando ejecutar antes
  sus ajustes de colisión y órbita. Sigue el flujo documentado de cámara libre
  de re4_tweaks, revisión 92c0208bd09c29c9640e13a2493b8e6bd6edd700. Fuera de
  FP o si falla una validación, ejecuta la rutina original.
- Usa una mirada de 1000 unidades en lugar de 1. En las coordenadas grandes
  del registro, sumar una dirección unitaria a la posición pierde precisión
  float. Fija roll a cero y conserva el FOV definido en QFPS_OFFSET.

No cambia colisiones del jugador, inventario, guardados ni movimiento W/S/A/D.
La eficacia sobre el desvío en reposo necesita probarse en RE4 Windows.

## Instalación

1. Cierra RE4 y el host. Ejecuta **UNINSTALL-RE4-CONTROLS.bat de tu carpeta
   0.4.3**; solo retira la DLL cuyo hash coincide con ese paquete.
2. Extrae este ZIP en una carpeta nueva y ejecuta su
   **INSTALL-RE4-CONTROLS.bat**. Comprueba el mensaje de copia de seguridad de
   partidas. Si indica que existe otra DLL, no la reemplaces manualmente.
3. Abre RE4 con la partida y usa tu host y VIEW-RE4 actuales. Minecraft conserva
   su mod 0.4.3. Ejecuta controller, firstperson si estaba apagado y arm.
4. Camina, suelta W y espera sin mover el ratón. Prueba luego giros lentos con
   el ratón. **F8** mantiene la función de detener y volver a RE4 normal.

Si sigue mirando de lado, usa **TRACE-RE4-CAMERA.bat de este nuevo ZIP** y repite
la prueba del README-TRACE.md. Ahora registra también las matrices y la dirección
de CameraControl.Camera_60. Envía el nuevo JSONL; no hace falta repetir el
diagnóstico antiguo. El script no cambia ni arma el puente.

Para regresar a la DLL anterior, cierra RE4, ejecuta el UNINSTALL de esta carpeta
0.4.4 y luego el INSTALL de tu carpeta 0.4.3.

Validación Linux: DLL PE32/i386 compilada, tres suites nativas y 27 pruebas
Python pasan. La regresión de mirada comprueba el error angular después de
sumar la posición grande y el pitch del registro. No se ejecutó RE4 en la nube.
