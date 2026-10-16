# RE4 Bridge0.4.57 — profundidad y flechas3D

[Paquete completo Windows](re4bridge-0.4.57-windows.zip) ·
[Fuentes](re4bridge-0.4.57-source.zip) · [Checksums](SHA256SUMS.txt)

Incluye DLL457 y host/core/mod0.4.57. Requiere actualizar juntos los tres;
no sirve sustituir solamente el JAR de arrowfix1.

El usuario confirmó flechas visibles/daño en arrowfix1 y patada visible en0.4.56.
Esta versión prueba dibujo opaco con Z nativo, flechas como quads separados
del HUD y oculta las pausas por terreno desconocido. La oclusión real todavía
requiere comprobación visual. No amplía el alcance del terreno conocido.

Compilación y pruebas automáticas verificadas en Windows:62 casos core,98
Minecraft y200 Python distintos, fixtures C y PowerShell5.1 sobre archivos
sintéticos;37 casos también con el Python3.13 del lanzador. No se instala
ningún archivo en el juego desde Codex.

Lee [actualización, prueba breve y límites](../../docs/RE4-BLOCK-0.4.57.md).
Conserva offline, F8 y las copias de seguridad. Modelo/skin Minecraft de Leon
sigue pendiente.
