# 0.4.1: raton y camara experimental en RE4

El usuario confirmo W/S, A/D y giro por raton con Minecraft enfocado y RE4
en ventana. En 0.4.0 la camara activa quedo a la altura de las piernas.
0.4.1 aumenta la altura inicial de 160 a 1600 unidades y amplia su ajuste
hasta 2500. Es una correccion para probar: la altura exacta y el espacio de
camara aun necesitan validacion en Windows; no se ejecuta RE4 en cloud. No se convierte el personaje en Steve ni se oculta el modelo de Leon.

## Actualizar desde 0.4.0 (o 0.3.5)

1. Cierra RE4, Minecraft y el host Python.
2. Ejecuta `UNINSTALL-RE4-CONTROLS.bat` desde la carpeta anterior. Solo retira
   la DLL de ese paquete si el hash coincide; si falla, conserva el archivo.
3. Extrae 0.4.1 en una carpeta nueva. Retira el JAR anterior de RE4 Bridge
   de `.minecraft\mods` y coloca `mods/re4bridge-0.4.1.jar`. Conserva Fabric API.
4. Ejecuta el nuevo `INSTALL-RE4-CONTROLS.bat`. Verifica el backup de Saves
   antes de instalar. No copia encima de DLL existentes; no la copies a mano.
5. Abre RE4 en ventana con partida cargada y el mundo Minecraft de prueba.
   Ejecuta el nuevo `START-RE4-CONTROLS.bat` con los mismos permisos de RE4.

El host anuncia raton=True y primera_persona=True si acepta sus firmas.
Si falta la firma de camara, sigue disponible el movimiento. El nuevo
`PROBE-RE4-NATIVE.bat` permite investigar sus coincidencias sin modificar RE4.
IPC v2 tiene 80 bytes; el host tambien acepta v1 para movimiento sin raton.

## Giro horizontal primero

Ejecuta controller, espera conexion, luego mouse y arm:

```text
/re4bridge controller
/re4bridge mouse
/re4bridge arm
```

Cierra chat y mueve el raton lentamente a derecha/izquierda. Debe girar el
personaje sin A/D; prueba tambien W/S. En este modo conserva la camara RE4
original y el raton vertical no la modifica. Espectador, Creativo y Survival
admiten la prueba. Mantene Minecraft enfocado; F8 detiene.

El indicador muestra `arm=si`, `raton=<muestras aplicadas>` y
`teclas=<enviadas>/<aplicadas>`. El giro se ancla al cerrar chat en cada arm,
sin saltar al angulo absoluto de Minecraft. Respeta su sensibilidad y envia
totales acumulados para no perder giros al agrupar muestras. Se limita a
30 grados por muestra y recupera la diferencia pendiente en muestras siguientes.
A/D conservan su giro nativo y pueden sumarse al raton.

## Primera persona despues

```text
/re4bridge firstperson
/re4bridge arm
```

Firstperson cambia la opcion y desarma; arm la habilita al cerrar chat.
Tambien activa el raton. `FP=si` confirma aplicacion del punto de camara
ese frame; `altura=1600` identifica el ajuste enviado. Prueba izquierda/derecha, arriba/abajo y unos pasos. Si apunta
en otra direccion, queda baja/alta o muestra el interior del modelo, envia
una captura de RE4 y del indicador; la orientacion/espacio de camara es experimental.

Altura inicial: 1600 unidades RE4, provisional. El codigo publico de
re4_tweaks (Trainer.cpp, revision 92c0208bd09c29c9640e13a2493b8e6bd6edd700)
eleva su camara libre 2000 unidades para situarla sobre el personaje; este
es el motivo de ampliar el rango, no una medicion de los ojos de Leon.
Si queda baja, prueba `/re4bridge eyeheight 2000` y luego arm; si queda alta,
prueba 1200. Su valor absoluto debe ser 40..2500; se admite signo negativo
para comprobar el eje Y opuesto. La altura no se guarda al reiniciar Minecraft. Pitch +/-85
grados; FOV y roll se conservan de la rutina original. No se modifican colision
del personaje ni guardados, ni se enlaza apuntado de armas con Minecraft.
Solo cambia la camara QuasiFPS durante entrada armada/fresca; no reemplaza
globalmente otras camaras, escenas o efectos. El modelo puede verse o recortar.

F8, perder foco, abrir UI/pausa o perder muestras desarman. El siguiente
frame calcula la camara original. No se rearma automaticamente. TTL 500 ms
y muestras cliente maximas de 200 ms. Repite firstperson para apagar esa
opcion; mouse apaga ambas. Disconnect limpia las opciones. Para retirar la
DLL, cierra RE4 y usa el UNINSTALL de 0.4.1. Conserva backups y partidas.

## Evidencia

Pruebas C cubren acumulacion/limites/consumo y matematicas del ojo. Java/Python
usan TCP autenticado con IPC sintetico, validacion de valores y capacidades.
Publicacion IPC v2 e instaladores se prueban sobre memoria/archivos sinteticos.
Compilacion DLL PE32 y mod Fabric1.21.1 no demuestran giro/camara reales.
