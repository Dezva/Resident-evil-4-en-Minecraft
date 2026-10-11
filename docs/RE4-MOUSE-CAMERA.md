# 0.4.3: raton y camara experimental en RE4

El usuario confirma el modo de vista y que la primera persona mira al frente
al moverse, pero se desvía al quedarse quieto y deja un arma flotando.
0.4.3 agrega un hook validado del CMP de lr_rate para suprimir el suavizado
QuasiFPS solo en frames de primera persona activa. Se preservan registros y
flags del CMP original salvo ZF cuando se suprime ese suavizado, segun la
referencia re4_tweaks/Trainer.cpp. La firma adicional es obligatoria para FP;
si falta, no se activa FP y siguen disponibles movimiento/raton.
Tambien oculta temporalmente los dos modelos de arma de cPlWep, deduplicando
objetos y restaurando solo mientras siguen perteneciendo al mismo jugador.
No modifica inventario ni borra armas. Incluye el complemento de vista 0.1.1:
F9 oculta todo el panel y F8 detiene/restaura; F9 lo trae de vuelta.
Las correcciones de camara/arma necesitan prueba real en Windows.

## Actualizar desde 0.4.2 (o versiones anteriores)

1. Cierra RE4, Minecraft y el host Python.
2. Ejecuta `UNINSTALL-RE4-CONTROLS.bat` desde la carpeta anterior. Solo retira
   la DLL de ese paquete si el hash coincide; si falla, conserva el archivo.
3. Extrae 0.4.3 en una carpeta nueva. Retira el JAR anterior de RE4 Bridge
   de `.minecraft\mods` y coloca `mods/re4bridge-0.4.3.jar`. Conserva Fabric API.
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
globalmente otras camaras, escenas o efectos. Se intenta ocultar el cuerpo de Leon; los dos modelos de arma se ocultan tambien; efectos independientes pueden seguir visibles.
No se cambia A/D a desplazamiento lateral: sigue siendo giro nativo.

F8, perder foco, abrir UI/pausa o perder muestras desarman. El siguiente
frame calcula la camara original. No se rearma automaticamente. TTL 500 ms
y muestras cliente maximas de 200 ms. Repite firstperson para apagar esa
opcion; mouse apaga ambas. Disconnect limpia las opciones. Para retirar la
DLL, cierra RE4 y usa el UNINSTALL de 0.4.3. Conserva backups y partidas.

## Evidencia

Pruebas C cubren acumulacion/limites/consumo y matematicas del ojo. Java/Python
usan TCP autenticado con IPC sintetico, validacion de valores y capacidades.
Publicacion IPC v2 e instaladores se prueban sobre memoria/archivos sinteticos.
Compilacion DLL PE32 y mod Fabric1.21.1 no demuestran giro/camara reales.

## Prueba de esta correccion

Con FP activo y vista F9: gira con el raton, avanza, suelta W y espera unos
segundos. La direccion no debe desviarse al parar. Observa que el cuerpo y arma
no aparezcan. F8 debe restaurar vista/cuerpo/arma y control RE4; usa F9 para
volver al panel/MC y rearmar explicitamente. Si FP no esta disponible, envia
la salida de PROBE-RE4-NATIVE: incluye camera_smoothing_matches.
