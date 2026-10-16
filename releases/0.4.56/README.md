# 0.4.56 — arco/ballesta y acciones nativas visibles

[Descargar paquete Windows (2.93 MB)](https://github.com/Dezva/Resident-evil-4-en-Minecraft/raw/refs/heads/codex/re4bridge-0.4.56-ranged-combat/releases/0.4.56/re4bridge-0.4.56-windows.zip)
· [Código fuente](re4bridge-0.4.56-source.zip)
· [SHA256](SHA256SUMS.txt)

Base0.4.55-hostfix2 confirmada por el usuario: estabilidad, trampas y bombas.
Nueva versión de prueba compilada en Windows: flechas de arco/ballesta con
daño nativo y modelos vanilla; cámara/cuerpo de Leon durante la acción nativa
contra enemigos. Pendiente de prueba real de gameplay/visibilidad de la patada.

**Actualiza los tres componentes:** DLL456, JAR0.4.56 y host de este ZIP.
Cierra ambos juegos, host y VIEW. Extrae en una carpeta nueva, ejecuta primero
UNINSTALL-RE4-CONTROLS y después INSTALL-RE4-CONTROLS del paquete; instala el
JAR dejando una sola versión. Usa START-RE4-CONTROLS/VIEW de la carpeta nueva.
El instalador verifica la copia de partidas y rechaza DLL ajena. No requiere
restaurar permisos de administrador ni reemplazar bio4.exe. El agente no ha
instalado estos cambios en el juego.

[Guía y cinco pasos concretos de prueba](../../docs/RE4-BLOCK-0.4.56.md).
Empieza con arco y ballesta contra un Ganado a3–6 bloques en una zona abierta,
comprueba una pared colocada y después una patada contextual.

Conserva carga, munición, durabilidad, trayectoria, crítico y Poder1.21.1.
Límites: terreno conocido local, overlay sin profundidad nativa, un impacto por
flecha; Perforación, fuego, Empuje, pociones/espectral y cohetes contra actores
nativos pendientes. La skin/modelo Minecraft en la animación de Leon todavía
no está implementada. No se presenta como integración completa de encantamientos.

Pruebas: core58; Minecraft92 de la suite completa más9 casos finales de combate
(93 distintos);191 Python/C iniciales con un diagnóstico corregido y32 casos
afectados repetidos correctamente;35 casos host con Python3.13 y prueba adicional
del paquete sobre bytecode0.4.55 (22 casos dentro de un intérprete nuevo).
Instaladores probados en carpetas y partidas sintéticas. [Validación detallada](../../docs/VALIDATION.md).

DLL SHA256: `5b25e60dd954c1b5e7480418f406cce8ecd43ee8775ada587da4ecae3ee88e6d`.
Windows ZIP SHA256: `50a060c2365e28b3314689857b26e8f0442f23d95a11693ce2f5642f41dba1bf`.
Fuente ZIP SHA256: `354feda7879e654a161dfff3f1b16fd48e99d3cec22bf390b5e4cdbd1c9e9391`.
