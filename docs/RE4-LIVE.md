# Puente 0.2.0: RE4 real -> Minecraft

La primera conexion real permite consultar la posicion/rotacion del personaje
de RE4 desde Minecraft y, de forma opcional, mover al jugador de Minecraft en
modo espectador para seguirlo. Esta es una prueba de coordenadas, no la fusion
visual de los juegos. Se mantiene Minecraft Java 1.21.1, Fabric y Python 3.10+.

## Instalar la actualizacion

1. Cierra Minecraft. Extrae todo el ZIP en una carpeta propia fuera de RE4.
2. En `%APPDATA%\.minecraft\mods`, retira `re4bridge-0.1.0.jar` y coloca el nuevo
   `re4bridge-0.2.0.jar` de la carpeta `mods` del ZIP. Debe quedar un solo JAR
   de RE4 Bridge. Conserva Fabric API; no necesitas cambiar Fabric/Minecraft.
3. Abre Minecraft con Fabric 1.21.1 y un mundo local de prueba con comandos
   permitidos. La configuracion existente `config/re4bridge.json` se reutiliza.
4. Abre RE4 UHD 1.1.0 y carga una partida. Cierra el simulador y el lector
   anterior; el nuevo host ocupa el mismo puerto local.
5. Ejecuta `START-RE4-BRIDGE.bat`. Debe decir `LECTOR RE4 REAL`.
   Si RE4 esta ejecutandose como administrador, usa el mismo nivel para este
   lanzador, tal como hiciste con el lector anterior.

La ruta RE4 por defecto sigue siendo
`D:\Games\Resident Evil 4 - Ultimate HD Edition\Bin32\bio4.exe`.
Para otras rutas o instancias puedes usar:

```powershell
python tools/re4_live_host.py --game-exe "D:\otra carpeta\Bin32\bio4.exe" --game-dir "C:\ruta\minecraft"
```

No compartas `config/re4bridge.json`: contiene el token local. El programa lo
lee desde la instancia de Minecraft; no lo introduce en argumentos ni registros.

## Probar primero la lectura en Minecraft

En el chat de Minecraft:

```text
/re4bridge re4
/re4bridge status
```

El estado debe indicar `RE4 reader (read only)` y coordenadas `RE4 raw`.
Camina/gira en RE4 y vuelve a consultar el estado: deben cambiar. La orden
`/re4bridge connect` sigue siendo exclusiva del simulador; no la uses con RE4.

## Probar el seguimiento

Minecraft cambiara la posicion de su propio jugador mientras sigues a RE4.
Usa un mundo de prueba en modo espectador para esta calibracion.
Colocate donde quieras que empiece el recorrido y deja quieto al personaje RE4.
En Minecraft ejecuta:

```text
/gamemode spectator
/re4bridge calibrate
/re4bridge follow
```

Pulsa F3 + P para que Minecraft siga ejecutandose al pasar a RE4. Camina y gira
en RE4: Minecraft debe seguir los desplazamientos y giros del personaje.
Se actualiza hasta 10 veces por segundo; aun no hay interpolacion por frame.
`/re4bridge follow` otra vez detiene el seguimiento. `/re4bridge disconnect`
cierra la conexion y restaura la perspectiva anterior; conserva el modo de
juego que elegiste. Puedes volver con `/gamemode creative`.

## Ajustar la calibracion

Cada referencia asocia la posicion y orientacion actuales de ambos personajes.
El punto de partida de Minecraft no necesita coincidir con las coordenadas
absolutas de RE4. La escala inicial **100 unidades RE4 por bloque** es provisional.
Estas ordenes detienen el seguimiento y fijan una nueva referencia; deja quietos
ambos personajes y vuelve a usar `/re4bridge follow` despues de cada ajuste:

| Orden | Efecto |
|---|---|
| `/re4bridge scale 200` | Mitad del desplazamiento Minecraft frente a escala 100 |
| `/re4bridge scale 50` | Doble del desplazamiento Minecraft frente a escala 100 |
| `/re4bridge ysign 1` | Elevacion Minecraft sigue el signo de Y de RE4 |
| `/re4bridge ysign -1` | Invierte Y; valor inicial |
| `/re4bridge turnsign -1` | Invierte el sentido de los giros |
| `/re4bridge turnsign 1` | Mantiene el sentido de los giros; valor inicial |
| `/re4bridge calibrate` | Fija otra referencia con los ajustes actuales |

X/Z conservan sus ejes relativos. La orientacion se ancla al inicio y usa el
cambio del angulo del personaje, no el de la camara RE4. Minecraft mantiene
pitch cero durante el seguimiento. Los ajustes duran esta sesion de Minecraft;
al reconectar hay que calibrar otra vez.

## Cuando se suspende o detiene

- Muestra de mas de un segundo, menu sin personaje o memoria no disponible:
  suspende las actualizaciones hasta recibir otra muestra valida.
- Desconexion, cambio de dimension o salida del modo espectador: detiene el
  seguimiento. Repite calibracion/seguimiento cuando corresponda.
- Salto mayor de 64 bloques entre destinos o coordenadas fuera de rango:
  detiene el seguimiento y solicita recalibrar; por ejemplo al cambiar de sala.
- Pausa de Minecraft: mantiene la conexion, pero no mueve su jugador.
- Reinicio de RE4: reinicia tambien el host; el proceso anterior ya no sirve.

## Alcance de la prueba

El host solo consulta RE4 con los mismos permisos y firmas del lector que ya
probaste. No controla RE4, su camara, dano o colisiones. El modo espectador no
prueba colision fisica: sirve para validar la transmision y conversion de
coordenadas sin que el suelo de Minecraft bloquee el seguimiento.
No hay imagen de RE4 dentro de Minecraft ni geometria de RE4 reconstruida.
El siguiente paso hacia el objetivo final requerira controlar RE4 y trabajar
con su camara/geometria, despues de comprobar este puente real.

Se compila y prueba en cloud con un peer Python sintetico sobre TCP real y
pruebas de escala, orientacion, caducidad y muestras invalidas. La lectura
independiente de RE4 fue confirmada por el usuario; el seguimiento completo de
esta version nueva aun requiere la prueba en Windows.
