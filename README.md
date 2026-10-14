# RE4 × Minecraft: primer prototipo del puente

Proyecto para ejecutar Minecraft y Resident Evil 4 Ultimate HD Edition 1.1.0
en Windows, con primera persona, construcción y combate de Minecraft dentro de
RE4. El desarrollo utiliza las guías y herramientas de
[Universal Modder](https://github.com/rehan-remade/universal-modder).

**Estado actual:** [0.4.44: construcción, salto y rotura](docs/RE4-BLOCK-0.4.44.md).
Selección por caras con mirada fresca y90°, salto del controlador con apoyo
sobre construcciones y colocación debajo, grietas de minería y fragmentos con
profundidad sin colisión. DLL444 nueva. El salto mueve Steve y la vista;
el cuerpo nativo de Leon conserva su Y. Vida/muerte, F5/Esc/R/F8 se conservan.
El fallo de colisiones con ganados reportado en el juego sigue pendiente,
al igual que combate Minecraft, vuelo creativo y pausa del reloj RE4.
Pruebas automáticas pasan; cambios de esta versión necesitan prueba Windows.

## Versiones fijadas

| Componente | Versión |
|---|---|
| RE4 Bridge | paquete/mod 0.4.44 |
| Minecraft Java | 1.21.1 |
| Fabric Loader | 0.16.10 (mínimo declarado) |
| Fabric API | 0.116.6+1.21.1 (base de compilación) |
| Yarn | 1.21.1+build.3 |
| Fabric Loom | 1.9.2 |
| JDK para compilar | 21 |
| Gradle Wrapper | 8.11.1 |
| Python para el simulador | 3.10 o posterior, sin paquetes adicionales |

Se recomienda probar primero esa combinación y sin otros mods. Fabric API es un
mod separado: instalar Fabric Loader no instala automáticamente Fabric API.

## Compilar y probar

Desde la raíz, con JDK 21 en `JAVA_HOME`:

```powershell
.\gradlew.bat test
py -3 -m unittest discover -s tools -p "test_*.py" -v
.\gradlew.bat -Pminecraft=true :minecraft:build
```

Para la prueba Java/Python en Windows, si no existe `python3` en PATH, configurar
`$env:RE4BRIDGE_PYTHON` con la ruta de `python.exe` (sin argumentos).
En Linux, usar `./gradlew` y `python3` en los mismos comandos.
El JAR instalable se genera en `minecraft/build/libs/re4bridge-0.4.44.jar`.
Para control y construcción visual reales, sigue [la guía actual](docs/RE4-BLOCK-0.4.44.md).
Compilar/probar el modulo y los instaladores:

```bash
bash tools/setup_native_toolchain.sh
bash tools/build_native.sh
bash tools/setup_powershell.sh
bash tools/test_install.sh
```
El JAR `-sources` no se instala. El núcleo del puente se incluye en el JAR final.

## Probar el simulador en Windows

1. Crear una instancia/perfil de Minecraft separado, con Fabric 1.21.1. Usar un
   **mundo nuevo de prueba**, modo Creativo y terreno superplano.
2. Copiar `re4bridge-0.4.44.jar` y Fabric API para 1.21.1 a `mods` de esa instancia.
   Retirar el JAR anterior para que quede una sola version de RE4 Bridge.
3. Abrir Minecraft una vez. El mod crea `config/re4bridge.json` con un token local.
4. Abrir PowerShell en la carpeta del proyecto/paquete y arrancar el simulador,
   indicando la carpeta de esa instancia (la que contiene `mods` y `config`):

   ```powershell
   py -3 tools\fake_re4_host.py --game-dir "$env:APPDATA\.minecraft"
   ```

   Esa ruta corresponde a la carpeta `.minecraft` indicada para SKLauncher. Si se
   configura una instancia separada, sustituirla por la carpeta de esa instancia.
   También se puede abrir `START-SIMULATOR.bat`, que utiliza `%APPDATA%\.minecraft`.

5. Dentro del mundo, escribir `/re4bridge connect` y luego `/re4bridge status`.
   La cámara pasa a primera persona. El estado debe decir `simulator` y aumentar
   el contador de mensajes confirmados. Eso **no significa que RE4 esté conectado**.
6. Caminar y girar: `feet` cambia en PowerShell. Colocar y romper bloques cerca
   del jugador: la cantidad `blocks` cambia. Probar losas inferiores y superiores.
   Para inspeccionar las cajas, iniciar con `--report diagnostico.json`; cerrar
   después con Ctrl+C y consultar `last_collision.blocks` en ese archivo.
7. Usar `/re4bridge disconnect` para restaurar la perspectiva anterior. Ctrl+C
   cierra el simulador. Salir del mundo también cierra el puente.

El mod no coloca bloques por sí mismo ni altera inventarios. Se conecta solo al
equipo local. No es necesario abrir puertos del router ni tocar la carpeta de RE4.
No compartir `config/re4bridge.json` ni añadirlo a Git. Para retirar el prototipo,
cerrar Minecraft y quitar únicamente su JAR de `mods`.

Si no conecta, comprobar que `--game-dir` apunta a la misma instancia, que el
simulador muestra `READY`, que no hay otro usando el puerto y que se está en un
mundo local. Después de un fallo, repetir `/re4bridge connect`.

## Próximas etapas

1. Identificar en RE4 UHD 1.1.0 las coordenadas, escala, cámara y colisión reales.
   [re4_tweaks](https://github.com/nipkownix/re4_tweaks) es una referencia para
   código nativo Win32 y Direct3D9; no ofrece una integración Minecraft lista.
2. Dibujar un bloque de prueba y sincronizar la cámara en RE4. Medir escala,
   oclusión y latencia. Resolver el campo de visión y la interfaz/mano de Minecraft.
3. Enviar terreno RE4 a Minecraft y bloques Minecraft a la colisión de RE4.
   Comprobar jugador y enemigos; la navegación de NPC es una tarea aparte.
4. Añadir entidades proxy, unidades de daño y deduplicación para TNT, espadas y
   flechas. Comprobar daño, muerte, aturdimiento y daño de vuelta al jugador.
5. Resolver puertas, cambios de área, Ashley, escaleras, guardados y cinemáticas.

La instalación del usuario está en
`D:\Games\Resident Evil 4 - Ultimate HD Edition\Bin32\bio4.exe`; la pantalla
principal indica 1.1.0. Esa información fue proporcionada por el usuario y no
equivale a una inspección del ejecutable desde esta máquina.

Consultar [el contrato del protocolo](docs/CONTRACT.md),
[el registro de desarrollo](MODLOG.md) y [la validación](docs/VALIDATION.md).
Se distribuye código propio, sin ejecutables ni recursos extraídos de los juegos.

## Vista de RE4 y retorno al juego normal

El [complemento de vista para Windows](docs/RE4-VIEW.md) permite mostrar RE4
sin bordes conservando el foco de Minecraft: F9 desde Minecraft presenta RE4;
F8 o el boton Volver a RE4 normal detienen el puente y restauran la ventana/foco.
Se distribuye separado, sin cambiar los mods ni DLL existentes.
