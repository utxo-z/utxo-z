# Enrutado histórico por shard — TAREA 05

## El defecto

Dentro de la clase shardeada una clave vive en un solo shard, el que nombra su
hash. Los recorridos históricos lo ignoraban: cada versión de cada shard se
buscaba con **todo** el lote pendiente.

- `full_resolve`, fase 1 (caché) y fase 2 (catálogo): `probe_full_file` buscaba
  cada clave pendiente en cada archivo. El comentario de la fase 2 decía lo
  contrario.
- `apply_deletes`, fase 1 y fase 2: el mismo patrón en `step_over_file`.

Los resultados eran correctos: buscar una clave en un shard ajeno no la
encuentra. Pero tres cosas no lo eran:

1. **Trabajo.** Un miss en N=8 leía las historias de los ocho shards.
2. **Fallas ajenas.** Un archivo ilegible de un shard que ninguna clave del lote
   podía ocupar hacía fallar el lote entero (`version_unreadable`), cuando nunca
   hacía falta leerlo.
3. **Telemetría.** `generations_probed` cargaba cada clave a cada archivo. Y había
   un segundo defecto, independiente: la cuenta se acumulaba por **clase**
   (`per_class[Index]`) y se publicaba en `lookup_stats_[index]`, que está
   indexado por **slot**. Con N>1 la historia de la clase 1 aparecía como shard 1
   de la clase 0.

`reference_resolve` no aplica: el modo referencia tiene un solo contenedor, sin
shards.

## El arreglo

- `detail::slot_may_hold(slot, key_shard)` en `shard.hpp`: un archivo de una
  clase no shardeada admite cualquier clave; un shard de la clase shardeada,
  sólo las suyas. Un `static_assert` comprueba la equivalencia para todo slot y
  todo shard del build.
- El shard de cada clave se calcula **una vez** por llamada (`key_shards_of`); en
  N=1 no se calcula nada y todo es elegible por `if constexpr`.
- `full_resolve`: un archivo sin claves pendientes elegibles **no se abre**; dentro
  de un archivo abierto sólo se buscan las elegibles; la fase 2 deja de recorrer
  un shard en cuanto no le queda ninguna elegible.
- `apply_deletes`: lo mismo. `step_over_file` recibe qué claves admite el archivo;
  el de referencia admite todas.
- Telemetría por slot. `generations_probed` cuenta las búsquedas **efectivamente
  hechas**, no las claves elegibles, para que el contador no pueda declarar menos
  trabajo que el que hizo el lazo.
- **El ordinal es el costo de cada clave** (revisión 1). Antes, `probe_ordinal` se
  tomaba de un contador de archivos abierto para todo el lote. Con enrutado, un
  archivo abierto para claves de otro shard no se busca para ésta, así que el
  contador global le cargaba archivos que nunca vio. Ahora hay un contador por
  pedido (índices ya deduplicados) que se incrementa donde se hace `map.find`, y
  es el que se usa para el ordinal. La distancia de versión sigue aparte. En N=1
  los dos contadores coinciden, porque cada archivo abierto busca toda clave
  pendiente.

**Lo que no cambia.** Todas las clases siguen siendo candidatas, porque una
consulta trae la clave y no la clase. `found`/`absent`, la deduplicación del
lote y la independencia del orden siguen igual. Se mantienen el caché global, el
`resolve_mutex_` y el comportamiento fail-closed: una ausencia sólo se declara
después de leer **todo archivo que pudiera tener la clave**, y un archivo
necesario que no abre sigue fallando la llamada. En N=1 el enrutado es la
identidad.

## Pruebas

`tests/test_shard_routing.cpp`, tag `[routing]`, diez casos. Las cifras esperadas
de archivos y probes salen de un modelo explícito del recorrido enrutado, así que
el mismo caso es exacto en N=1 y en N=8.

| caso | qué fija |
|---|---|
| hit y miss en un shard | resultado, altura, archivos y probes sólo de ese shard |
| claves de varios shards en un lote | duplicado, orden directo e invertido, misses; costo exacto |
| clave de una clase no shardeada | se sigue buscando en todas las clases y se cuenta en la suya |
| archivo propio ya en caché | se sigue usando: 1 archivo, 1 cache hit |
| archivo de otro shard en caché | ni se busca ni se cuenta (sólo N>1) |
| archivo que no abre (failpoint) | falla sólo lo que lo necesitaba: hit anterior a él responde, la clave de ese archivo y el miss de ese shard no se declaran ausentes |
| archivo dañado de un shard no pedido | no falla la consulta; el del shard dañado sí (sólo N>1) |
| borrados históricos | varios shards y miss; failpoint en shard ajeno no frena, en el propio queda `unresolved` |
| ordinal por clave, fase de catálogo | hit temprano (ordinal 1), hit tardío en otro shard (ordinal 2), miss; histogramas y probes exactos |
| ordinal por clave, fase de caché | la clave del archivo cacheado tiene ordinal 1, y el archivo de otro shard en caché no infla a la clave del shard 0 |

Las expectativas de los dos casos de ordinal están escritas a mano, clave por
clave, según dónde vive cada una; no salen de ningún conteo de archivos del lote.

### Discriminación: misma fuente de tests, biblioteca con y sin arreglo

La comparación se hizo con una reversión temporal y acotada de mis propios
cambios en `src/`, por parche (`git apply -R`), sin `checkout`. Después de
restaurar se verificó que `src/` quedara idéntico al parche.

| `[routing]` | N=1 (`lookup`) | N=8 `keep` (`lookup`) |
|---|---|---|
| sin arreglo | 6 pasan, 2 se saltean | **7 de 8 fallan** |
| con arreglo | 6 pasan, 2 se saltean | **8 de 8 pasan** |

En N=1 el resultado es idéntico, que es lo que debe dar. Los dos que se
saltean sólo tienen sentido con dos shards o más. Catch2 sale con 4 cuando todo
un caso se saltea, y el runner lo registra con ese código. El que pasa también
sin arreglo en N=8 es el control positivo de caché.

**Rojo preexistente.** `two classes are counted apart`, de
`test_lookup_telemetry.cpp`, **fallaba en N=8 con telemetría antes de este
arreglo** y pasa después. Ningún build lo corría: todos los builds existentes son
nivel `basic`, donde ese caso se saltea.

### Mutaciones, en N=8, `[routing]` + `[telemetry]` (23 casos)

| mutación | resultado | lo detecta |
|---|---|---|
| M1 `resolve` abre archivos sin claves elegibles | detectada | archivo de otro shard en caché |
| M2 `resolve` busca toda clave en todo archivo abierto | detectada | varios shards en un lote (probes) |
| M3 telemetría publicada por clase, como antes | detectada | clase no shardeada; *two classes are counted apart* |
| M4 borrados abren y recorren todos los shards | detectada | borrados históricos |
| M5 borrados prueban toda clave en todo archivo abierto | **sobrevive** | — |

**M5 queda sin cubrir, y es una limitación de esta batería.** No cambia ningún
resultado observable: un `erase` en un shard ajeno no encuentra nada, y los
borrados no tienen contador de búsquedas. Detectarla pediría un contador
productivo nuevo o medir tiempos, y ninguna de las dos cosas se hizo. Lo que hay
es el filtro `admits` presente en `step_over_file`, verificable leyendo el
código, no por una prueba. Estas mutaciones muestran que las pruebas detectan
M1–M4; no son una afirmación de cobertura completa.

M1 sola no rompe los casos de failpoint ni de archivo dañado, porque la fase 2
también corta el recorrido de un shard sin claves elegibles. Son dos capas
independientes, y M1 quita sólo la que protege el camino del caché, que es el
caso que la detecta.

Cada mutación se aplicó sobre una copia guardada del archivo corregido, y la
restauración se verificó por hash.

### Un error de mi propia prueba, corregido antes de medir

La primera versión del caso del archivo dañado hacía
`std::move(*full_db::open_for_testing(...))` sin comprobar el resultado. El open
**rechaza** con `file_open_failed` una base con una versión dañada, así que lo que
seguía era comportamiento indefinido y terminaba en SIGSEGV, con o sin arreglo.
No era la biblioteca. El caso ahora comprueba el open y daña el archivo con la
base ya abierta, que es el escenario de `resolve`: un archivo que se rompe bajo
una instancia en marcha, en un shard que la consulta no necesitaba leer.

## Suites dirigidas

Selección por riesgo, enumerada antes de correr y con `[.slow]` excluido en cada alternativa del OR: `[routing]`, `[telemetry]`, `[deletion]`, `[ownership]`, `[unresolved]`, `[deferred]`, `[rotation]`, `[shard]`, `[failpoint]`, `[failpoints]`, `[statistics]`, `[full]`. Son 123 casos por build, corridos de a uno con el runner persistido (progreso, latido, exit real por caso).

| build | perfil | casos | exit 0 | con aserción fallida | sólo salteados (exit 4) |
|---|---|---|---|---|---|
| `build-lk1` | N=1 keep, lookup | 123 | 121 | **0** | 2 |
| `build-lk8` | N=8 keep, lookup | 123 | 123 | **0** | 0 |
| `build-div1` | N=1 divide, basic | 123 | 107 | **0** | 16 |
| `build-n8` | N=8 keep, basic | 123 | 108 | **0** | 15 |
| `build-div8` | N=8 divide, basic | 123 | 108 | **0** | 15 |

Los salteados son casos de telemetría en builds `basic`, donde los contadores no se compilan, y los dos casos de `[routing]` que piden N>1 en los builds N=1. Catch2 sale con 4 cuando un caso se saltea entero, y el runner lo registra con ese código; ninguno tiene una aserción fallida. No se corrió la suite completa de 385 casos.

### Revisión 1: el ordinal

Misma fuente de tests (los dos casos nuevos), biblioteca de `1e3e9cd` contra la
corregida, `[routing][telemetry]`:

| | N=1 (`lookup`) | N=8 `keep` (`lookup`) |
|---|---|---|
| ordinal global (`1e3e9cd`) | 2 de 2 pasan | **2 de 2 fallan** |
| ordinal por pedido | 2 de 2 pasan | **2 de 2 pasan** |

Con el contador global, en N=8 la clave tardía salía con ordinal 5 (promedio 3,0
en vez de 1,5): se le cargaban el archivo del shard 0 y los dos del shard del
miss, aunque nunca se buscó en ellos. En N=1 los dos contadores coinciden, y por
eso ahí pasa en ambos casos.

Después, `[routing],[telemetry]` completo, 25 casos:

| build | exit 0 | con aserción fallida | sólo salteados (exit 4) |
|---|---|---|---|
| `build-lk1` (N=1, `lookup`) | 23 | **0** | 2 (piden N>1) |
| `build-lk8` (N=8, `lookup`) | 25 | **0** | 0 |

La biblioteca compila sin advertencias en nivel `basic` (`build-n8`,
`build-div1`), donde el contador por pedido no existe. No se repitieron las cinco
suites de 123 casos: el cambio de esta ronda sólo toca la telemetría de
`full_resolve` a nivel `lookup`.

## Límites

- No se midió rendimiento ni se corrió ninguna campaña.
- `build-lk1/` y `build-lk8/` quedan fuera del repo: ignorados en
  `.git/info/exclude` (local, no versionado), sin borrar.
- Builds de prueba nuevos: `build-lk1` y `build-lk8`, con
  `UTXOZ_STATISTICS_LEVEL=lookup` y `keep`. Los existentes `build-div1`,
  `build-n8` y `build-div8` (nivel `basic`) se recompilaron. N=2, 4, 16 y 32 no se
  corrieron en esta unidad.
- No se tocaron el formato, los tamaños, el caché, los mutex ni nada de KTH.
