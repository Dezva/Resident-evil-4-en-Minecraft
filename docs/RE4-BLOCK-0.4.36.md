# 0.4.36: proyección y colocación corregidas

El registro del usuario contiene `depth_reason: proyeccion no validada`,
`depth_draws: 0` y cientos de intentos. En0.4.35 se omitían las caras del bloque
al rechazar la proyección. La selección negra es una capa aparte: puede indicar
un bloque real o una celda candidata de construcción y no garantiza que una
colocación haya sido confirmada.

La captura posterior identifica el motivo exacto del rechazo: matriz GX con
frustum4:3, FOV50, near100/far≈1e6, pero viewport1366×768. D3D GetTransform
retorna identity. Se valida la proporción propia de GX y se usa su focalX
real para bloques y contorno, sin exigir que el frustum tenga la misma proporción
que la ventana. No se cambian la cámara ni los planos del juego.

## Actualizar y probar

1. Cierra juegos, host y VIEW. Desinstala la DLL anterior e instala la de este
   paquete con los BAT habituales. Sustituye el mod anterior por
   `mods/re4bridge-0.4.36.jar`, conserva Fabric API y usa el host/VIEW nuevos.
2. Abre RE4 con partida cargada y Minecraft en mundo local creativo. Ambos en
   ventana, sin minimizar; chat e inventario cerrados.
3. Abre START-RE4-CONTROLS y VIEW. **Preparar Minecraft → esperar confirmación → F9**.
4. Coloca varios bloques nuevos, cambia de ranura y rompe uno. Las colocaciones
   deben aparecer con sus texturas y la retirada actualizarse. F8 vuelve a RE4 normal.

Cuando la proyección nativa no se valida, se usa automáticamente el dibujo de
compatibilidad sin profundidad. Esto recupera el comportamiento visual previo
sin pedir otro comando. Ese modo puede seguir superponiendo bloques a árboles:
**la oclusión corregida necesita comprobarse en el juego real**.
La profundidad continúa funcionando en los pases con proyección validada.

## Colocación que desaparece y selección elevada

Se corrige otra incoherencia de35: al apartar una celda del suelo o de un muro,
el hit del paquete seguía apuntando al suelo original. El servidor integrado
1.21.1 rechaza el paquete si cualquier coordenada del hit dista1.0000001 o más
del centro de la celda. La predicción del cliente puede parecer un bloque que
luego desaparece. El nuevo hit queda dentro de la celda real; se conserva la
cara de la superficie nativa y se mantienen las comprobaciones de geometría.
No se desactiva ni se parchea la validación del servidor.

La búsqueda propone como máximo una celda adyacente, evita celdas no reemplazables
y comprueba el alcance real. Un contorno en el suelo puede ser la propuesta de
colocación, no un bloque construido. La cuadrícula aún puede quedar algo elevada
sobre una pendiente: no existe una copia del terreno de RE4 en losas de Minecraft
ni se mueve una construcción existente para ajustarla al suelo.

Para comprobar gráficos, no minimices ninguno de los juegos. RE4 minimizado
puede suspender los pases de dibujo aunque el host continúe recibiendo estado.

## Diagnóstico sin escribir comandos

Después de intentar colocar un bloque, ejecuta **CAPTURE-RE4-GRAPHICS.bat**
y revisa el JSON cuya ruta imprime. Conserva las matrices de los últimos intentos
aunque abrir la consola saque el foco del juego. La sección está en `capture.depth`:

- `depth_projection` y `depth_gx_projection`: matrices de16floats necesarias
  para identificar la representación que utiliza este RE4.
- `depth_fov`, `depth_compare` y `depth_source`: cámara y pase registrados.
- `depth_draws`: dibujos con profundidad nativa validada.
- `depth_fallback_draws`: dibujos de compatibilidad, contados por separado.

La captura36 ya no abre la memoria del proceso: lee directamente los mappings
de la DLL y evita el OpenProcess de35 que produjo Windows5. Si el propio
mapping rechaza el acceso, utiliza los mismos permisos que RE4.

El informe también conserva `capture.cube.draws`, fallos, restauraciones y razón
de rechazo. La captura es de solo lectura y no activa ningún dibujo ni control.
Si las matrices aún no están validadas, no se inventan planos ni se relajan los
controles de generación. La colocación sigue utilizando las superficies SAT/EAT
integradas en35; se corrige la coherencia del hit sin omitir colisiones.

La ruta anterior al HUD también reconoce Clear(Z) con un único rectángulo
que cubra el viewport, además de la forma sin rectángulos. No usa los clears
parciales como si fuesen el final del mundo.

## Validación y límites

Fixture con proyección inválida y textura Minecraft: dibujo de compatibilidad,
Z/ZWRITE desactivados y restauración completa de estados/referencias. Se comprueba
que la compatibilidad espera hasta EndScene, que el pase anterior al clear conserva
la profundidad validada y que proyección/buffer ausente no se confunden con éxito
nativo. F8, TTL, generación incorrecta, escena vacía y fallo de estado siguen
rechazando el dibujo. Pruebas C con ASan/UBSan, Python y build/tests de Fabric.
El resultado visual en el equipo del usuario necesita una prueba con RE4 real.
