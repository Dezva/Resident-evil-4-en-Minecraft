# 0.4.46: pulso de interacción, detector de ganados y partículas

Esta compilación parte de la 0.4.45 disponible en el repositorio. No recupera
una 0.4.46 de otro chat. Incluye correcciones para los registros y capturas
aportados: R llega a la DLL pero no produce interacción; el detector de enemigos
está ausente; los fragmentos de bloques se mueven a saltos.

## Actualizar

1. Cierra Minecraft, RE4, el host y VIEW.
2. Extrae el paquete completo en una carpeta nueva. Deja solamente
   `re4bridge-0.4.46.jar` y Fabric API 1.21.1 en los `mods` de tu instancia.
3. Ejecuta `UNINSTALL-RE4-CONTROLS.bat` y después
   `INSTALL-RE4-CONTROLS.bat` del paquete nuevo. El desinstalador reconoce la
   DLL445 publicada; el instalador verifica la DLL nueva y respalda partidas.
4. Abre ambos juegos, `START-RE4-CONTROLS.bat` y VIEW; prepara Minecraft y pulsa F9.
   Mod, host y DLL446 deben actualizarse juntos.

## Cambios y comprobación en Windows

- **R:** mantiene la acción contextual nativa durante un máximo de 120 ms,
  con un solo disparo inicial. Las siguientes lecturas del pad conservan la
  pulsación; las llamadas auxiliares de cámara no borran su disparo. El respaldo
  de cámara también atiende la interacción cuando el pad lleva 80 ms inactivo.
  No se cambia el foco ni se escribe una E en Minecraft. Se usa la acción de RE4,
  equivalente a interactuar, independiente de la tecla configurada en Windows.
  Se elimina el mensaje de chat que afirmaba una interacción sin confirmarla.
  Prueba R ante una puerta, objeto recogible o aviso de acción, con el foco en
  Minecraft y sin ningún menú abierto. El contador de pulsos no confirma por sí
  solo que RE4 haya ejecutado la acción.
- **Ganados:** la búsqueda de la firma conocida del SDK acepta varios sitios
  que apunten al mismo administrador. Rechaza destinos distintos, fuera del
  módulo o sin alineación; no usa direcciones adivinadas. En los registros
  aportados `enemy_reason=6` y `enemy_scanned=0` confirman que el detector anterior
  no estaba disponible, pero no permiten demostrar cuántas firmas había.
  Coloca una pared de bloques y comprueba la detención del ganado y el aumento
  de contactos/correcciones en `/re4bridge status`. Si continúa en detector 6,
  ejecuta `PROBE-RE4-NATIVE.bat`: ahora informa coincidencias, destinos y tamaño
  del administrador mediante lecturas, sin modificar el juego.
- **Partículas:** la DLL predice el movimiento de los fragmentos entre paquetes
  de 20 Hz en cada fotograma, con un horizonte máximo de 50 ms. Solo modifica
  las cajas visuales de una copia local. Conserva los colisionadores originales;
  reinicia la predicción al cambiar sesión, actor, material o conjunto de
  fragmentos. Los paquetes atrasados no prolongan su vida.

R/F6 conservan las guardas de F8, menú, caducidad, generación y muerte. F6
continúa limitado a eventos/cinemáticas que admiten la cancelación nativa.

## Alcance de la validación

Se compilan el mod Fabric 1.21.1 y la DLL PE32 con JDK21/Gradle8.11.1 y
MinGW14.2. Pasan 43 pruebas del núcleo, 72 de Minecraft y 160 de Python,
incluidos los callbacks nativos simulados; también pasan los fixtures de
instalación, actualización desde DLL445, desinstalación y diagnósticos.
Las pruebas nuevas cubren pulsación sostenida sin disparos repetidos,
referencias compartidas/ambiguas al administrador y partículas entre paquetes
sin modificar el prefijo de colisión.

RE4, su pad real y el driver Direct3D9 no se ejecutan en esta máquina Linux.
La interacción y la colisión de los ganados necesitan confirmación en Windows.
**Los golpes desde abajo mientras el jugador está elevado siguen pendientes:**
la elevación actual es virtual y no modifica la posición Y del actor nativo.
Esta versión no desactiva el daño ni promete corregir ese problema.
