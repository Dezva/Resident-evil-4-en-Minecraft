# 0.4.55 hostfix2 — caché Python y arranque habitual

Sustituye a hostfix1. El ZIP anterior podía conservar la fecha y el tamaño
de los fuentes0.4.55 y hacer que Python reutilizara un `.pyc` antiguo con las
firmas incorrectas. En Python3.13 del usuario se reprodujo esa situación:
el archivo `.py` era correcto, pero importarlo seguía cargando el valor antiguo.

- [Parche pequeño](re4bridge-0.4.55-hostfix2.zip): cerrar solo el host y extraer
  sobre la carpeta que contiene START-RE4-CONTROLS.bat. Volver a abrir ese BAT.
- [Paquete completo corregido](re4bridge-0.4.55-hostfix2-windows.zip).
- [SHA256SUMS](SHA256SUMS.txt).

Los dos fuentes cambian de tamaño y los ZIP usan una fecha nueva para las
entradas corregidas. Python invalida las cachés originales al importar; no se
necesita borrar carpetas ni reinstalar Python. Una incompatibilidad nueva
muestra magic/versión/build/PID en el error para facilitar el diagnóstico.

DLL455/JAR0.4.55 y los demás archivos del paquete completo conservan sus bytes.
No se requiere reinstalar DLL/JAR, cerrar RE4 ni cambiar sus permisos para este
parche del host. Las instrucciones de instalación de DLL del paquete completo
solo corresponden a instalaciones nuevas.

34 pruebas pasan con Python3.13 real del usuario. La prueba del paquete genera
caché válida del fuente0.4.55, extrae el parche conservando fechas ZIP e inicia
otro intérprete: ambas firmas correctas se cargan. Compara todos los archivos
del paquete completo para confirmar que solo cambian los dos fuentes.

El BAT habitual se ejecutó con su intérprete real y token no elevado: llegó a
READY, con reacciones nativas listas, y terminó exit0 por la ruta normal antes
de aceptar conexiones. La instrumentación temporal solo se aplicó al proceso
de prueba; el BAT no se modificó. RE4 permaneció abierto. Gameplay pendiente.
