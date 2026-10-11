# Diagnóstico gráfico de solo lectura para0.4.12

El registro enviado por el usuario confirma **armed=True, cámara activa**, pero
**status3, frames0, tables0** después de5 intentos. El dispositivo no pasó la
búsqueda/validación del detector; no permite identificar todavía qué filtro
lo descartó o si su referencia estaba fuera del rango examinado. Ratón y primera
persona no explican ese estado. HRESULT0 no prueba que se haya dibujado.

Este paquete es **solo diagnóstico**. No incluye ni instala una DLL o mod nuevos.
Deja RE4 Bridge0.4.12 y el JAR0.4.11 tal como están. No contiene instalador.

## Uso

1. Deja RE4 abierto, en ventana y con una partida cargada. Pulsa **F8** para
   detener los controles del puente mientras se hace el diagnóstico. Minecraft
   y el host pueden quedarse abiertos; no necesitas seleccionar otro bloque.
2. Extrae el ZIP en una carpeta nueva. Ejecuta **PROBE-RE4-GRAPHICS.bat** con los
   mismos permisos que RE4 (como administrador si RE4 está elevado).
3. Espera unos20 segundos. Se genera **graphics-probe-<fecha UTC>.json** en la
   carpeta extraída. Comparte ese archivo. Si aparece ERROR en consola, comparte
   ese texto; una lectura no disponible no se informa como búsqueda exitosa.

Usa Python3.10 o posterior, como el host actual. Comprueba ruta del ejecutable y
firmas/version1.1.0 antes de revisar memoria. Solo solicita VM_READ y permisos de
consulta; no escribe en el proceso, no ejecuta métodos COM remotos, no inyecta
código y no abre una conexión a Minecraft ni sube archivos por red. Escribe
únicamente el informe local. El informe contiene módulos gráficos/rutas, RVAs,
contadores y metadatos de posibles tablas; no incluye token, guardados ni un
volcado de memoria.

## Alcance

Enumera bibliotecas gráficas cargadas; analiza datos PE del juego y, como máximo,
4096 estructuras referenciadas (prefijo4096bytes, frente a256 en el detector
actual). Búsqueda limitada a20segundos y32MiB de lecturas; `truncated=true` indica
que no se completó. No busca en todos los heaps ni sigue cadenas recursivas.
La firma de creación D3D9 procede de la referencia pública re4_tweaks fijada en
92c0208bd09c29c9640e13a2493b8e6bd6edd700; únicamente se observa, no se parchea.

`possible_device_tables` son **pistas**, no interfaces COM validadas para enganchar.
Revisa métodos ejecutables y su módulo, admite observar tablas privadas o métodos
que pertenezcan a bibliotecas distintas para explicar los filtros del detector.
Puede incluir falsos candidatos o no encontrar un objeto por su ubicación.
No habilita dibujado, colisión ni nuevas funciones. La reparación gráfica sigue
pendiente del informe del equipo del usuario; no se afirma que el bloque se vea.

## Evidencia

Cinco pruebas del diagnóstico verifican búsqueda indirecta más allá de256bytes,
firma/slot de creación, tabla privada, método de módulo distinto, rechazo de
memoria protegida/punteros desconocidos/métodos no ejecutables y límites explícitos.
Comprueban escritura del informe y cierre de handles incluso al fallar.
Comparan bytes antes/después para comprobar que la inspección sintética no los
modifica. No ejecutan las APIs Win32 ni el driver de RE4 en Linux.
