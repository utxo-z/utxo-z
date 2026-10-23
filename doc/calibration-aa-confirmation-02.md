# Confirmación A/A 02 — método congelado

Corrida de confirmación con el método fijado **antes** de medir. Sin cambios de
código: arnés, runner y módulo estadístico son los de `ca8caa6`, y sus hashes se
tomaron antes de la primera medida.

Protocolo: `calibration-aa-02/protocol.md`,
`sha256 = 4ebc9cd15d133929b261c99e3efc8ea4f4debe4289c297913765a382341e6a02`,
sellado a las `2026-10-23T12:38:38Z`.

Datos: `/home/fernando/dev/utxo-z/calibration-aa-02/` — ruta nueva, nada
sobrescrito. Crudos, stderr, comandos, hashes y stdout completo con progreso y
latido, más el código de salida real de cada corrida.

## Identidades, antes de medir

| | sha256 |
|---|---|
| `benchmarks/calib_stationary.cpp` | `7e18fc2c7d35a34d…` |
| `scripts/run_aa_pilot.py` | `b19ade2499e84e52…` |
| `scripts/recompute_aa_family.py` | `2eb7f747965f844c…` |
| binario `utxoz_calib` | `15fe2d2311dac2c4…` |
| `../wt-bench/scripts/bench_analyze.py` (estadística) | `9afff990d9d9adfe…` |

El módulo estadístico vive en otro worktree (`wt-bench`) y entra por argumento
obligatorio, validado y hasheado: no es un import implícito.

HEAD `ca8caa6`, árbol limpio, binario cuatro segundos posterior a su fuente y
`cmake` sin trabajo pendiente: no se reconstruyó nada.

## Lo que se ejecutó

20 pares exactos por métrica, `find_hit` primero y `find_miss` después. Sin
parada temprana, sin ampliar `n` y sin cambiar la duración.

| | |
|---|---|
| lados | N=1 contra N=1, el mismo binario |
| ops por muestra | 10 000 000 |
| warmup | 500 000, fuera del elapsed |
| población | 90 000 entradas, payload de 33 B distinto por clave |
| orden | AB en pares impares, BA en pares pares |
| semilla | el índice del par, igual en los dos lados |
| afinidad | `taskset -c 2`; host sin tocar |
| bootstrap | 3000 remuestreos, semilla 5 |

**Validez operativa:** 40 crudos por métrica, **0 muestras inválidas**, **0
errores reales**, 0 fallos registrados, 400 000 000 de operaciones medidas por
métrica. Los dos informes salen **`verified-current`** del verificador de
familia: config que hashea a sí misma, `2·pairs` muestras con `exit 0` y
`timed_out false`, crudos que hashean a lo registrado y vueltos a pasar por el
mismo `parse_sample` del runner, y razones recalculadas desde las muestras.

## Resultado

| métrica | mediana | IC 95 % individual | IC nominal simultáneo (97,5 %) |
|---|---|---|---|
| `find_hit` | +0,017 % | [−1,403 %, +0,769 %] | [−2,094 %, +0,944 %] |
| `find_miss` | −0,117 % | [−0,218 %, +0,201 %] | [−0,259 %, +0,263 %] |

**Peor límite superior nominal simultáneo: +0,944 %**, por debajo de +2 %.

Los anchos observados son compatibles con un margen de +2 % **en este punto de
operación**. Esa es la afirmación entera.

## Qué no se afirma

1. **No es potencia.** No se corrió control positivo. Sin inyectar un efecto
   conocido y verificar que el instrumento lo encuentra, esto habla del ancho de
   estos intervalos y no de la capacidad de hallar un efecto de 2 %.
2. **No es una garantía sobre falsos positivos.** Bonferroni es un ajuste
   **nominal**: si cada intervalo cubriera a su tasa nominal, la afirmación
   familiar valdría al 95 %. Un bootstrap percentil de una mediana sobre veinte
   pares puede subcubrir, y entonces la familiar también.
3. **Es un punto de operación, no todos.** 90 000 entradas en un contenedor con
   caché caliente cabe en caché. No dice nada de un store con historia, donde
   dominan fallos de TLB, páginas no residentes y recorrido por generaciones.
4. **No es un piso para futuras comparaciones A/B.** Es la precisión observada de
   esta corrida con este workload.

Comparación con el piloto exploratorio 01: las medianas se mueven poco (+0,017 %
contra −0,329 % en `find_hit`; −0,117 % contra +0,050 % en `find_miss`), y el
intervalo de `find_hit` sale **más ancho** acá (semiancho 1,086 % contra
0,498 %). El 01 no tiene standing —esquema legacy, sin muestras ni hash de
config— así que esto no es una réplica: es la primera corrida verificable.

## Recomendación para la unidad siguiente

En este orden, y sin ejecutar nada de esto todavía:

1. **Control positivo.** Inyectar un retardo conocido del orden del 2 % y
   comprobar que el instrumento lo detecta con este mismo método. Hasta
   entonces no hay afirmación de potencia, y sin potencia el margen no es un
   criterio utilizable.
2. **El punto de operación que importa.** Un fixture con historia: varias
   generaciones de la clase 48 y una población que no quepa en caché, midiendo
   `find_hit`, `find_miss` y `resolve` con caché fría y caliente declaradas
   por separado. `find_hit` ya mostró acá el intervalo más ancho de los dos;
   con historia es donde puede ensancharse de verdad.
3. **Las métricas de mutación.** `insert` y `apply_deletes` necesitan restaurar
   la imagen entre repeticiones, que es otro arnés y otra unidad.
