# Geometría compartida: resultado de la investigación0.4.34

## Lo comprobado con la exportación del usuario

La exportación completa del12 de octubre contiene430 triángulos:205 SAT y225
EAT;98 suelos,109 pendientes y223 paredes. Es una región local de radio20000
unidades nativas, aproximadamente20 bloques con la escala actual. No es todo
el mapa ni la malla de los árboles, hojas o texturas visibles.

La base `tools/re4_geometry.py` valida el informe, construye un BVH y permite:

- Obtener el impacto más cercano de un rayo sobre un triángulo, desde ambos
  lados, con punto, normal, propietario SAT/EAT y categoría de superficie.
- Consultar intersecciones exactas entre caja y triángulos mediante ejes
  separadores. Un bounding box de triángulo no se toma por un obstáculo sólido.
  Un pequeño inset permite distinguir apoyar un bloque de atravesar la superficie.
- Consultar suelo/pendientes con un rayo vertical y filtrar esas categorías.
- Rechazar informes incompletos y consultas fuera de la región exportada:
  ausencia de datos no significa espacio libre.

Con la posición del usuario, un rayo vertical desde2000 unidades más arriba
intersecta una pendiente SAT en Y=-454.6412477 frente a la posición
Y=-454.6429443: diferencia de0.0017 unidades nativas. La prueba demuestra que
el suelo exportado puede consultarse con precisión; no prueba todas las zonas.
También se verificaron ambas caras de una pared, impactos más cercanos en hojas
distintas del BVH, pendientes y cajas que están dentro del bounding box del
triángulo pero fuera de su superficie. Las consultas son offline: no escriben
memoria/IPC ni modifican el mundo de Minecraft o RE4.

## Integración física necesaria

El transform fijo de `ControllerMapping` debe aplicarse a jugador, bloques,
rayos y geometría; no debe recalibrarse moviendo bloques cada vez que Leon gira.
La geometría debe adquirirse en segundo plano con presupuesto y caché por
habitación/identidad de objetos, actualizarse para objetos móviles y caducar al
cambiar de habitación. Una exportación antigua no sirve como estado vivo.

El clic de construcción debe comparar el impacto del rayo de Minecraft con
el impacto RE4 y usar el más cercano válido. El punto y la normal RE4 permiten
elegir una celda de apoyo; su volumen completo debe comprobarse antes de colocar.
Esto exige adaptar la interacción de Minecraft: no basta con corregir la
posición de Steve ni con mover una plataforma de cristal bajo sus pies.

Los triángulos permiten detectar cruces de superficies. No describen por sí
solos el volumen interior de un sólido ni indican que un espacio sin cruces sea
construible. Se necesitan las reglas/atributos de colisión apropiados, una prueba
de interior donde exista una malla cerrada y rechazo conservador de casos desconocidos.
Convertir todo en cubos o losas pierde precisión en troncos, diagonales y bordes.
Una representación auxiliar en Minecraft puede ayudar a su física, pero la
colocación y los proyectiles deben consultar las superficies originales.

Para una flecha se prueba el segmento entre su posición anterior y siguiente,
no solo el punto final: evita atravesar una pared a gran velocidad. El impacto
debe conservar propietario/posición local para seguir un objeto móvil. El daño
a enemigos requiere además sus hitboxes y reglas de daño; no sale de SAT/EAT.
Esta versión no activa aún esas mecánicas.

## Ocultación visual: problema separado

El renderer actual convierte los bloques a vértices de pantalla (`XYZRHW`),
les asigna Z=0 y desactiva `ZENABLE`/`ZWRITEENABLE`. Por eso el escenario no puede
ocultarlos: no es un fallo de textura ni se arregla creando colisiones invisibles.

La solución requiere dibujar usando la profundidad y proyección compatibles
con el pase del escenario de RE4, antes de que se limpie/reutilice su profundidad
para otros pases o HUD. Hay que observar el depth-stencil activo, viewport,
proyección y momento de los clears en el dispositivo ya envuelto. No basta con
activar ZENABLE sobre los vértices Z=0 actuales. Una malla de colisión usada como
oclusor tampoco reproduce hojas, transparencias o decoración sin colisión.

Debe validarse en RE4/D3D9 real el pase, formato y conservación de profundidad.
No se introdujo un parche especulativo ni se recuperó el escaneo de dispositivos
retirado tras el crash0.4.13. La DLL434 conserva la ocultación HUD de433; el cambio nativo de esta versión
se limita a cachear la imagen durante una escritura IPC y marcar el build.
