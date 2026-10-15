# 0.4.52: impactos nativos, partículas y estabilidad del host

Clic izquierdo mantiene el daño de la herramienta o espada, alcance 3D y recarga
vanilla. Esta versión sustituye la escritura directa de HP y de rutinas por el
recorrido de impactos de RE4.

## Reacciones y objetivos

RE4 comprueba el rayo contra sus paredes y partes animadas del cuerpo. Se entrega
la parte realmente alcanzada al mensaje nativo de disparo: la cabeza, rodillas,
piernas y cuerpo conservan las reacciones que decide el propio juego, incluidas
sus condiciones de aturdimiento, resistencia, acciones contextuales y muerte.
No se fuerza una animación ni se escribe `Routine0::Die`. El cálculo Minecraft
se aplica únicamente al mensaje del puente cuando RE4 tramita ese daño; los
disparos y el daño ordinario de RE4 mantienen sus valores y sus banderas.

Se añaden cuervos y los tipos nativos Box, Rack, Window y Barrel al catálogo de
objetivos, siempre que estén activos, tengan HP válido y sean alcanzables por el
rayo nativo. Sus destrucciones y drops siguen a cargo de RE4. Esto no convierte
cualquier decorado, puerta u objeto del escenario en destruible.

Los bloques colocados también bloquean el golpe, incluida una pared que quede
entre la caja aproximada de selección y el impacto real sobre el modelo.

El desgaste, los sonidos y las partículas esperan a que RE4 confirme una bajada
de HP. Se utilizan las partículas originales de Minecraft: indicadores de daño
sobre seres vivos y efecto de espada con el ataque cargado. No aparecen corazones
en las cajas. El efecto de espada es visual; no añade daño de barrido a objetivos
secundarios. Se dibujan únicamente estas partículas en la captura transparente,
con interpolación de Minecraft y el campo de visión de RE4. Su composición en
el overlay aún no usa la profundidad nativa para ocultarlas detrás de paredes.

## Rendimiento y errores

Se elimina la extracción continua del segundo mapa de combate. El mapa de
construcción sigue funcionando; la consulta de combate se hace en RE4 al atacar.
La consola muestra contadores y estados, sin volcar miles de triángulos ni el
catálogo completo. No se ha medido un aumento de FPS en Windows.

Los fallos de sesión desarman el control y permiten reconectar el host. Los
errores y sus trazas se guardan en `logs/re4bridge-host.log`, con rotación limitada
a tres archivos de aproximadamente 2 MiB cada uno. El log recibido no contenía
una excepción: estos cambios no identifican todavía una causa única para todas
las interrupciones observadas.

## Instalar y comprobar

1. Cierra RE4, Minecraft, el host y VIEW. Extrae el paquete nuevo completo.
2. Ejecuta `UNINSTALL-RE4-CONTROLS.bat` y después `INSTALL-RE4-CONTROLS.bat`
   del paquete nuevo. El desinstalador reconoce la DLL 0.4.51 publicada.
3. Deja únicamente `re4bridge-0.4.52.jar` y Fabric API en los mods de la instancia.
4. Abre RE4, el host y VIEW. El host debe mostrar `Reacciones nativas: listas`.
   Prepara Minecraft y pulsa F9 como antes.

**DLL 452, host y JAR deben actualizarse juntos.** El canal Combat v2 tiene 5296
bytes y espera una confirmación asíncrona. No mezcles componentes de 0.4.51.

Comprueba primero cuerpo, cabeza y piernas de un ganado; después un cuervo y una
caja dentro de alcance. Deja recuperar el golpe, prueba paredes/bloques y observa
la animación al morir. El perfil del ejecutable y las tres funciones fueron
confirmados por el diagnóstico de solo lectura del usuario. Las pruebas de
compilación, protocolo y callbacks pasan en Linux con APIs simuladas; las nuevas
animaciones, destrucciones y partículas aún requieren comprobación visual en
RE4/Windows real.

Si vuelve a interrumpirse, conserva `logs/re4bridge-host.log` y sus últimas líneas.
Los estados incluyen `pending`, `queued`, `expired`, `hit_part`, HP antes/después,
impactos y bajas. Motivos: 0 impacto confirmado, 1 inicial, 2 rayo sin objetivo
válido, 3 bloque colocado, 4 identidad/control cambiado, 5 ventana de daño,
6 perfil no reconocido, 7 mensaje ocupado, 8 caducado y 9 esperando a RE4.
