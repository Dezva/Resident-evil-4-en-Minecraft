# Registro de desarrollo

## 2026-10-10 — Inicio del puente de diagnóstico

- Solicitud: ambos juegos reales en Windows; Minecraft/Steve en primera persona
  dentro de RE4, bloques con colisión, TNT y armas con daño cruzado. Losas para
  aproximación inicial de terreno irregular; formas más precisas cuando haga falta.
- RE4: Ultimate HD Edition, 1.1.0 según pantalla principal del usuario;
  `D:\Games\Resident Evil 4 - Ultimate HD Edition\Bin32\bio4.exe`.
- Minecraft: el usuario instaló Fabric con Minecraft Java 1.21.1 y usa SKLauncher.
  Su carpeta de instancia es `%APPDATA%\.minecraft`.
- Ruta: dos procesos con puente de estados y posteriormente renderizado integrado.
  Se empieza con el lado Minecraft y un receptor sintético, sin direcciones de
  memoria inventadas ni loaders copiados en RE4.
- Referencias leídas: Universal Modder `mod-any-game`, `mashup-mods`, nota de
  colisión/combate y ejemplos GTA V/RE2. RE2 usa RE Engine; no se traslada su
  REFramework a RE4 clásico. La base `re4_tweaks` usa Win32 y Direct3D9.
- Autoridad inicial: Minecraft mueve al jugador; el simulador solo recibe datos.
  El contrato está en `docs/CONTRACT.md`. No se transmite todavía renderizado ni
  se conoce el cambio de ejes/escala de RE4.
- Versiones Fabric/Yarn/Loom verificadas en metadatos oficiales. Se fijaron en
  Gradle; no se requiere cambiar la versión de Minecraft del usuario.
- La máquina cloud tenía Java 21 runtime pero no `javac`. Se preparó Temurin
  JDK 21 con descarga oficial y SHA-256 verificado, fuera del checkout.
- Se añadieron requisitos de red oficiales Fabric/Mojang al borrador. La primera
  consulta dio 403; una consulta posterior a Fabric Maven respondió 200.
- No se lanzaron juegos en el PC del usuario, no se tocaron sus perfiles ni
  archivos de RE4 y no se publicó nada en GitHub.

Resultados actuales y límites de evidencia: `docs/VALIDATION.md`.

## 2026-10-10 — Lanzador Python en Windows

- El usuario ya puso los JAR y abrió el mundo superplano en SKLauncher, pero el
  lanzador batch falló porque Windows no encontró el comando `py`.
- Se añadió detección de `python` como alternativa y comprobación de Python 3.10+,
  con instrucciones de instalación si ninguno de los dos comandos funciona.
- No se cambiaron los JAR ni el protocolo. La ejecución batch en Windows sigue
  pendiente de confirmar en el equipo del usuario.

## 2026-10-10 — Lector externo de RE4

- El usuario confirmo conexion Minecraft/simulador y cambios de `blocks` al
  colocar/romper; se actualizo la evidencia de validacion manual.
- Solicito crear el primer componente de RE4. Se implemento lector externo
  Python/Win32, permisos de consulta/lectura exclusivamente, sin instalacion en
  el juego. Comprueba ruta, PE32/i386, version 1.1.0, unicidad de firmas y punteros.
- Referencia: nipkownix/re4_tweaks revision
  `92c0208bd09c29c9640e13a2493b8e6bd6edd700`, Game.cpp para version/pPL y
  SDK/cCoord.h para posicion/angulos. No se copio el loader ni se uso una
  direccion absoluta de una instalacion particular.
- Seis pruebas sinteticas del lector y cinco del simulador pasaron. El lector
  no esta conectado al protocolo Minecraft, no controla RE4 y su API Win32 aun
  debe probarse en el PC del usuario. No se afirma integracion nativa completa.
- Paquete independiente `re4bridge-re4-reader-0.1.0-windows.zip`, con lanzador,
  lector e instrucciones; generado por `tools/package_re4_reader.py`.
