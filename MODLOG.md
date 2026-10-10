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
