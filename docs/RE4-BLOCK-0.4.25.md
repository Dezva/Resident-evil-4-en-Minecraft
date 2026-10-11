# 0.4.25 — texturas estáticas corregidas y Minecraft como controlador fijo

Esta versión corrige el damero que apareció en piedra, tierra y madera con
`blocks style minecraft`. En Minecraft 1.21.1, el flujo de fotogramas de un sprite
sin animación devuelve 1: el puente lo interpretaba como índice y buscaba una
segunda imagen inexistente. La corrección elige cero cuando no hay animación.
Las texturas animadas usan su primer fotograma declarado (todavía no se animan
en RE4).

Se leen los píxeles del sprite ya decodificado por Minecraft, con conversión
ABGR→ARGB, tintes y UV del modelo. No se vuelven a abrir PNG por cara/tick ni se
leen texturas del GPU. Esto admite también sprites creados por el atlas y los
recursos activos. Los píxeles pertenecen a Minecraft: el puente no los modifica
ni libera. No se incluyen imágenes originales del juego en el paquete.

La escena, sus materiales y su JSON se reutilizan mientras no cambien. Colocar,
romper, cambiar formas/estilo/altura/colisión o recargar recursos invalidan la
caché. La paleta se reconstruye con las colocaciones que quedan, para que los
materiales retirados no consuman indefinidamente sus 64 teselas.

## Actualizar desde 0.4.24

**La DLL es exactamente la de 0.4.24. No desinstales ni reinstales controles.**
SHA256: `f75dc2d963d984fa7da1822aca08a95607bac0ab33bdcfdf14749be8fe68b3e4`.

1. Cierra Minecraft y el host. Extrae el paquete 0.4.25 completo.
2. Sustituye el JAR del puente en `%APPDATA%\.minecraft\mods` por
   `mods/re4bridge-0.4.25.jar`, dejando solo un JAR RE4 Bridge.
   Fabric API, Fabric Loader y Minecraft siguen iguales.
3. Usa `START-RE4-CONTROLS` del paquete nuevo. Conserva la DLL instalada 0.4.24.
   RE4 en ventana y partida cargada; Minecraft enfocado y sin minimizar.
   VIEW sigue siendo opcional.

Desde una DLL anterior, sigue primero la instalación de 0.4.24 descrita en
[su guía](RE4-BLOCK-0.4.24.md). Este paquete incluye esa misma DLL y los
instaladores, que siguen rechazando DLL ajenas sin sobrescribirlas.

## Minecraft quieto al controlar a Leon

Por defecto, al activar `arm`, W/A/S/D y correr controlan RE4 sin mover al
personaje de Minecraft. Así no recorrerá/generará chunks por esas teclas mientras
pruebas a Leon. Ratón, inventario y colocar/romper bloques siguen disponibles.
La física normal de Minecraft continúa: no se congela el servidor ni se cambian
la posición o velocidad del jugador. Un empujón, caída o movimiento previo puede
seguir moviéndolo; para la prueba usa creativo, terreno plano y sin mobs cerca.

**F8** devuelve el control normal. Para caminar por Minecraft o construir más
lejos, detén con F8 y después vuelve a usar `arm` para probar en RE4.
Abrir chat/menú, perder foco, desconectar o detener también libera el input.
La configuración excepcional `/re4bridge walking on` permite mover ambos juegos
como antes; `/re4bridge walking off` vuelve al modo fijo. Ambas desarman y requieren
otra `arm`. El modo fijo no importa aún el terreno de RE4 a Minecraft.

## Prueba breve

1. `/re4bridge controller`, espera conexión; `/re4bridge blocks on`.
2. `/re4bridge blocks collisions on`, después `/re4bridge arm`, cierra el chat.
3. Coloca piedra, tierra, tablones y un tronco de roble, más una losa de piedra.
   En RE4 deberían verse los materiales distintos, las caras del tronco y la losa.
4. Mantén W contra la pared varios segundos. Minecraft debería quedarse en el
   lugar; Leon debería detenerse ante ella. Retrocede y prueba una diagonal.
5. Pulsa F8, camina en Minecraft y vuelve a armar. Comprueba también romper un
   bloque y recargar recursos con F3+T, seguido de otra `arm` si se desarma.

Si reaparece el damero o desaparecen los bloques, copia `/re4bridge status`:
incluye causa de desarmado, parada nativa, estado gráfico, contactos y ahora
`caras sin textura` / `error material`. Los errores de material también aparecen
con el prefijo `RE4 material` en `.minecraft/logs/latest.log`, sin inundarlo cada
tick. Abrir el chat desarma; los contadores y el último error quedan disponibles.

No está demostrado que la desaparición anterior fuese causada por chunks.
La lectura fallida de PNG se repetía por cada cara y tick; se eliminó ese trabajo.
El control sigue deteniéndose si faltan ticks recientes, hay una parada nativa o
fallan dibujo/colisión. No se alargaron las caducidades para ocultar tirones.
El coste real restante, incluida la creación del atlas D3D9 por dibujo, debe
comprobarse en Windows.

## Límites y validación

Se conserva la barrera horizontal 0.4.24 para Leon, que el usuario confirmó con
el estilo piedra. No incorpora apoyo vertical, saltos, enemigos, Ashley,
proyectiles ni ocultación por el escenario RE4. Escala provisional 1000 unidades,
64 colocaciones, 128 cajas y 64 teselas (una de reserva).

Las pruebas usan NativeImage/SpriteContents, acceso Fabric y BakedQuadFactory
reales, sin ventana ni contexto gráfico: seis caras, sprite estático sin PNG,
canales, tinte, transparencia, fotograma animado, imagen prestada, invalidación
por geometría/recarga/eliminación, input/restauración/conflicto entre componentes,
TCP y caducidad. El JAR se compila y su access widener se valida/remapea.
Pasan 97 pruebas Python, 21 de bridge-core y 24 de Minecraft, sin fallos ni
pruebas omitidas. La nube no ejecuta RE4 ni comprueba la apariencia/rendimiento
reales de Windows.

Para recuperar 0.4.24 basta reponer su JAR y host; la DLL no cambia. F8 conserva
el retorno normal sin desinstalar nada.
