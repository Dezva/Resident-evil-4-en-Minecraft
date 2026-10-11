# 0.4.5: mirada independiente y movimiento relativo a la cámara

Paquete completo: DLL **0.4.5**, mod **0.4.3**, host y VIEW-RE4. El mod sigue
siendo el mismo: no hace falta cambiar los mods instalados ni Fabric API.
Incluye START-RE4-CONTROLS.bat para iniciar todo desde esta carpeta.

En primera persona, el ratón controla una orientación independiente del rumbo
del personaje. W/S avanzan y retroceden respecto a esa mirada; A/D solicitan
desplazamiento lateral y W+A/W+D las diagonales. Se usa la locomoción nativa:
el puente orienta al personaje hacia el rumbo solicitado y envía avance, sin
escribir posiciones ni desactivar colisiones. La cámara mantiene la mirada.
Las diagonales usan un solo avance, sin aumentar la velocidad.

Esto es un prototipo de movimiento horizontal. No implementa todavía salto,
sprint, agachado, física Minecraft ni animaciones laterales. RE4 puede imponer
restricciones por estado del personaje o animaciones. Fuera de primera persona
se conservan los controles tanque anteriores.

El vídeo y el registro 0.4.4 muestran saltos y deriva al detenerse. En el segundo
37.8, yaw del jugador = 1.841018, pero CameraControl.Camera_60 apunta a 2.049860,
unos 12 grados distintos, y su ojo difiere unos 212 unidades del ojo solicitado.
Esta versión añade un hook en el sitio de llamada validado a CameraCurrentProjection
para fijar también las matrices/punto/ejes de CameraControl.Camera_60 y
GLOBAL_WK.Camera_74 antes del dibujo. Conserva la proyección y restaura el estado
previo antes del siguiente procesamiento nativo. Firmas de GlobalPtr y proyección
son obligatorias: si faltan, no anuncia capacidad FP.

Las direcciones/cámara/control siguen sujetos a F8, desarmado y TTL de 500 ms.
No se rearma automáticamente. La DLL nueva no tiene prueba visual en Windows;
compilar y pasar pruebas no garantiza el comportamiento real.

## Actualización desde 0.4.4

1. Cierra RE4 y el host. Ejecuta **UNINSTALL-RE4-CONTROLS.bat de 0.4.4**.
2. Extrae 0.4.5 en una carpeta nueva. Conserva tus mods actuales 0.4.3.
3. Ejecuta el **INSTALL-RE4-CONTROLS.bat nuevo**, que verifica la copia de partidas.
4. Abre los juegos en ventana y ejecuta **START-RE4-CONTROLS.bat nuevo** con los
   mismos permisos que RE4. Usa el VIEW-RE4 de esta carpeta si quieres ver RE4.
5. Activa `/re4bridge controller`, `/re4bridge firstperson` si estaba apagado y
   `/re4bridge arm`. Cierra el chat.

## Prueba

En una zona despejada, con el ratón inmóvil: W debe avanzar hacia la mirada,
S retroceder y A/D desplazarse lateralmente sin girar la cámara. Prueba W+A/D
y después suelta todas las teclas: la vista debe conservar su orientación.
Gira suavemente el ratón caminando y estando quieto. Comprueba una pared para
verificar que la locomoción respete las colisiones y **F8** para volver a RE4 normal.

Si el personaje no se desplaza correctamente o hay saltos de cámara, detén con
F8 y envía el registro del TRACE-RE4-CAMERA de este paquete y un vídeo corto.
El registro nuevo incluye GLOBAL_WK.Camera_74. Si FP no está disponible, envía
PROBE-RE4-NATIVE (Global/projection matches); no reemplaces DLL a mano.

Para regresar: cierra RE4, ejecuta el UNINSTALL de 0.4.5 y el INSTALL de 0.4.4.
Las partidas y sus copias se conservan.
