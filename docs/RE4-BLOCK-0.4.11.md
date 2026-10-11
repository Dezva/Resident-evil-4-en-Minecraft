# 0.4.11: primer bloque visual dentro de RE4

El usuario confirmó que correr en0.4.10 funciona. Esta prueba añade un **cubo
de líneas verdes** asociado a un bloque seleccionado en Minecraft. Su posición
se fija delante de Leon al ejecutar el comando; después no sigue al personaje.
Sirve para verificar tamaño, cámara y anclaje antes de texturas y colisión.

**Es visual, sin colisión y sin ocultación por el terreno**: las líneas pueden
verse a través de paredes. No es todavía un objeto nativo, ni una copia general
del mundo Minecraft. No modifica guardados, posiciones, inventario o terreno.
El defecto de cámara anterior permanece pendiente; el cubo usa la cámara actual
de RE4, no una orientación ficticia del mod.

## Actualizar

1. Cierra ambos juegos y el host. Usa **UNINSTALL de0.4.10**.
2. Extrae este paquete en una carpeta nueva y ejecuta su **INSTALL**.
3. Retira el antiguo JAR RE4 Bridge de `%APPDATA%\.minecraft\mods` y copia
   `re4bridge-0.4.11.jar`. Conserva Fabric API.
4. Abre RE4 **en modo ventana** y carga una partida. Abre Minecraft y el
   **START-RE4-CONTROLS nuevo** con los permisos de RE4. VIEW/F9 conserva su uso.
   La detección inicial de Direct3D puede provocar una pausa breve.

## Probar un bloque

1. En Minecraft usa un mundo local en **Creativo**, coloca un bloque de piedra
   y apunta hacia él. Usa `/re4bridge controller` y espera conexión.
2. Mantén el punto de mira sobre ese bloque y escribe **`/re4bridge blocktest`**.
   Su selección usa el bloque bajo el punto de mira; no una posición escrita.
   El comando fija un cubo de1000 unidades RE4 aproximadamente tres tamaños
   delante de Leon, apoyado en su altura de pies actual. No lo uses junto a
   desniveles para esta primera prueba. No se ha medido todavía una escala real
   Minecraft↔RE4:1000 es un tamaño inicial ajustable, no una equivalencia validada.
3. Activa ratón/firstperson como antes si los necesitas, usa `/re4bridge arm`,
   cierra el chat y mira RE4 con VIEW/F9. Camina alrededor del cubo y gira.
   Debe quedar en el lugar donde se fijó; puedes atravesarlo por ahora.
4. Regresa a la vista de Minecraft con F9. Si el cambio de foco desarmó el
   puente, usa arm otra vez. **Rompe el bloque seleccionado**: el cubo debe
   retirarse. Colocar otro no lo selecciona automáticamente; repite blocktest.
5. F8 debe ocultar el cubo y devolver RE4 normal. `/re4bridge blocktest off`
   retira la selección. `/re4bridge blocktest 1500` permite repetir con mayor
   tamaño; tamaño permitido40..2500. Cambiar tamaño requiere nueva selección.

Abrir chat/pausar/perder foco oculta el dibujo junto con los controles. Tras
armar, la misma selección puede volver a mostrarse en su posición fija; no
se recalibra automáticamente. Cambiar mundo/dimensión o desconectar la retira.
El superplano, aldeas y mobs no se copian: solo se observa el bloque seleccionado.

## Si no aparece

Después de cargar RE4, espera unos10 segundos y consulta **`/re4bridge status`**.
Comparte la línea `Bloque visual: estado=...; frames=...; dibujos=...` y el
mensaje del host. No es necesario repetir una larga captura de cámara.

- **1:** dispositivo enganchado. frames debe aumentar; dibujos aumenta cuando
  hay selección armada válida y líneas dentro de la pantalla.
- **0:** esperando detección inicial. **2:** falló la sonda Direct3D.
- **3:** no se encontró un dispositivo compatible. **4:** detección ambigua
  o cambió la interfaz durante la instalación. Se omite el dibujo.
- **-1:** el host no informó el canal nuevo; comprueba host/DLL0.4.11.

El detector exige un dispositivo de una ventana visible del proceso actual, del mismo hilo que ejecuta la sonda, y
tabla de métodos exactamente igual a la de una sonda de Direct3D9 del sistema.
Algunas implementaciones, wrappers o interfaces Ex pueden no cumplirlo. No
usa direcciones de dispositivo supuestas. Si falla, conserva los controles
existentes; estado1 no demuestra por sí solo que hayas visto un cubo correcto.

## Cómo funciona y validación

El mod conserva el bloque seleccionado y envía una caja nativa fija, solo al
estar armado. El host valida seis coordenadas finitas y tamaños permitidos;
una memoria separada68bytes se publica con secuencia y TTL500ms. F8, estado
de controles, generación y datos caducados tienen prioridad sobre el dibujo.

Se dibuja al terminar la escena D3D9, proyectando aristas con CAMERA.v_mat y
FOV actuales. Se recortan líneas al plano frontal y viewport. Se captura y
restaura el estado gráfico, y se llama al EndScene original incluso al fallar.
No retiene texturas/buffers del dispositivo entre fotogramas. Si el juego
recrea por completo el dispositivo en lugar de Reset, reinicia RE4.

Linux: compila el mod Fabric1.21.1 y DLL PE32; pruebas de transporte autenticado,
anclaje/cajas, recorte y parada. Un fixture compila el **callback EndScene real**
con llamadas COM simuladas y comprueba restauración y errores. No ejecuta el
driver D3D9 ni prueba el descubrimiento real del dispositivo. Es necesaria la
prueba Windows antes de afirmar que el bloque se ve y permanece fijo.

Para volver atrás: cierra juegos/host, usa UNINSTALL0.4.11 e INSTALL0.4.10,
restaura JAR/host0.4.10. Conserva partidas y copias.
