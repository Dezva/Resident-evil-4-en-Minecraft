# 0.4.56-arrowfix1 — primer tick de las flechas

El usuario probó0.4.56: Leon ya muestra la patada correctamente; arco y ballesta
no dañan Ganados y sus flechas no aparecen en RE4, con o sin paredes.
Se conserva la patada y queda pendiente adaptar el modelo/skin Minecraft a Leon.

## Actualizar teniendo0.4.56 instalada

1. Cierra Minecraft.
2. Retira `re4bridge-0.4.56.jar` de `mods` y copia el nuevo
   `mods/re4bridge-0.4.56-arrowfix1.jar` del ZIP. Deja una sola versión del mod.
3. Abre Minecraft y usa el mismo START-RE4-CONTROLS/VIEW0.4.56 de antes.

Este parche cambia solo el mod. No ejecutes los instaladores/desinstaladores
de DLL por este cambio: DLL456 y host0.4.56 permanecen idénticos.
El agente no instala ni sustituye archivos del juego ni modifica partidas.
Para volver atrás basta con cerrar Minecraft y recuperar el JAR0.4.56 anterior.

El ZIP completo es una alternativa para una instalación nueva: mantiene los
archivos del host y la DLL456 publicados en0.4.56. Su README explica este parche;
para una instalación desde cero consulta también la guía0.4.56 del repositorio.

## Causa y alcance

La condición `arrow.age==0` no se cumple al entrar por primera vez en
PersistentProjectileEntity.tick: ServerWorld y ClientWorld incrementan age
antes de invocar Entity.tick. La flecha nueva ya llega con edad1. La misma
marca de pertenencia controla daño y visualización; al no registrarse, ambos
quedaban sin activar. No era un cambio de DLL ni de permisos.

El registro ahora acepta la edad0/1 solo durante la primera comprobación de
vida de la entidad. Esa decisión se guarda incluso si el puente está desarmado,
para no adoptar flechas viejas al armarlo después. Siguen siendo obligatorios
propietario local, dimensión, sesión, cobertura del terreno y validación nativa
del impacto. No se relajan paredes, F8, caducidad ni límites de daño.

Se añaden mensajes acotados de flecha en el log de Minecraft: registro,
espera de terreno, solicitud y resultado de impacto. Sin geometría ni tokens.
Permiten revisar el siguiente intento leyendo el equipo directamente.

Los logs de la prueba también tienen intervalos de mapa incompleto/caducado.
Durante ellos la pausa de flechas se mantiene por seguridad. No se trata el
terreno desconocido como vacío para conseguir impactos.

## Prueba breve offline

En una zona abierta, espera a que el puente esté activo. Dispara con arco
completamente tensado contra un Ganado a3–5 bloques y repite con ballesta.
Comprueba si la flecha se ve y si el enemigo reacciona al alcanzarlo. Después
coloca una pared delante del Ganado y comprueba que no recibe ese disparo.
Si algo falla, indica arma y si se vio la flecha; los logs los revisa el agente.

Los límites de0.4.56 continúan: cobertura local, overlay sin Z nativo y efectos
especiales/perforación/cohetes pendientes. La patada queda confirmada por el
usuario; el modelo Minecraft durante ella todavía no está implementado.
