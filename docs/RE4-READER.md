# Primera prueba con RE4 UHD 1.1.0

Este componente detecta el proceso real `bio4.exe`, comprueba su version por la
cadena que utiliza el menu y lee la posicion y los angulos del personaje cuatro
veces por segundo. Es un lector externo de memoria para Windows, escrito con la
biblioteca estandar de Python 3.10+. No requiere `re4_tweaks` instalado.

## Uso

1. Extrae todo el ZIP del lector en una carpeta propia, fuera de la carpeta de RE4.
2. Abre RE4 desde tu instalacion habitual y carga una partida donde puedas caminar.
3. Abre `START-RE4-READER.bat`. Minecraft y el simulador pueden estar cerrados.
4. Debe aparecer `RE4 UHD 1.1.0 detectado` y muestras con `state: player`.
5. Camina en RE4: `position_raw` debe cambiar. Gira: comprueba `angles_raw`.
6. Cierra el lector con Ctrl+C. No hay archivos del juego que restaurar.

La ruta comprobada por defecto es:
`D:\Games\Resident Evil 4 - Ultimate HD Edition\Bin32\bio4.exe`.
Si cambias la instalacion, desde una terminal en la carpeta del paquete usa:

```powershell
python tools/re4_reader.py --game-exe "D:\otra carpeta\Bin32\bio4.exe"
```

Puedes usar `py -3` en lugar de `python`. El programa no solicita permisos de
administrador. Si sale acceso denegado, comprueba que RE4 y el lector se ejecutan
con tu usuario habitual y que RE4 no se haya iniciado como administrador.

`waiting_player` significa que no hay personaje cargado. `transition` descarta
una muestra cuando el puntero cambia durante la lectura. Las lecturas no son una
captura atomica del frame; pueden mezclar valores de dos frames consecutivos.
Si el juego se cierra o la memoria deja de estar disponible, el lector se detiene
tras diez errores consecutivos. Reinicialo despues de volver a abrir RE4.
Si indica firma incompatible, ambigua o version no compatible, envia el texto
del error; no utiliza direcciones de respaldo ni pide reemplazar tu ejecutable.

## Alcance y referencias

Solo abre el proceso con permisos de consulta y lectura (`0x1010`). No escribe
memoria, inyecta DLL, modifica ejecutables, guardados o configuraciones, ni escucha
en red. No cambia camara, movimiento, colisiones o dano. No se conecta todavia
con el mod de Minecraft; el modo `simulator` del protocolo sigue siendo exclusivo
del simulador y no se suplanta para que el cliente parezca conectado a RE4.

Las posiciones son unidades originales de RE4; no se asume una conversion a
bloques de Minecraft ni una correspondencia de ejes. Los angulos tambien se
presentan sin conversion. Primero debemos comprobar esos valores en el juego.

Localizacion basada en las firmas publicadas en
[re4_tweaks Game.cpp](https://github.com/nipkownix/re4_tweaks/blob/92c0208bd09c29c9640e13a2493b8e6bd6edd700/dllmain/Game.cpp)
y la disposicion de
[cCoord.h](https://github.com/nipkownix/re4_tweaks/blob/92c0208bd09c29c9640e13a2493b8e6bd6edd700/dllmain/SDK/cCoord.h).
Implementacion independiente; no incluye codigo, binarios ni recursos del juego.
Cada firma debe aparecer una sola vez en las secciones ejecutables PE32/i386 y
sus operandos deben apuntar dentro del modulo. Solo admite la cadena `1.1.0`.

Probado en cloud con memoria sintetica: firmas, cabeceras PE, versiones,
ambiguedad, punteros y cambios de escena. La API Win32 y la lectura de RE4 real
quedan pendientes de esta prueba en tu PC. No se ha ejecutado RE4 en cloud.
