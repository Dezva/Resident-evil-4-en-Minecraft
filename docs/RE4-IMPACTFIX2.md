# 0.4.57-impactfix2 — proyectiles y profundidad combinada

Parche de prueba. La versión anterior sigue superponiendo bloques sobre árboles,
según las capturas del usuario. Las flechas ya son visibles y hacen daño, pero
fallan contra algunos blancos elevados, cajas, ventanas y trampas. Esta versión
corrige rutas reproducidas con pruebas; todavía requiere verificación jugando.

## Actualizar y recuperar

Incluye host, DLL y mod nuevos. Conserva los contratos DLL457/Combat4/Runtime5,
pero **actualiza los tres componentes juntos** para usar la cobertura ampliada
y el tratamiento nuevo de impactos fallidos.

1. Cierra RE4, Minecraft y el host. Extrae el ZIP en una carpeta nueva.
2. Ejecuta UNINSTALL-RE4-CONTROLS.bat del paquete nuevo y después
   INSTALL-RE4-CONTROLS.bat. Solo retiran DLL propias conocidas; el instalador
   rechaza sobrescribir y verifica una copia de partidas/configuración.
3. Sustituye solo el JAR re4bridge anterior de `.minecraft/mods` por
   `re4bridge-0.4.57-impactfix2.jar`. Conserva Fabric API.
4. Abre los juegos offline y usa START-RE4-CONTROLS/VIEW del paquete nuevo.

Codex no instala estos cambios en el juego automáticamente. Es una DLL nativa
experimental: ante una interrupción pulsa F8 y cierra el host. Para volver,
cierra los juegos, desinstala únicamente esta DLL con el paquete nuevo y
reinstala el paquete depthfix1 guardado, incluidos su host/JAR. No se requiere
activar administrador ni se sustituye bio4.exe.

## Prueba breve

- Desde el suelo, dispara un arco y una ballesta al torso de un Ganado en una
  escalera o plataforma. Repite con una pared sólida interpuesta: no debe recibir
  daño a través de ella. Observa si las flechas llegan o desaparecen antes.
- Dispara al centro de una caja intacta y de una ventana rompible. Comprueba la
  rotura y el contenido habitual de la caja; después prueba un golpe normal.
- Apunta al mecanismo de una trampa y a una bomba desde dos ángulos. Un disparo
  claramente desviado debe fallar, sin activar otra trampa cercana.
- Coloca dos bloques junto a un árbol grueso y da la vuelta: comprueba la parte
  totalmente oculta y la parcialmente visible. La oclusión **no se considera
  resuelta** hasta confirmar este resultado y la nueva telemetría real.

## Cambios y límites

La lectura local encontró 1081 triángulos a12000 unidades y2985 a32000; antes,
superar1024 descartaba todo el mapa y pausaba las flechas. Ahora se admiten4096,
con radio deseado32000 y presupuesto JSON320000 bytes. Si no cabe, se reduce el
volumen conocido conservando todos sus triángulos. Nunca se publica una malla
truncada como completa. Fuera de la cobertura aún se pausa/oculta la flecha;
no se promete alcance vanilla ilimitado ni escenarios sin límite de geometría.
Se conserva un worker, refresco cada0.5s y caducidad1s.

El impacto de flecha consulta el rayo nativo antes del SAT/EAT de construcción,
que puede contener la superficie de la propia caja o ventana. Los bloques
colocados por el jugador siguen bloqueándolo. El rayo nativo elige actor y parte
reales dentro del catálogo validado; las cajas aproximadas son candidatos.
Un fallo geométrico permite avanzar un paso con colisión de terreno y continuar,
sin consumir daño ni repetir indefinidamente la consulta del mismo segmento.
Un impacto enviado, timeout o rechazo restante retira la flecha; se conservan
confirmación nativa, identidad, F8, generación, TTL y prevención de repeticiones.

La profundidad anterior buscaba una proyección aislada. El ejecutable local usa
shaders con matriz combinada modelo/vista/proyección en c0..c3. Se reconoce solo
su cálculo lineal de posición, mediante getters COM acotados, y se recuperan
los coeficientes Z/W de esa matriz; source4 identifica esta ruta. No se cambian
shaders, superficies ni planos del juego. Los programas no reconocidos conservan
la ruta anterior. Un canal opcional de128 bytes, solo de telemetría, publica la
matriz, el resultado y la antigüedad para leerlos localmente sin scripts manuales.
La prueba sintética no demuestra oclusión en el controlador real ni descarta
otros problemas con los pases del escenario.

Se mantienen salto, construcción, HUD, colisiones, combate previo, trampas y
patada visible. El modelo Minecraft sobre Leon continúa pendiente.
