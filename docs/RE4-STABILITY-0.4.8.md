# 0.4.8: estabilidad al alternar y combinar teclas

ZIP completo: DLL 0.4.8, mod/host 0.4.3, START-RE4-CONTROLS, VIEW-RE4,
instaladores y TRACE-RE4-CAMERA. Conserva tus mods actuales y Fabric API.

El usuario confirma que FP0.4.7 se activa y mejora, pero se altera al alternar
A/D o W/S y mezclar teclas. El registro de221 muestras confirma DLL0.4.7;
en varios cambios sin ratón aplicado_yaw permanece estable, pero la cámara
Global aún se separa de la mirada solicitada. Las lecturas no son un fotograma
atómico y no demuestran por sí solas la causa de cada salto visual.

0.4.8 corrige dos problemas de implementación:

- En el perfil observado, engancha la **entrada E9 validada de proyección**
  en lugar de solo seis sitios de llamada. Así cubre otros llamadores que
  pasan por esa entrada sin coincidir con la firma corta. Delega al cuerpo
  original comprobado, evitando recursión por el thunk modificado. El perfil
  estricto de llamadas/thunks/destinos de0.4.7 sigue siendo obligatorio.
- Lectores de controles hacen hasta tres intentos de obtener una muestra
  completa. Si el host está publicando, pueden reutilizar la última completa
  aún vigente; no reinician la orientación ni vuelven a aplicar todo el giro
  acumulado por una lectura rota. Nunca renuevan su timestamp. Stop/desarmado
  tienen prioridad y el TTL original de500ms limita la reutilización.

A+D y W+S simultáneos cancelan su eje, como antes. La mirada depende del ratón,
no de esas teclas; diagonales mantienen velocidad nativa de avance. No añade
retardo deliberado ni interpolación del movimiento. Tampoco añade salto,
sprint, agachado ni cambios de colisión/posiciones/guardados/inventario.
La efectividad visual y de locomoción requiere la prueba real del usuario.

## Actualización desde 0.4.7

1. Cierra RE4 y el host. Ejecuta **UNINSTALL-RE4-CONTROLS.bat de0.4.7**.
2. Extrae este ZIP en una carpeta nueva y ejecuta su **INSTALL-RE4-CONTROLS.bat**.
   Verifica el mensaje de copia de partidas. Conserva los mods0.4.3.
3. Abre RE4 con partida y Minecraft. Usa el **START-RE4-CONTROLS.bat nuevo**
   con los mismos permisos de RE4. VIEW-RE4 también viene en esta carpeta.
4. `/re4bridge controller`, `/re4bridge firstperson` si estaba apagado,
   `/re4bridge arm` y cierra el chat. F8 mantiene el retorno a RE4 normal.

## Prueba

Sin mover el ratón, alterna A/D y luego W/S; pulsa A+D y W+S a la vez,
prueba W+A/D y cambia rápidamente entre diagonales. Al soltar, la vista no
debería girar por las teclas. Después repite moviendo el ratón suavemente.
Comprueba F8, abrir chat/perder foco y detener el host: deben desarmar y exigir
un arm nuevo. El fallback de muestras no debe mantener controles caducados.

Si todavía hay sacudidas, envía un TRACE-RE4-CAMERA del ZIP nuevo y un vídeo
corto. Indica si giraba la mirada o temblaba la posición mientras el ratón
estaba inmóvil. Para volver atrás, cierra RE4, usa el UNINSTALL nuevo y el
INSTALL0.4.7. Las partidas y copias se conservan.

Validación Linux: tres suites C y32 pruebas Python pasan, con límites de TTL,
desarmado/stop prioritarios y wrap de reloj para caché. Una prueba compila el
lector nativo real e interrumpe la publicación entre copia y confirmación
de secuencia para comprobar reintento/caché. Instalación/diagnóstico
se verifican con fixtures. No se ejecutó RE4 Windows ni se garantiza aquí la
ausencia de sacudidas en animaciones o estados especiales del personaje.
