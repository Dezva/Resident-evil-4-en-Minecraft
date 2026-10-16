# 0.4.55 hostfix1 — inicio del host

Corrige dos constantes Python de identificación de los canales Runtime y Combat.
El paquete0.4.55 rechazaba su propia DLL455 como incompatible: el host esperaba
`0x52455452`/`0x52455446` en vez de `0x52453452`/`0x52453446`.

- [Parche pequeño para0.4.55](re4bridge-0.4.55-hostfix1.zip): solo los dos archivos
  Python y las instrucciones. Cerrar el host, extraer sobre su carpeta habitual
  (la que contiene START-RE4-CONTROLS.bat) y volver a abrirlo normalmente.
- [Paquete completo corregido](re4bridge-0.4.55-hostfix1-windows.zip).
- [SHA256SUMS](SHA256SUMS.txt).

No hace falta cerrar RE4 ni reinstalar su DLL para aplicar el parche del host.
El JAR0.4.55, DLL455, VIEW y demás archivos conservan exactamente sus bytes.
Las instrucciones de instalar la DLL del paquete completo solo corresponden a
una instalación nueva; este arreglo no requiere tocar Bin32.

Validación:33 tests de Runtime/Combat/control/protocolo/recuperación pasan en
Windows. Los fixtures nuevos usan firmas del contrato independientes de las
constantes del host y rechazan cabeceras incompatibles sin escribir bytes.
Las pruebas de aceptación fallan antes y pasan después de la corrección.

Arranque real con RE4 y DLL455: reconoce todos los canales, informa reacciones
nativas listas y alcanza READY en localhost28741. Se detuvo mediante la ruta
normal de KeyboardInterrupt antes de aceptar conexiones, siempre desarmado.
Esto valida el inicio del host, no sustituye la prueba de gameplay de0.4.55.
