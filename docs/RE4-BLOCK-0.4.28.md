# 0.4.28 — rendimiento con el puente inactivo

El usuario confirma que RE4 va lento incluso abierto solo, con Minecraft, host
y VIEW cerrados. Su informe reconoce DLL/build **0.4.27**, dibujo desactivado en
la instantánea, cero fallos de dibujo/restauración y 174 fallos acumulados de
Present. Esos contadores no contienen tiempos ni el código del último fallo de
Present; no permiten atribuir la lentitud a esos fallos o medir FPS. El vídeo
incluye también una parte con primera persona y Minecraft abiertos.

La revisión anterior redujo cargas de texturas, pero no eliminó trabajo nativo
que continuaba antes de conectar/armar el puente. Esta versión corrige ese camino.
**El JAR Minecraft sigue siendo 0.4.26 y sus bytes no cambian.**

## Cambios

- La entrada E9 de proyección sirve a más llamadas del juego que los seis sitios
  inicialmente reconocidos. Ahora, con controles desarmados o detenidos, llama
  directamente a la función original. Solo restaura una cámara/modelo prestado
  si lo había, e invalida una vez el diagnóstico anterior. No vuelve a consultar
  jugador/FOV/yaw, leer controles, comprobar barreras ni consultar F8 en cada
  proyección inactiva. Los contadores de diagnóstico de cámara pasan a medir
  trabajo activo; no cuentan las proyecciones que solo se reenvían.
- La restauración de primera persona retorna de inmediato si no hay cámara ni
  modelo que restaurar. La cámara desarmada y el mando que no había cambiado
  orientación evitan consultas innecesarias al jugador.
- Las 112 llamadas D3D9 de paso normal conservan la protección de vida de la
  interfaz sin añadir AddRef/Release **nativos** en cada llamada. Si un callback
  libera referencias durante una llamada, sus liberaciones nativas se difieren
  hasta que retorna la llamada exterior. Se mantienen serialización, argumentos,
  resultados, LastError, QueryInterface y propiedad de las referencias del cliente.
  El informe del usuario usa flags0x44, que incluyen el modo D3D multihilo.
- Dibujo inactivo comprueba solo una cabecera coherente de 32 bytes. Los buffers
  grandes de escena/vértices/HUD están en funciones separadas que se llaman solo
  cuando se solicita el dibujo correspondiente. Se conservan las cachés de
  texturas de 0.4.27 y las comprobaciones completas antes de dibujar.
- Un fallo de Present guarda ahora su HRESULT en `cube.last_hresult`, para que
  una captura futura pueda informar el error; el campo sigue representando el
  último resultado gráfico registrado y un dibujo posterior puede actualizarlo.

No se modifican ajustes gráficos de RE4, archivos de partidas ni el modo de
Minecraft. No se añaden nuevos hooks, mapas o elementos del HUD.

## Actualizar y comprobar

1. Cierra ambos juegos, host y VIEW. Conserva el paquete anterior.
2. Ejecuta **UNINSTALL-RE4-CONTROLS del paquete anterior**.
3. Extrae 0.4.28 completo y ejecuta su **INSTALL-RE4-CONTROLS**. Puedes reutilizar
   la carpeta después de desinstalar y retirar sus archivos anteriores.
4. Conserva **re4bridge-0.4.26.jar** y Fabric API en Minecraft. El ZIP incluye
   esos mismos JAR, para instalaciones completas; deja una sola copia del puente.
5. **Primera prueba: abre únicamente RE4**, entra en la misma partida/zona y
   camina con sus controles normales. Minecraft, START y VIEW permanecen cerrados.
6. Si el rendimiento mejora, abre después Minecraft y **START-RE4-CONTROLS de
   0.4.28** y compara con el puente armado. No hace falta construir otro mapa.

DLL SHA256: `374e92fc07b52abc1ed894897374869f4d69955b9d50ab0f80b1e9ae0ff93187`.
Build428; control v2/80, escena v6/96352 y HUD v1/32792 conservados.
Instaladores/VIEW sin cambios. Los comandos y límites están en las guías
[0.4.26](RE4-BLOCK-0.4.26.md) y [0.4.27](RE4-BLOCK-0.4.27.md).

Si RE4 solo sigue lento, la siguiente comparación es cerrarlo, desinstalar la
DLL del puente y abrir la misma zona. Eso distingue el coste residual de la DLL
de un problema que también aparece sin ella. Para retomar el puente, cierra RE4
y ejecuta otra vez INSTALL. Una nueva CAPTURE-RE4-GRAPHICS puede aportar el HRESULT si los
fallos de Present continúan; no es necesaria para la primera prueba.

## Evidencia y límites

Comparación con el código publicado de 0.4.27 en la misma API simulada, 10000
proyecciones desarmadas después de la restauración final:

| Operación adicional | 0.4.27 | 0.4.28 |
| --- | ---: | ---: |
| Consultas de legibilidad/escritura de cámara | 40000 | 0 |
| Consultas al jugador | 20000 | 0 |
| Lecturas de control | 10000 | 0 |
| Consultas a F8 | 10000 | 0 |
| Comprobaciones de colisión | 10000 | 0 |
| Actualizaciones de diagnóstico | 10000 | 0 |

Las llamadas originales continúan ejecutándose. La prueba activa de cámara,
F8, restauración y cambios rápidos sigue pasando. 10000 dibujos inactivos no
leen la escena completa ni crean recursos; 10000 llamadas de HUD inactivo no
leen sus píxeles. Las 112 llamadas gráficas de paso hacen cero pares nativos
extra de referencias. Se prueban callbacks anidados que liberan una o dos
referencias finales durante Reset, sin liberar prematuramente el dispositivo.

Pasan 104 pruebas Python, 11 fixtures C también con ASan/UBSan, cuatro suites C
portátiles y fixtures PowerShell del instalador/diagnóstico. Se verifica el JAR
idéntico al publicado y se compila la DLL Win32. **Esto mide operaciones del
código, no FPS reales ni tiempo de GPU. La mejora en RE4/Windows requiere la
primera prueba descrita arriba.**

Para recuperar 0.4.27: cierra todo, desinstala con 0.4.28 e instala con 0.4.27;
usa su host. El JAR se conserva. F8 mantiene el retorno normal durante el uso.
