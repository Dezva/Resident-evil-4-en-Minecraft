# Probar RE4 Bridge 0.1.0 en SKLauncher

Este es un **prototipo de comunicación**, todavía sin conexión a RE4.
Necesitas Minecraft Java 1.21.1 con Fabric Loader 0.16.10 o posterior y Python 3.10
o posterior. Fabric API para 1.21.1 viene en la carpeta `mods` del paquete, con su
licencia incluida en el JAR original.

1. Extrae el ZIP en una carpeta de trabajo, por ejemplo en Descargas.
2. Con Minecraft cerrado, abre `%APPDATA%\.minecraft\mods` (puedes pegar la ruta
   en el Explorador). Copia allí los dos JAR de la carpeta `mods` del paquete.
   Si ya tienes Fabric API para 1.21.1, no dejes dos versiones: conserva una sola
   compatible. Para aislar el diagnóstico, usa una instancia sin otros mods.
3. Abre la instalación Fabric 1.21.1 de SKLauncher y crea un mundo nuevo Creativo,
   superplano. No uses tus mundos importantes para esta prueba.
4. Abre `START-SIMULATOR.bat` de la carpeta extraída. Su ventana debe mostrar
   `READY 127.0.0.1:28741 — SIMULADOR, no RE4`. Si falta Python, instala Python 3
   desde su sitio oficial con el lanzador `py` y vuelve a intentarlo.
5. En el chat del mundo, escribe `/re4bridge connect`. Después consulta
   `/re4bridge status`: debe decir `simulator` y aumentar el contador de mensajes.
6. Camina, gira, coloca bloques y rómpelos cerca del jugador. En la ventana del
   simulador deben cambiar `feet` y `blocks`. También puedes probar losas.
7. Termina con `/re4bridge disconnect` y Ctrl+C en el simulador. La perspectiva
   vuelve a la que tenías antes de conectar.

Si utilizas otra carpeta de instancia, abre PowerShell en la carpeta extraída y
ejecuta:

```powershell
py -3 tools\fake_re4_host.py --game-dir 'C:\ruta\de\tu\instancia'
```

Para guardar una muestra de las cajas de colisión:

```powershell
py -3 tools\fake_re4_host.py --game-dir "$env:APPDATA\.minecraft" --report diagnostico.json
```

Tras desconectar y cerrar el simulador con Ctrl+C, ese archivo contiene la última
muestra recibida. Si algo falla, comparte el resultado de `/re4bridge status` y
el mensaje de error. **No compartas `config/re4bridge.json`**, que contiene el
token local de la conexión.

Este paquete no edita la carpeta de RE4 y no requiere abrir puertos del router.
Para desinstalarlo, cierra Minecraft y quita `re4bridge-0.1.0.jar` de `mods`.

Desarrollado con ayuda de IA y las guías de Universal Modder. Minecraft/Fabric y
RE4 pertenecen a sus respectivos autores. El código del proyecto está separado
de los archivos de los juegos. El JAR de Fabric API se obtiene de su Maven oficial,
sin modificaciones; su licencia se conserva dentro del archivo.
