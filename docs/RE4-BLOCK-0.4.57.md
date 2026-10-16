# 0.4.57 — profundidad y flechas como geometría del mundo

Versión de prueba; la oclusión en el controlador real de RE4 todavía requiere
comprobación visual. El usuario confirmó que arrowfix1 muestra flechas y hace
daño. Se mantienen combate, trampas, colisiones y patada visible.

## Actualizar

Este paquete contiene DLL457, host/core/mod0.4.57. No mezclar con el host/JAR
anterior. No requiere volver a activar administrador.

1. Cierra RE4, Minecraft y el host. Extrae el ZIP en una carpeta nueva.
2. Con los scripts del paquete nuevo, ejecuta UNINSTALL-RE4-CONTROLS.bat y luego
   INSTALL-RE4-CONTROLS.bat. El desinstalador solo acepta hashes propios conocidos;
   el instalador rechaza sobrescribir una DLL y verifica copia de partidas/config.
   No reemplaza bio4.exe. Conserva la carpeta y las copias de seguridad anteriores.
3. En `.minecraft/mods`, sustituye únicamente el JAR re4bridge anterior por
   `re4bridge-0.4.57.jar`; conserva Fabric API. Usa START-RE4-CONTROLS y VIEW de
   este paquete, con el procedimiento habitual de Preparar Minecraft/F9, offline.

Codex no ha ejecutado estos pasos contra la instalación del usuario.

## Prueba breve

Coloca dos bloques delante de un árbol grueso o una esquina de piedra.
Da la vuelta hasta que el obstáculo tape uno de ellos: el bloque completamente
oculto debe desaparecer y el parcialmente oculto debe mostrar solo su parte
expuesta. Acércate de nuevo para comprobar que sigue colocado y colisionando.
Dispara una flecha hacia el suelo cerca del obstáculo y mírala desde detrás.
Comprueba después un impacto de arco y otro de ballesta contra un Ganado.
F8 debe detener el puente; conserva la prueba en una partida offline.

La flecha que llega al límite del terreno conocido se oculta mientras espera
cobertura. No representa una colisión ni se deja volar por geometría desconocida.
El alcance físico verificable sigue limitado por el mapa local; esta versión
no implementa exportación de terreno siguiendo cada proyectil ni promete
alcance vanilla ilimitado.

## Cambio técnico

Los modelos vanilla de las flechas se capturan como quads texturados, separados
del HUD. RE4 los dibuja con Z y prueba alfa durante el mismo pase 3D que los
bloques; mano/inventario/hotbar permanecen en la capa de interfaz. Máximo16
flechas,128 quads y atlas128×128 por sesión de render; las texturas se leen del
Minecraft instalado y no se redistribuyen. Texturas de resource packs se
muestrean a64×64 por celda; no se garantiza fidelidad de packs de alta resolución.

La DLL intenta dibujar después del primer draw opaco válido en el backbuffer,
con restauración íntegra de estados. Consulta el estado real de Z para incluir
cambios realizados mediante state blocks. Prefiere una proyección D3D válida
en las constantes de GPU; conserva GX validado como alternativa. Reinicia bias
de profundidad durante el dibujo propio. El modo depth activado no puede
convertirse silenciosamente en un overlay sin Z. La comparación y el momento
del pase necesitan validación visual en cada escenario/controlador.

Telemetría privada: `D:\logs\re4-occlusion-20261016`. La muestra anterior
registró stage0/EndScene, source2/GX, near100/far≈1e6 y cero fallos de restauración;
no demuestra que el contenido del buffer fuera el del escenario.
La nueva DLL publica stage2 durante la inserción opaca, source3 cuando usa una
proyección encontrada en constantes GPU y contadores separados de quads.

Pendientes: skin/modelo Minecraft durante la patada de Leon, efectos especiales
completos de proyectiles, alcance fuera del mapa conocido. El contorno negro de
selección sigue perteneciendo a la interfaz y no demuestra visibilidad del bloque.
