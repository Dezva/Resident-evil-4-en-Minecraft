# 0.4.51: combate cuerpo a cuerpo de Minecraft contra ganados

Con Minecraft enfocado, apunta al ganado y pulsa clic izquierdo. El puente usa
el objeto de la mano principal, el alcance de interacción de entidades y la
recarga de ataque de Minecraft. El golpe modifica la vida real del ganado en
RE4; no dispara el arma de León ni consume su munición.

## Cómo funciona

- Mano, herramientas y espadas usan el atributo real de daño del jugador.
  Sin efectos adicionales: mano1, espada de madera4, hierro6, diamante7 y hacha
  de hierro9 puntos al cargar completamente. La velocidad del objeto determina
  cuánto tarda en recuperarse el golpe; hacer clic rápido reduce su fuerza.
- Se usa el indicador de recarga configurado en Minecraft y su animación de mano.
- El alcance se comprueba en tres dimensiones: la altura de la columna cuenta.
  El objetivo es el primer ganado alcanzado por el rayo de la mirada. Un enemigo
  más cercano impide golpear al que está detrás.
- Bloques colocados y paredes nativas bloquean el ataque. Si el bloque está
  delante del ganado, el clic mantiene la minería normal. La geometría de combate
  se consulta por separado, en una zona más pequeña que el mapa de construcción.
  Si tampoco está disponible esa geometría, se muestra la espera y se aplaza el
  daño; no se autoriza un golpe a través de una pared desconocida.
- Una confirmación nativa consume el desgaste del objeto usado en supervivencia
  (espada1, herramienta2) y extenuación0.1. Creativo no desgasta el inventario.
  Fallos e impactos rechazados no gastan durabilidad ni dañan otro objeto al
  cambiar de ranura. Hay sonido de impacto fuerte/débil o de golpe sin daño.

La conversión inicial es50HP de RE4 por punto de daño de Minecraft. La recarga
pondera el daño con la fórmula vanilla `base*(0.2+0.8*carga²)`. Los impactos
comparten una ventana de daño de500ms por enemigo: repetir paquetes no repite
el golpe y un golpe más fuerte dentro de la ventana aplica solo la diferencia.
Al agotar HP, el módulo entrega al ganado a su rutina de muerte nativa.

Esta entrega cubre el ataque básico solicitado. No añade todavía flechas,
barrido, críticos, encantamientos de daño, retroceso ni defensa con escudo. El
volumen de selección es una caja corporal; no hay daño por huesos ni cabezas.
Se admiten los tipos humanoides de la lista existente; Ashley, mercaderes,
cadáveres y actores de escenas quedan fuera del daño.

## Instalar y comprobar

Cierra RE4, Minecraft, el host y VIEW. Extrae el paquete completo nuevo. Deja
únicamente `re4bridge-0.4.51.jar` y Fabric API en los mods de la instancia.
Ejecuta `UNINSTALL-RE4-CONTROLS.bat` y después `INSTALL-RE4-CONTROLS.bat` del
paquete nuevo. Se reconoce la DLL450 publicada por checksum. Abre los juegos,
el host nuevo y VIEW; prepara Minecraft y pulsa F9 como antes.

**DLL451, host y JAR deben actualizarse juntos.** El combate tiene un canal nuevo;
mezclar versiones impide preparar la sesión.

Prueba primero un ganado con mano vacía y después con espada, dejando recuperar
cada golpe. Repite fuera de alcance, detrás de un bloque y desde una columna.
Comprueba la durabilidad en supervivencia, su muerte y la continuidad de RE4.
El log incluye `combat_state`, `ack`, `reason`, HP antes/después, impactos y bajas.
Los motivos de la última solicitud son0impacto,2sin objetivo válido,3obstáculo,
4datos/identidad cambiados y5ventana de daño;1corresponde al inicio sin golpe.
`active=False` indica que no hay un catálogo actual utilizable para atacar.

Las compilaciones y pruebas automatizadas pasan en Linux con APIs simuladas.
La animación/reacción del ganado, la muerte y los eventos posteriores requieren
confirmación en RE4/Windows real. No se modifican máximos de HP, inventarios de
RE4 ni sus banderas de misión, ni se fabrican botines de Minecraft.
