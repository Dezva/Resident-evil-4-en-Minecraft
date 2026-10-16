# 0.4.57-impactfix2 — flechas y profundidad combinada

[Descargar Windows](re4bridge-0.4.57-impactfix2-windows.zip) ·
[Fuentes](re4bridge-0.4.57-impactfix2-source.zip) · [Checksums](SHA256SUMS.txt)

Actualiza DLL, host y mod juntos. Conserva los contratos DLL457/Combat4 y las
mecánicas anteriores. Amplía la cobertura de proyectiles con límites de tamaño,
consulta el impacto nativo antes de incrustar flechas en superficies destructibles
y permite continuar los fallos geométricos sin repetir daño. Reconoce matrices
combinadas de GPU y añade telemetría de lectura para verificar la profundidad.

23 pruebas Java,46 comprobaciones principales C/Python/paquetes y la instalación
sintética en PowerShell5.1 pasan. También se verificó con Python3.13 del lanzador.
No se ha instalado el parche en el juego ni se modificaron partidas/bio4.exe.

La superposición sigue confirmada en depthfix1. **Oclusión y resultados del
combate real de este parche pendientes de prueba offline**; no se declaran
resueltos por simulaciones. El alcance sigue limitado al terreno conocido.

[Instalar, recuperar y realizar pruebas concretas](../../docs/RE4-IMPACTFIX2.md).
Ante interrupciones usa F8 y cierra el host; conserva el paquete anterior.
