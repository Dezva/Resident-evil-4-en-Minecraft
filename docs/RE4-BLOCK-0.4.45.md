# 0.4.45: altura, interacción y colisiones de ganados

Esta versión corrige el desarmado al subir una columna, mejora la interpolación
del salto, amplía el detector de ganados y añade F6 para saltar cinemáticas.
R envía la acción contextual de RE4, incluyendo recogida y confirmación.
El combate de Minecraft contra ganados sigue pendiente.

## Actualizar y jugar

Cierra ambos juegos, host y VIEW. Sustituye el paquete completo y deja solamente
**re4bridge-0.4.45.jar** en mods, además de Fabric API. Ejecuta
**UNINSTALL-RE4-CONTROLS → INSTALL-RE4-CONTROLS** del paquete nuevo. La DLL445 y el
host usan Runtime v3: no mezcles archivos de versiones anteriores. El
instalador conserva partidas; el desinstalador reconoce la DLL444 anterior.

Abre las partidas, START-RE4-CONTROLS y VIEW. Pulsa Preparar Minecraft y, cuando
termine, F9. Estos cambios también sirven con los dos juegos en ventana sin VIEW.

| Tecla | Función |
|---|---|
| Espacio / salto de Minecraft | Saltar, apoyarse en bloques y construir debajo |
| R | Acción contextual, recoger y confirmar en RE4; E sigue siendo inventario Minecraft |
| F6 | Pedir a RE4 que salte la cinemática/evento actual |
| Esc | Pausa de Minecraft; cerrar el menú conserva el puente |
| F5 | Alternar creativo y supervivencia |
| F8 | Terminar el puente y recuperar RE4 normal |

R/F6 funcionan con el menú de Minecraft cerrado. F6 usa la señal nativa de
cancelación de eventos, se rechaza fuera de películas/eventos y durante la
muerte. Sigue respetando las secuencias que RE4 no permite saltar. Las
peticiones son pulsos únicos y se cancelan al abrir un menú o detener el puente.
Durante películas que dejan de actualizar el pad, el callback de proyección
existente puede entregar el pulso con duración limitada. No cambia el foco.

## Altura y fluidez

El corte reportado alrededor de diez bloques procedía de consultar un techo
fuera del volumen de geometría local. Antes se presentaba erróneamente como
«salto de zona». Ahora el mapa cercano se consulta alrededor de la altura virtual
del jugador, manteniendo el radio de12000unidades y el presupuesto de1024
triángulos. Si una consulta de techo queda fuera del mapa actual, la subida
espera a una lectura válida; no desarma el puente ni supone libre esa zona.
La colocación en geometría desconocida continúa rechazándose.

Steve recibe su posición al final del tick, conservando la posición anterior
para interpolar. La vista nativa interpola entre los extremos consecutivos del
salto; antes cada paquete reiniciaba el suavizado desde una posición atrasada.
El suelo auxiliar sigue a la altura del terreno, no a la del salto. Se mantiene
el límite vertical de128bloques y el límite de64bloques construidos por sesión.

El salto mueve Steve y la cámara/pies virtuales en RE4. La Y del cuerpo nativo
de Leon y su animación continúan bajo control de RE4; esto todavía no permite
saltar las barreras nativas del escenario ni ofrece vuelo creativo libre.

## Ganados

El detector anterior rechazaba gestores con más de256slots y pools que cruzaban
regiones de memoria, y exigía un bit de dibujo para aplicar física. El detector
nuevo valida hasta4096slots/64MiB a través de regiones comprometidas, con un
máximo de128humanoides corregidos por callback. Comprueba también después del
movimiento nativo, usando el callback de proyección existente.

Se barren los bloques sólidos y se separan horizontalmente los actores que
quedan dentro de una colocación. Los fragmentos de rotura siguen siendo
exclusivamente visuales. Se revalidan identidad, flags, vida, rutina, armado,
generación y memoria antes de escribir X/Z. No se escriben Y, IA, vida ni
animaciones del enemigo. Esto aún no añade navegación alrededor de paredes,
colisión de ataques ni soporte para jefes/animales.

En el host, runtime_state incluye enemy_capacity/stride, enemy_scanned,
enemy_eligible, enemy_contacts/corrections y enemy_reason:

| Motivo | Significado |
|---|---|
| 0 | Gestor válido; detector activo |
| 1 | Puente/escena inactivos |
| 2 | Sin escena física válida/reciente o sin pase gráfico disponible |
| 3 | Cabecera ilegible |
| 4 | Layout/capacidad fuera del rango admitido |
| 5 | Pool ilegible o fuera del presupuesto de regiones |
| 6 | Firma del gestor no encontrada |

/re4bridge status muestra detector, slots revisados, ganados elegibles y
contadores R/F6. Una excepción local conserva su causa y altura en
.minecraft/logs/latest.log, en lugar de ocultarla bajo un aviso genérico.

## Mapeado y cambios de zona

El lector utiliza los gestores SAT/EAT que RE4 tiene cargados: no está fijado a
la zona inicial y puede leer el pueblo y otras áreas. Solo obtiene geometría de
colisión nativa; no reconstruye toda la geometría visual de árboles/decoración.
La transición continua entre zonas y la separación de construcciones por zona
siguen pendientes: una carga o cambio grande de coordenadas puede requerir
fijar otra referencia. No se garantiza aún recorrer toda la campaña sin rearmar.

## Prueba breve

1. Coloca una pared ante varios ganados y observa el host: detector0, slots y
   ganados positivos, contactos/correcciones al intentar atravesarla.
2. Ante un aviso de acción de RE4, pulsa R; debe aumentar acciones y ejecutar
   la interacción. E debe seguir abriendo el inventario Minecraft.
3. Activa una cinemática y pulsa F6. Comprueba el contador skips, retorno de
   cámara y conservación del puente; Esc debe continuar abriendo la pausa.
4. Construye una columna de más de12bloques saltando y colocando debajo.
   Abre/cierra Esc durante el salto; después baja y comprueba caída/daño.

Si persiste un fallo, conserva el registro alrededor del corte y latest.log.
No hace falta borrar las construcciones para probar esta actualización.

## Validación

Compilación Fabric/JDK21 y DLL PE32, pruebas TCP de R/F6, pausa y expiración,
salto/columna de24bloques con mapa móvil y espera en cobertura desconocida,
barridos repetidos de enemigos, pool de300slots, regiones de memoria y pulsos
nativos de cancelación durante pad inactivo. RE4 y un driver D3D9 real no se
pueden ejecutar en este entorno: interacción, cinemáticas y ganados necesitan
la prueba del paquete en Windows antes de considerar cerrado el combate.
