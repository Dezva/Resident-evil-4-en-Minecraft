# RETIRADA: 0.4.13 provoca cierres antes del menú en el equipo del usuario.
Usa [la recuperación 0.4.14](RE4-RECOVERY-0.4.14.md). Las descripciones siguientes
son históricas; este detector ya no se incluye en la DLL actual.

# 0.4.13: tablas gráficas privadas observadas en el diagnóstico

El informe graphics-probe081059 muestra Direct3D9 oficial de Windows y una
**candidata** cuya tabla está en memoria privada: sus119 métodos pertenecen a
d3d9.dll y su referencia se encuentra a0xC30 bytes de una estructura referenciada
por RE4. El detector0.4.12 excluía tablas fuera de la imagen DLL y examinaba solo
256bytes por estructura, de modo que esta disposición quedaba descartada.
El informe es de lectura y **no demuestra que la candidata sea el dispositivo
activo**. No se usan direcciones absolutas del informe como configuración.

Ahora se admiten tablas privadas completas/legibles: mantiene coincidencia de
los tres métodos de consulta con la sonda oficial y comprobación de pertenencia
ejecutable de cada uno de los119 métodos a Direct3D9. GetCreationParameters debe
indicar una ventana visible del proceso y GetDirect3D debe devolver la fábrica
que utiliza RE4, localizada mediante firma única validada. Cada referencia COM
obtenida se libera; candidatos con otra fábrica se rechazan. Se confirma que la
fábrica no cambió antes de instalar. Solo acepta un dispositivo único.

Se examina hasta4096bytes de cada estructura referenciada, máximo4096 raíces
únicas, sin cadenas recursivas ni escaneo global del proceso. Puede provocar
una pausa breve al comenzar. Clona tabla privada para modificar únicamente
EndScene del dispositivo; mantiene restauración gráfica, parada/F8 y TTL.
No admite interfaces arbitrarias del driver ni convierte pistas en hooks.

**La visualización efectiva todavía requiere tu prueba Windows.** No se cambia
el algoritmo de cámara y el bloque sigue siendo un cubo de líneas verdes sin
texturas, colisión u ocultación por terreno.

## Actualizar desde0.4.12

1. Cierra RE4, el host y VIEW. Ejecuta **UNINSTALL-RE4-CONTROLS del paquete0.4.12**.
2. Extrae0.4.13 en una carpeta nueva y ejecuta su **INSTALL-RE4-CONTROLS**.
3. **Conserva re4bridge-0.4.11.jar y Fabric API** en Minecraft. El ZIP contiene
   esos mismos mods; no necesitas copiarlos otra vez.
4. Abre RE4 **en ventana** y carga partida; abre Minecraft. Inicia el
   **START-RE4-CONTROLS del paquete0.4.13** con los permisos de RE4.
   Este host es necesario para leer el canal gráfico nuevo. Usa su VIEW.
5. En Minecraft: `/re4bridge controller`; apunta al bloque colocado y ejecuta
   `/re4bridge blocktest`, después `/re4bridge arm`. Cierra el chat y F9 muestra
   RE4. Puedes activar ratón/primera persona como antes. Espera unos15segundos.

Si aparece, camina alrededor y comprueba que permanece fijo. Romper el bloque
seleccionado debe ocultarlo. F8 oculta el dibujo y devuelve el control habitual.
Chat/pérdida de foco detiene entradas: vuelve a armar cuando retomes la prueba.
No se copian terreno, estructuras ni mobs del mundo Minecraft.

## Qué compartir

Envía `/re4bridge status` y la última línea **BLOQUE DIAGNOSTICO:** del host.

- Estado1: dispositivo enganchado; **frames debe aumentar**. Dibujos solo crece
  con preview armado, vigente, perteneciente al jugador y líneas en pantalla.
- Estado3: no encontró dispositivo validado. `tables` cuenta tablas compatibles;
  `devices` candidatos de ventana/fábrica válidos. `factory_rejected` indica otra
  fábrica o fallo de esa consulta; `window_rejected` indica ventana no válida.
- `probe_valid=1`: la tabla de la sonda pasó la misma validación de métodos.
  Cero no permite inferir que las tablas reales sean compatibles.
- Estado2: sonda fallida;4: ambiguo/cambio al instalar. Son fallos anteriores al
  dibujo. `HRESULT=0` con frames0 no demuestra éxito.

También se incluye PROBE-RE4-GRAPHICS para diagnóstico externo independiente;
no hace falta ejecutarlo otra vez para esta primera prueba de0.4.13.

## Evidencia y límites

PE32 compilada con advertencias como errores; cuatro suites C y48 pruebas Python
pasan. Se ejercitan los helpers nativos reales con tabla privada, referencia a
0xC30, consulta de fábrica y liberación tanto aceptada como rechazada. Métodos
ajenos, identidad de consulta distinta, ventanas ajenas/ocultas, regiones
protegidas, duplicados y ambigüedad se rechazan. Callback EndScene real conserva
estado/parada/TTL/generación mediante COM simulado. IPC visualv3 de100bytes;
host admitev1/v2/v3 y conserva contadores al publicar. Mod0.4.11 idéntico al
paquete anterior, previamente probado con17 tests core y2 Minecraft.

No se ejecutó D3D9 ni el juego en Linux. Diagnóstico del usuario localiza una
candidata; todavía no se ha confirmado dibujo real ni colisión. Si RE4 recrea
el dispositivo, reinícialo. No cambia partidas, inventario o posiciones.

Reversión: cierra juegos/host/VIEW, UNINSTALL0.4.13 e INSTALL0.4.12; usa host
anterior y conserva mod0.4.11 y copias de seguridad.
