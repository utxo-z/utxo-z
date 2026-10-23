# Plan para comparar N = 1, 2, 4, 8 en el punto histórico

Para revisión. **Nada de esto se construyó ni se midió**: no hay binarios
candidatos, fixtures por geometría ni corridas. Las cifras de geometría salen de
transcribir a aritmética las fórmulas del código (`key_of`, `shard_of`,
`segment_bytes_needed`, `widest_rung_within`, umbral 133/160) y aplicarlas a las
mismas 6 M claves. El script y su salida están en `hist-04/review1/`
(`predict_grid.py`, `predict_grid.json`, `predict_grid_exact.json`). El censo que
se tome al construir cada fixture manda sobre estas predicciones.

## Qué tiene que ser igual entre geometrías

Una comparación entre N vale si lo único que cambia es N. Fijo para todas:

- **Las mismas entradas**: `key_of(n)` para n en [0, 6 000 000), el mismo payload
  `value_for(n)`, la misma altura `1000 + n/1000`, insertadas en el mismo orden.
- **Las mismas sondas**: n uniforme, sin reemplazo dentro del lote, desde un
  **prefijo histórico común** a las cuatro geometrías (abajo); las mismas semillas
  por par. Los misses son las mismas claves desde 2^40.
- **El mismo lote** (4 000) y las mismas ops, warmup y CPU.

Lo que cambia con N, y es lo que se quiere medir: cómo se reparten esas entradas
en shards y generaciones, y cuánto cuesta encontrarlas.

## Ningún perfil existente mantiene la distribución comparable

Predicción por perfil para las mismas 6 M claves (clase 48):

| perfil | archivo por shard | rung | entradas/gen | generaciones | gens por shard | activas | MiB clase 48 |
|---|---|---|---|---|---|---|---|
| N=1 (cualquiera) | 10 485 760 | 122 879 | 102 143 | 59 | 59 | 75 706 | 590 |
| `keep` N=2 | 10 485 760 | 122 879 | 102 143 | 60 | 30 | 75 706 | 600 |
| `keep` N=4 | 10 485 760 | 122 879 | 102 143 | 60 | 15 | 279 992 | 600 |
| `keep` N=8 | 10 485 760 | 122 879 | 102 143 | 64 | 8 | 279 992 | 640 |
| `divide` N=2 | 5 242 880 | 61 439 | 51 071 | 118 | 59 | 75 764 | 590 |
| `divide` N=4 | 2 621 440 | 30 719 | 25 535 | 236 | 59 | 75 880 | 590 |
| `divide` N=8 | 1 310 720 | **7 679** | 6 383 | 944 | **118** | 25 512 | **1180** |

- **`keep`** (el default de testing) multiplica la capacidad activa por N: la
  historia por shard cae de 59 a 8 generaciones y el activo sube de 75 706 a
  279 992 entradas. Comparar N así mezcla N con capacidad.
- **`divide`** (la regla de producción) es comparable en N=2 y N=4, pero en N=8
  **pierde un peldaño**: el archivo de 1 310 720 bytes no alcanza para 15 359
  slots, que necesitan 1 314 732 con la holgura fija de 8 KiB. Queda en 7 679, con
  el doble de generaciones por shard (118) y el doble de bytes (1180 MiB).

**En producción eso no pasa.** Con `divide` sobre 1340 MiB, N = 1, 2, 4 y 8 dan
exactamente el rung de N=1 dividido por N (15 728 639 → 1 966 079 en N=8). La
pérdida de peldaño es del archivo de 10 MiB del perfil de testing.

### Propuesta: un perfil experimental "rung exacto"

Cada shard recibe el rung de N=1 dividido por N, y el archivo más chico que lo
contiene según la misma `segment_bytes_needed`:

| N | archivo por shard | rung | entradas/gen | generaciones | gens por shard | activas | prefijo histórico | MiB clase 48 |
|---|---|---|---|---|---|---|---|---|
| 1 | 10 461 100 | 122 879 | 102 143 | 59 | 59 | 75 706 | 5 924 294 | 588,6 |
| 2 | 5 234 604 | 61 439 | 51 071 | 118 | 59 | 75 764 | 5 922 825 | 589,1 |
| 4 | 2 621 356 | 30 719 | 25 535 | 236 | 59 | 75 880 | 5 917 345 | 590,0 |
| 8 | 1 314 732 | 15 359 | 12 767 | 472 | 59 | 76 112 | 5 914 791 | 591,8 |

59 generaciones por shard en las cuatro, ~75,7–76,1 k entradas activas, ~590 MiB:
la misma historia repartida en N shards. Es lo que hace producción con `divide`.

En N = 1, 2 y 4 el rung coincide con el de `divide`; sólo cambia el tamaño del
archivo, en la holgura sin uso. En N=8 el archivo crece 4 012 bytes sobre
`divide` para no perder el peldaño.

**Requiere un cambio de código sólo de testing** en `capacity_policy.hpp` (una
tabla experimental detrás de una macro propia, como hoy
`UTXOZ_TESTING_DIVIDE_FILE`). **Decisión D1**, no mía: aprobarlo, o medir con
`divide` y declarar que N=8 no es comparable.

## Hallazgo en `full_resolve` que dominaría la grilla

> **Histórico, ya corregido.** Leído de la rama anterior al cierre, en `4fecb04`
> (2026-10-23): las líneas citadas abajo son de esa revisión. TAREA 05 lo corrigió en
> `1e3e9cd` con el filtro `admits` (cada archivo se busca sólo con las claves que puede
> contener), y así está hoy en `master`. Ver `doc/shard-routing-05.md`. El texto que
> sigue es el diagnóstico de entonces, no un defecto actual.

Leído del código, **no medido**. En la fase 2 de `full_resolve`, el lazo recorre
los shards de la clase (`src/database_impl.cpp:3915`) y por cada versión de cada
shard busca **todo** `pending` (`probe_full_file`, el lazo de
`src/database_impl.cpp:3838`). `pending` no se filtra por el shard que cada clave
puede ocupar. El comentario de `:3915` dice lo contrario: "a key can only be in
the shard its own hash names. Walking the others would be work with no possible
result."

Consecuencia esperada con N>1, a confirmar antes de nada:

- un **miss** se busca en las versiones de **todos** los shards: con el perfil
  exacto, 472 archivos en N=8 contra 59 en N=1;
- un **hit** del shard s se busca antes en las versiones de los shards 0…s−1;
- con el caché de un archivo, cada uno de esos archivos se vuelve a mapear por
  lote.

Una grilla medida hoy mediría sobre todo esto. **Decisión D2**, no mía:

- (a) corregirlo primero — filtrar `pending` por shard por slot — como cambio de
  biblioteca aparte, con sus tests; o
- (b) medirlo así y rotularlo.

Recomiendo (a): el comentario muestra que la intención era la otra. Antes de
decidir, confirmarlo con el contador `generations_probed` de las estadísticas en
un test N=8 existente, sin construir candidatos nuevos.

## Cambios mínimos de arnés y runner que la grilla necesita

1. **La frontera activa.** `find_active_boundary` busca binariamente porque en N=1
   el activo es un sufijo contiguo de n. Con N>1 no lo es: cada shard tiene su
   propio sufijo, y la unión no es contigua. Se reemplaza por un barrido completo
   en el modo build — `find` sobre los 6 M n, unos segundos — que registra
   `historical_prefix` = el menor n que `find` resuelve.
2. **Prefijo común.** `--probe-below P` con P = el mínimo de los cuatro
   `historical_prefix`, predicho en 5 914 791 con el perfil exacto. Por
   construcción, todo n < P es histórico en las cuatro geometrías. N=1 pierde
   9 503 claves de su span actual, así que **N=1 se vuelve a medir** con P.
3. **Lado B con otro binario y otro fixture.** `--harness-b` y `--fixture-b` en el
   runner, `shard_count` por lado en `expect_from_config`, y el registro de imagen
   por lado en `verify_report`. Es el mismo contrato, con un campo más por lado.
4. **Orden desacoplado de la semilla.** Hoy pares impares = AB = semilla impar.
   Propuesta, predeclarada: bloques ABBA — AB si r mod 4 ∈ {0, 1}, BA si
   r mod 4 ∈ {2, 3}. Así cada paridad de semilla tiene los dos órdenes (r=1 AB,
   r=3 BA; r=2 BA, r=4 AB), y una diferencia por orden ya no queda confundida con
   la paridad.

## Secuencia, con cada paso condicionado al anterior

1. D1 y D2 decididas.
2. Binarios N = 1, 2, 4, 8 desde el mismo `calib_historical.cpp`.
3. Fixtures, uno por geometría. Censo de cada uno contra la predicción de arriba
   (generaciones por shard, entradas por generación, activas, prefijo). Si una
   difiere, se para y se informa.
4. **A/A N1 de nuevo**, con el orden ABBA y el prefijo común: 20 pares por métrica
   más el control de +5 %, igual que en esta unidad. Si el peor límite superior
   sigue por encima de 2 %, la grilla sólo puede señalar diferencias mayores que
   ese ancho, y se dice antes de medirla.
5. **N vs N1**: N ∈ {2, 4, 8} × {`hist_hit`, `hist_miss`} = familia de 6,
   Bonferroni nominal 99,17 % por métrica, 20 pares cada una. A = binario N1 sobre
   fixture N1; B = binario N sobre fixture N.

## Presupuesto

| | |
|---|---|
| disco | cuatro fixtures de ~590 MiB = **~2,36 GiB**, por encima del tope de 2 GiB |
| medición | 1,5–2 s por muestra en N=1; con D2 sin corregir, un miss en N=8 puede costar hasta 8 veces más. Del orden de 40 min en total, timeout de 300 s holgado |
| builds | cuatro configuraciones de la biblioteca y el arnés, nunca durante una medición |

**Decisión D3**, no mía: subir el tope a ~2,5 GiB (hay 41 GiB libres), o tener
sólo N1 más un candidato a la vez (~1,2 GiB) y borrar el fixture del candidato
después de su corrida, conservando censo, shape y hash. Lo segundo es borrar un
fixture y por eso lo pregunto.

## Lo que esta grilla no va a decidir

- **No elige N.** Ni candidato ni final.
- **No resuelve 2 %** mientras el A/A histórico siga INCONCLUSIVE en 2 %.
- **No es producción.** El perfil exacto reproduce la *regla* de producción
  (rung₁/N), no sus tamaños: 10 MiB por generación contra 1340 MiB.
- N = 16 y N = 32 quedan afuera hasta que haya algo que decida entre los cuatro.
