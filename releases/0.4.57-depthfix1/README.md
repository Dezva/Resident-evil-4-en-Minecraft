# 0.4.57-depthfix1 — interrupción con bloques

[Descargar Windows](re4bridge-0.4.57-depthfix1-windows.zip) ·
[Fuentes](re4bridge-0.4.57-depthfix1-source.zip) · [Checksums](SHA256SUMS.txt)

Parche de selección de pase/proyección y reintentos del renderer0.4.57.
Mantiene el JAR, core, host de controles y contrato IPC457; cambia la DLL
propia y los lectores de diagnóstico. No modifica partidas ni bio4.exe.

33 casos nativos/profundidad, integración real C de200 frames,42 casos con
Python3.13 y pruebas PowerShell sobre archivos sintéticos pasan.
La protección de colisión500ms y F8 permanecen activas.

[Actualizar y probar10+10 segundos](../../docs/RE4-DEPTHFIX1.md).
No se instala automáticamente. Estabilidad/oclusión reales pendientes de
verificación offline; no se da por resuelto visualmente sin esa prueba.
