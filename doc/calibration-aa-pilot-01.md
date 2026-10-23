# Piloto A/A 01 — resolución real de este host

> Verificada con los runners de su momento. Con los mergeados en #149 un reporte sin
> `provenance_after` no se certifica: ver `doc/roadmap-1.0.md`, "Evidencia anterior a #149".
>
> **ESTADO: EXPLORATORIO.** El protocolo cambió a mitad de camino — ver «Cambio
> de protocolo» más abajo — así que estos números no son una corrida de 20 pares
> predeclarada. Sirven para dimensionar el método, no para cerrar un criterio.

**Qué es.** Ambos lados son el *mismo* binario N=1. Lo que sale de acá es la
**precisión observada** del instrumento en esta métrica y en este punto de
operación. **No mide nada sobre sharding** y no puede: no hay candidata.

Tres cosas que este intervalo **no** es, y que una versión anterior de este
documento llamó mal:

- no es un «piso» para futuras comparaciones A/B;
- no es una garantía sobre falsos positivos;
- no es una afirmación sobre potencia: no se corrió control positivo.

Crudos, por corrida y por par: `/home/fernando/dev/utxo-z/calibration-aa-01/`.

## Método, declarado y hasheado antes de medir

| | |
|---|---|
| arnés | `benchmarks/calib_stationary.cpp`, target `utxoz_calib` |
| `source_sha256` | `1c1e2c08e58b0d62…` |
| `harness_sha256` | `0afa1086d4efed39…` |
| runner | `scripts/run_aa_pilot.py` |
| pares | 20, fijos — no se expanden |
| orden | AB en pares impares, BA en pares pares |
| semilla por par | `r`, la misma en los dos lados: misma secuencia de lookups |
| población | 90 000 entradas, valor de 33 B → clase 48 |
| generaciones | 1 archivo de la clase 48, 5 en total |
| caché | caliente; el warmup recorre la misma distribución que se mide |
| ops por muestra | 10 000 000 |
| warmup por muestra | 500 000, fuera del cronómetro |
| región medida | 361 ms (`find_hit`), 199 ms (`find_miss`) |
| afinidad | `taskset -c 2`, proceso propio; **sin cambios de host** |
| scratch | creado por la corrida con nombre aleatorio; **falla si existe**, nunca borra algo heredado |
| estimador | `bench_analyze.bootstrap_median_ci` de R3a: bootstrap percentil de la **mediana de razones pareadas**, remuestreando pares completos |
| bootstrap | 3000 remuestreos, confianza 0,95, semilla 5 |

Setup y warmup están fuera del tiempo medido. Cada operación se valida **fuera** del cronómetro y un fallo es exit distinto de
cero, no una advertencia.

Y se discriminan los tres resultados posibles de `find`, que una versión anterior
confundía usando sólo `has_value()`:

- **resolvió** — se cuenta como hit.
- **`not_resolved`** — no contestable desde las versiones activas.
- **cualquier otro código** — falla real. Aborta con exit 3, en warmup y en
  medición. Tratar esto como un miss reporta una cifra de una operación que no
  ocurrió.

La clasificación vive en `classify()`, extraída del bucle, y `--selftest-classifier`
la ejercita **sin base de datos**: resultado resuelto, `not_resolved`,
`not_found`, y tres fallas reales distintas (`version_unreadable`,
`file_open_failed`, `closed`). Un clasificador que sólo corre contra un store
sano nunca demostró distinguir una falla de una búsqueda incontestable.

Lo único que se agregó dentro de la región medida son cuatro contadores y la
llamada a `classify`. Queda declarado porque la alternativa —un solo
`has_value()`— confunde las dos cosas.

### La verificación de contenido es una segunda pasada

Hay que decirlo con precisión: la verificación **no** inspecciona lo que recibió
el bucle cronometrado —esos resultados ya no existen—. **Repite la misma
secuencia de sondeos** después de parar el reloj y comprueba las respuestas. Por
lo tanto establece que el fixture responde correctamente para esa secuencia, no
que las llamadas medidas lo hicieron.

Y una corrección: antes todas las entradas compartían un único payload, de modo
que comparar bytes **no podía** detectar una respuesta que trajera el valor de
otra clave — los bytes eran idénticos. Una versión anterior de este documento
afirmaba que sí. Ahora cada payload codifica su propio índice
(`value_for(n, size)`, tamaño fijo para no mover la clase), y la comparación es
contra el payload de la clave que se pidió. `--selftest-wrong-payload` compara
contra el payload del **vecino** y debe fallar: verificado, exit 3 con «4000
answers carried the wrong entry's payload».

**Política familiar.** Una métrica por corrida. El cuantil familiar de R3a
corrige la multiplicidad de una familia de benchmarks; con una sola métrica no
aplica, así que los intervalos de abajo son **por métrica** y están rotulados
como tales. Aplicar una política familiar exige declarar la familia completa
antes de medirla.

## Resultados — 0 muestras inutilizables en ninguna de las dos corridas

**Las dos métricas son una familia de dos**, aunque se hayan corrido con dos
comandos: separarlas no elimina la multiplicidad. Con dos intervalos
independientes al 95 %, la probabilidad de que al menos uno se desvíe es cerca
de 1 en 10, no 1 en 20. Así que se reportan los intervalos por métrica **y** una
banda simultánea declarada: Bonferroni al 97,5 % por métrica, que alcanza para
una familia de dos y no supone nada sobre cómo covarían.

Recalculado desde los crudos existentes con `scripts/recompute_aa_family.py`;
no se volvió a medir para cambiar la estadística.

| métrica | mediana | IC por métrica (95 %) | IC simultáneo (97,5 %) | ns/op típico |
|---|---|---|---|---|
| `find_hit` | −0,329 % | [−0,633 %, +0,363 %] | [−0,665 %, +0,712 %] | ~28–39 |
| `find_miss` | +0,050 % | [−0,183 %, +0,286 %] | [−0,236 %, +0,412 %] | ~20 |

Peor límite superior simultáneo **nominal**: **+0,712 %**.

> **Retirada de una afirmación.** Una versión anterior concluía acá que los
> anchos eran «compatibles con un margen de +2 %». Bajo las reglas endurecidas de
> la ronda 3, estos crudos **no sostienen ese veredicto**: son del esquema
> `utxoz-aa-pilot-1`, que no lleva registros de muestra ni hash de configuración,
> de modo que no hay con qué comprobar que las razones se derivan de pares A/B
> reales. `family-exploratory-v3.json` los marca `legacy-exploratory` y deja el
> margen en **NOT ASSESSED**.
>
> Los intervalos siguen siendo los que se midieron. Lo que no se puede hacer es
> sacar de ellos una conclusión sobre el margen.

**Bonferroni es un ajuste nominal.** Ensancha cada intervalo de modo que, *si
cada uno cubriera a su tasa nominal*, la afirmación familiar valdría al 95 %. Un
bootstrap percentil de una mediana sobre veinte pares puede subcubrir, y entonces
la familiar también subcubre. Por eso dice «nominal» y no se afirma que la
afirmación familiar valga al 95 %.

Detalle en `calibration-aa-01/family-exploratory-v3.json`.

### Qué exigía, desde la ronda 3, un informe para que se le crea

`recompute_aa_family.py` sólo etiquetaba `verified-current` un informe cuyo (desde
#149 además tiene que traer `provenance_after`; ver la nota de arriba):

- config esté completa y **hashee a sí misma** — un campo editado después de
  escribir el hash lo delata;
- `pairs` sea un entero en 1..20 — ausente o como cadena ya no evade la
  comparación;
- `failures` sea una lista **explícitamente vacía**;
- `samples` tenga exactamente `2·pairs` registros, con `(par, lado)` únicos,
  `exit == 0`, `timed_out == false` y `ns_per_op` finito y positivo;
- crudos presentes, que **hasheen a lo que el informe registró** y que vuelvan a
  pasar por el mismo `parse_sample` del runner, con el `expect` reconstruido de
  la config.

Y las razones se **recalculan desde las muestras**; las que trae el documento se
comparan pero nunca se usan. Un `ratios` es una salida, y confiar en una salida
es por donde entra una fabricada.

Si los crudos no están, el informe se lee pero queda `summary-only` y **no se
emite veredicto de margen**. Si la config es legacy, queda
`legacy-exploratory` y tampoco. Sólo una familia enteramente `verified-current`
recibe un veredicto.

Comprobado entonces en los dos sentidos: una corrida real de 2 pares con el esquema
de ese momento salió `verified-current` y **sí** obtuvo veredicto —`False`, porque dos
pares dan un límite superior de +2,403 % y no resuelven 2 %—, en
`calibration-aa-01/smoke-2pairs-verified.json`. Esa corrida es un smoke de
funcionamiento, **no** una inferencia.

## Cambio de protocolo, declarado

La primera corrida usó 200 000 ops por muestra: región medida de 7,84 ms.
Mirando ese resultado decidí que la región era demasiado corta para sostener
nada y la subí a 10 000 000 —361 ms y 199 ms—, con lo que el semiancho de
`find_hit` bajó de 0,773 % a 0,498 %.

Eso ocurrió **después de ver resultados**, así que las dos corridas juntas son un
ejercicio exploratorio y no una corrida de 20 pares predeclarada. Las dos quedan
guardadas: `find_hit-short-200k-ops/` y `find_hit/`. Una confirmación con método
congelado es una unidad aparte, a correr antes de usar cualquier número de acá
como criterio.

## Qué devuelve un miss en este fixture

Medido, no supuesto: **`not_resolved`, no `not_found`** (2000 contra 0). De modo
que `find_miss` mide «no contestable desde las versiones activas» y **no**
«ausencia probada». Con una sola generación la distinción no cambia el costo,
pero cambia lo que la métrica nombra, y con historia cambiarían las dos cosas.

## Qué permite y qué no afirmar

**Permite:** sólo describir lo medido. En este punto de operación los límites
superiores del A/A fueron +0,363 % y +0,286 % (simultáneo nominal +0,712 %). Por la
retirada de arriba, no se saca de eso ninguna conclusión sobre un margen de +2 %: el
margen queda **NOT ASSESSED**.

**No permite:**

1. **No es una garantía sobre falsos positivos.** Es la precisión *observada* de
   este piloto. Un IC del 95 % por métrica no es una afirmación del 95 % sobre
   una sesión de varias métricas; eso es lo que corrige el cuantil familiar, y
   acá no se aplicó porque hay una sola métrica.
2. **No se afirma potencia.** No hay control positivo. Sin inyectar un efecto
   conocido y verificar que el instrumento lo detecta, «resuelve 2 %» es una
   afirmación sobre el ancho del intervalo y no sobre la capacidad de hallar un
   efecto de ese tamaño.
3. **Calibra un punto de operación, no todos.** 90 000 entradas en un archivo,
   caché caliente: eso cabe en caché. No dice nada de un store histórico de
   1,3 GiB con 37 generaciones, donde el costo lo dominan fallos de TLB, lecturas
   de páginas no residentes y recorridos por generaciones.

## Métricas pendientes, con su razón

| métrica | estado | por qué |
|---|---|---|
| `find_hit` | **medida** | de sólo lectura; estacionaria por construcción |
| `find_miss` | **medida** | ídem |
| `insert` | pendiente | muta el fixture: cada repetición corre contra otro store. Exige restaurar la imagen entre repeticiones, que es otro arnés |
| `apply_deletes` | pendiente | ídem, y además una clave ya borrada pasa de `erased` a `absent`, cambiando la operación medida |
| `resolve` | pendiente | de sólo lectura, pero su costo depende del estado de la caché de archivos y de cuántas generaciones haya; exige declarar frío/caliente y una población con historia, que es el siguiente punto de operación |

## Siguiente paso recomendado

El punto de operación que falta es el que importa: **población con historia**.
Concretamente, un fixture con varias generaciones de la clase 48 y una
población que no quepa en caché, midiendo `find_hit`, `find_miss` y `resolve`
con caché fría y caliente declaradas por separado.

Y antes de usar cualquier piso como criterio, un **control positivo**: inyectar
un retardo conocido —del orden del 2 %— y comprobar que el instrumento lo
detecta. Sin eso, el piso describe el ancho del intervalo y no la capacidad de
hallar un efecto.
