# 0.4.56-arrowfix1 — parche del mod para arco/ballesta

[Descargar parche del mod (232 KB)](https://github.com/Dezva/Resident-evil-4-en-Minecraft/raw/refs/heads/codex/re4bridge-arrowfix1/releases/0.4.56-arrowfix1/re4bridge-0.4.56-arrowfix1-mod.zip)
· [Paquete completo (2.93 MB)](https://github.com/Dezva/Resident-evil-4-en-Minecraft/raw/refs/heads/codex/re4bridge-arrowfix1/releases/0.4.56-arrowfix1/re4bridge-0.4.56-arrowfix1-windows.zip)
· [SHA256](SHA256SUMS.txt)

Si ya tienes0.4.56: **cierra Minecraft y sustituye únicamente el JAR anterior**
por `mods/re4bridge-0.4.56-arrowfix1.jar`. Deja una sola versión del mod.
Conserva DLL456 y host0.4.56; no ejecutes instaladores/desinstaladores de DLL.
El agente no modificó la instalación real del juego ni las partidas.

Se corrige el registro de flechas nuevas: Minecraft incrementa su edad a1
antes del primer tick, mientras0.4.56 exigía edad0. Ese fallo impedía tanto
el daño como la inclusión de flechas en el overlay. La decisión de registro
ahora ocurre una sola vez, conservando propietario/dimensión/sesión.

La patada de Leon está confirmada por el usuario y no cambia en este parche.
Adaptar el modelo/skin del jugador Minecraft a Leon sigue pendiente.

13 pruebas Minecraft afectadas pasan, incluidos ambos dispatchers reales de
tick y el hook de flecha; remapeo y access widener correctos. Comparación de ZIP:
solo cambia JAR/README, DLL/host/core anidado idénticos. Nuevos eventos acotados
en latest.log permiten revisar registro, pausa, overlay y resultado sin pedir
scripts manuales. La visibilidad y el daño del parche aún requieren jugar.

Prueba en partida offline: Ganado a3–5 bloques, arco tensado y ballesta; observa
flecha y reacción. Repite con una pared colocada delante: debe bloquear el daño.
[Instrucciones y límites](../../docs/RE4-ARROWFIX1.md).

Parche SHA256: `1d516bc72edbecf209d6b493f4eaac44f201c31b2c30be1f11942b447aaaaa55`.
Completo SHA256: `240328ce82e9d3fbf6f63b57f0a4411f3b2a7717db8d97397453bb0d382751df`.
