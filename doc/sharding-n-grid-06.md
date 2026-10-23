# Grilla N = 1, 2, 4, 8 en el punto histórico — TAREA 06

> **Al cierre de 1.0:** N quedó fijo en 8 por decisión de producto
> (`doc/roadmap-1.0.md`). Estas mediciones siguen valiendo tal como están: en un hilo,
> N=8 resultó más caro que N=1, y así se lo declara en 1.0.

Protocolo predeclarado antes de la primera medida:
`grid-06/protocol.md`, sha256
`cfe21a7fabf5d171f56e5b42bf5c286ff5f553a446148772879e0121bd91e211`, sellado
`2026-10-23T14:16:54Z`, sobre la fuente `33c481e`. Datos fuera del repo, en una
ruta nueva: `/home/fernando/dev/utxo-z/grid-06/`. Nada sobrescrito, todo preservado.

## Resumen

- **A/A N1/N1: INCONCLUSIVE para 2 %.** El peor límite superior simultáneo nominal
  es +3,45 %.
- **Control +5 % de trabajo: detectado** en las dos métricas.
- **Grilla: todas las celdas salen *peor*, con efectos grandes y monótonos en N.**
  N=2 cuesta +9,8 % en `hist_hit` y +18,3 % en `hist_miss`; N=8 cuesta +51 % y +69 %.
  El menor de esos efectos es casi tres veces el ancho observado del A/A, y el resto
  lo supera por mucho.
- **En lo medido, N=1 domina:** es más barato que todo N>1 en las dos métricas.
  N=2 es sólo el menos malo de los N>1, no un ganador del sharding. **N final
  sigue abierto:** los beneficios por los que se quiere sharding (inserción,
  rotación y compactación por shard) no se midieron, y elegir con evidencia de un
  solo lado sería forzar una decisión de producto.
- **Diagnóstico por contadores, revisión 1:** con un lote fijo, N1 y N8 dan las
  mismas respuestas y la misma cantidad de búsquedas por clave, pero N8 abre 8
  veces más archivos por lote (464 contra 58). Ver abajo.

## Lo implementado

**Perfil experimental `UTXOZ_TESTING_RUNG_EXACT`, sólo de testing.** Cada shard de la
clase shardeada recibe el rung de N=1 dividido por N, en el archivo más chico que,
según la misma `segment_bytes_needed` que ya existía, lo contiene. Es lo que da la
regla `divide` de producción hasta N=8. En N=1 no tiene efecto, y nunca toca el
perfil productivo.

- Dos `static_assert` comprueban la regla en los dos sentidos: el rung está en la
  escalera de Boost, el archivo lo contiene, y es el rung más ancho que ese archivo
  contiene. Un tercero exige que en N=1 la tabla sea la del perfil de testing, sin
  cambios.
- **Conflicto de opciones:** CMake rechaza `UTXOZ_TESTING_DIVIDE_FILE=ON` junto con
  `UTXOZ_TESTING_RUNG_EXACT=ON` (exit 1, `logs/cmake-conflict.txt`). Un `-D` directo
  con ambos no compila (`logs/cxx-conflict.txt`); el control con uno solo sí.
- **Tamaño validado con un store real**, no con una fórmula empírica nueva. El censo
  reporta segmento libre en toda generación: como mínimo 7 644 B en N=8, 7 660 B en
  N=2 y N=4, y 32 336 B en N=1.

**Arnés `calib_historical.cpp`, documento v2:**
- `historical_prefix` sale de un barrido de `find()` sobre las 6 M claves, porque con
  N>1 las activas no forman un sufijo contiguo. El total de activas se contrasta con
  el que da el censo.
- `--probe-below` fija el span de sondas.
- `probe_digest` (FNV-1a) identifica la secuencia exacta de sondas.
- El censo incluye el detalle por shard.

**Runner `run_hist_grid.py`:**
- Cada lado tiene su binario y su fixture, hasheados, con su shape y su censo.
- El `expect` sale de la config de cada lado: N, geometría, prefijo, span, trabajo
  extra y semilla.
- Cada par exige el mismo digest en A y B, salvo en el control.
- Rondas intercaladas entre celdas, con bloques AB, BA, BA, AB desacoplados de la
  paridad de la semilla.
- Exporta el contrato por lado que usa el verificador.
- `recompute_aa_family.py` nombra al miembro de la familia por su celda cuando el
  informe la tiene; el resto de los informes se verifica igual que antes.

**Pruebas:**
- `test_rung_exact.cpp`: la tabla por slot y la creación y rotación reales de un
  shard en el umbral de su rung, en N = 1, 2, 4 y 8.
- `test_hist_grid.py`: 16 casos, entre ellos N, geometría, span o digest distintos
  por lado, orden alterado, store movido y control mal declarado.
- `test_hist_pilot.py` 58/58 y `test_recompute_family` siguen verdes.
- La revalidación del estacionario aceptado da cifras idénticas a su `family.json`,
  y la de TAREA 04 (v1) sigue verificando.

**Un defecto de prueba que el perfil hizo visible.** `small_with_room_to_grow()`
comprobaba que cupiera sólo el peldaño siguiente, pero un rehash de Boost mantiene
viva la tabla vieja mientras reserva la nueva. Con el archivo justo del rung exacto,
"with room to grow…" fallaba con `bad_alloc` en N=4 y N=8. Ahora el helper exige que
quepan las dos tablas.
- No cambia los valores de `keep`, N=1 ni `divide` N=8.
- `divide` N=4 tenía la misma falla latente, y no existe ningún build de ese perfil.

**Pruebas dirigidas** `[profile]`, `[capacity]`, `[shard]`, `[rotation]`, sin `[.slow]`,
44 casos por build:

| | N=1 | N=2 | N=4 | N=8 |
|---|---|---|---|---|
| con aserción fallida | 0 | 0 | 0 | 0 |
| sólo salteados (telemetría en `basic`, o casos que piden N>1) | 5 | 4 | 4 | 4 |

Los dos casos del perfil corrieron y pasaron en los cuatro N.

## Los fixtures

Mismas 6 000 000 claves, mismo payload, misma altura y mismo orden, uno por N,
construido por el binario de ese N. El censo medido coincide **cifra por cifra** con
la predicción aritmética de TAREA 04.

| N | geometry_id | generaciones | por shard | rung | entradas/gen | activas | prefijo histórico | archivo/shard | MiB clase 48 | MiB total |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 | 3 | 59 | 59 | 122 879 | 102 143 | 75 706 | 5 924 294 | 10 485 760 | 590,0 | 630,0 |
| 2 | 0x80000010 | 118 | 59 | 61 439 | 51 071 | 75 764 | 5 922 825 | 5 234 604 | 589,1 | 629,1 |
| 4 | 0x80000011 | 236 | 59 | 30 719 | 25 535 | 75 880 | 5 917 345 | 2 621 356 | 590,0 | 630,0 |
| 8 | 0x80000012 | 472 | 59 | 15 359 | 12 767 | 76 112 | 5 914 791 | 1 314 732 | 591,8 | 631,8 |

La historia es la misma, repartida en N shards: 59 generaciones por shard, ~590 MiB
y ~76 k activas en los cuatro casos. **Prefijo común = el mínimo medido =
5 914 791.** Todas las sondas `hist_hit` de todos los lados salen de ahí, y el digest
de sondas es idéntico entre los cuatro binarios.

## Resultados

Estimador predeclarado: bootstrap percentil de la mediana de ratios apareados B/A,
3000 remuestreos, semilla 5. 20 pares por celda.

**A/A N1/N1** (familia de 2, simultáneo nominal 97,5 %):

| | mediana | simultáneo nominal |
|---|---|---|
| `hist_hit` | +1,60 % | [−3,57 %, +3,45 %] |
| `hist_miss` | +0,40 % | [−3,27 %, +3,05 %] |

Peor límite superior **+3,45 % > 2 %: INCONCLUSIVE para 2 %.** Por la regla
predeclarada, ninguna celda de la grilla puede declarar "no-regresión dentro de
2 %". Esta corrida usó otro orden de bloques que la de TAREA 04, y los anchos son
comparables (±3,5 % contra ±4,9 %); no se atribuye causa a la diferencia.

**Control +5 % de trabajo real** (familia de 2, 97,5 %):

| | mediana | simultáneo nominal |
|---|---|---|
| `hist_hit` | +2,44 % | [+1,85 %, +8,46 %] |
| `hist_miss` | +5,33 % | [+2,05 %, +8,23 %] |

Los dos excluyen 0: **detectado**. +5 % de trabajo no es +5 % de tiempo, y esto no
cuantifica potencia.

**Grilla** (familia de 6, IC individual 95 %, simultáneo nominal 99,1667 %). B/A, con
B = N binario sobre su fixture y A = N=1:

| N | MiB clase 48 | archivos clase 48 | rung/shard | activas | `hist_hit` Δ [simultáneo] | `hist_miss` Δ [simultáneo] | clasificación | validez |
|---|---|---|---|---|---|---|---|---|
| 2 | 589,1 | 118 | 61 439 | 75 764 | **+9,77 %** [+8,89, +10,44] | **+18,27 %** [+17,91, +18,70] | peor / peor | 40/40, 0 fallas, 20/20 digests |
| 4 | 590,0 | 236 | 30 719 | 75 880 | **+25,53 %** [+24,94, +26,76] | **+39,01 %** [+38,18, +39,65] | peor / peor | 40/40, 0 fallas, 20/20 digests |
| 8 | 591,8 | 472 | 15 359 | 76 112 | **+51,24 %** [+49,79, +53,83] | **+69,20 %** [+67,90, +71,64] | peor / peor | 40/40, 0 fallas, 20/20 digests |

Ningún límite inferior simultáneo baja de +8,89 %, casi tres veces el ancho
observado del A/A (±3,5 %). No son diferencias chicas ni caen dentro de la dispersión
del instrumento.

**Validez operativa** de las dos corridas:
- 400 muestras, 0 fallas, 0 inválidas, 0 errores reales.
- 0 sondas contestadas por versiones activas.
- 64 320 000 operaciones medidas.
- Las cuatro imágenes sin cambios.
- Orden ejecutado igual al declarado (`schedule.json`).
- Las tres familias salen íntegramente `verified-current` con el contrato por lado
  (exit 0).

## Lo que esto dice, y lo que no

- **Dice:** en este punto (perfil de testing con rung exacto, 6 M entradas, 59
  generaciones por shard, caché de archivos de un archivo, lotes de 4 000, un hilo,
  CPU 2), resolver lookups históricos cuesta más con más shards, y bastante más.
- **No dice por qué.** Hay un hecho estructural, leído del código y del censo y no
  medido como causa:
  - Un lote de 4 000 claves toca los N shards.
  - Cada shard tiene sus 59 versiones.
  - El caché de archivos guarda uno solo.
  - Entonces un lote abre del orden de 58·N archivos (58 en N=1, ~464 en N=8), cada
    uno 1/N más chico.
  - En cambio, la cantidad de búsquedas por clave no cambia con N, porque con el
    enrutado de TAREA 05 cada clave se busca sólo en las versiones de su shard.

  La hipótesis compatible es que domina un costo fijo por archivo abierto. En la
  revisión 1 se contaron archivos y búsquedas (sección siguiente): el conteo
  estructural se confirma. Que ese conteo sea la **causa** del tiempo no está
  probado: los contadores no miden tiempo.
- **No mide el beneficio del sharding.** Inserción, rotación y compactación por shard,
  y concurrencia, quedan fuera. Ese beneficio es la razón de existir del sharding.
- **No es producción.** La regla es la de producción (rung₁/N); los tamaños no:
  10 MiB contra 1340 MiB por generación. Con generaciones 134 veces más grandes, el
  peso relativo de un costo fijo por archivo debería ser mucho menor, y eso no se
  midió.
- **No hay afirmación de potencia.** Bonferroni es nominal.

## Revisión 1: lo que la regla predeclarada daba, y por qué no es una recomendación

La regla predeclarada ("el N más grande que no salga *peor*; si todos salen peor, el
de menor costo") da N=2. **Se retira como recomendación.**
- La regla compara sólo N>1 entre sí.
- En lo medido, **N=1 domina en las dos métricas**.
- N=2 es el menos malo de los N>1, no un argumento a favor de shardear.

El sharding se quiere por beneficios que esta grilla no mide: inserción, rotación y
compactación más acotadas por shard. Elegir N con evidencia que sólo ve el costo
sería forzar una decisión de producto. **N final queda abierto**, y lo que falta para
decidirlo está en el plan de abajo.

## Revisión 1: diagnóstico por contadores, sin reloj

Un lote fijo: el primero de la secuencia de sondas, semilla 1, span 5 914 791, 4 000
claves. Se resuelve en un proceso con el caché de archivos vacío, y se leen los
contadores de la biblioteca. Builds `build-diag{1,8}`: las mismas fuentes y opciones
que `build-grid{1,8}` salvo `UTXOZ_STATISTICS_LEVEL=lookup`, que hace falta para que
los contadores existan. **No es una ventana cronometrada.** Los binarios medidos de
`build-grid*` no se recompilaron (hashes verificados), y los stores quedaron
intactos.

| | N | archivos abiertos | cache hits | búsquedas (`generations_probed`) | búsquedas por clave | búsquedas por archivo | respondidas |
|---|---|---|---|---|---|---|---|
| `hist_hit` | 1 | 58 | 0 | 116 423 | 29,11 | 2 007 | 4 000 |
| `hist_hit` | 8 | **464** | 0 | 116 412 | 29,10 | 251 | 4 000 |
| `hist_miss` | 1 | 58 | 0 | 232 000 | 58,00 | 4 000 | 0 (4 000 ausentes) |
| `hist_miss` | 8 | **464** | 0 | 232 000 | 58,00 | 500 | 0 (4 000 ausentes) |

**Las respuestas son idénticas entre N1 y N8.** Hay un digest de claves, alturas y
payloads de lo encontrado, y de las claves ausentes, ordenadas por clave. Las sondas
también son idénticas: mismo `probe_digest`.

Lo que los contadores establecen:
- **La cantidad de trabajo de búsqueda por clave no cambia con N.** En misses son
  exactamente 58 por clave en los dos. En hits, 116 423 contra 116 412: la profundidad
  de cada clave dentro de su shard no es idéntica a la que tiene en la historia única.
- **Cambia la cantidad de archivos abiertos por lote, ×8**, y con ella las búsquedas
  por archivo, ÷8. Con el caché de un archivo, cada uno de esos archivos se mapea en
  cada lote.

Lo que no establecen: que esa diferencia de archivos sea la que produce el +51 % /
+69 % de tiempo. Es la diferencia estructural observada, no una medición de causa.

Una corrección de mi propio diagnóstico: la primera corrida comparó mal los hits.
`flat_map` es `boost::unordered_flat_map`, y su orden de iteración sigue al de
inserción, que cambia con el recorrido. El digest ahora ordena por clave. Las dos
corridas están preservadas en `review1/diagnose/` y `review1/diagnose2/`.

## Plan para la próxima unidad: medir los beneficios (sólo plan, sin ejecutar)

Lo que se espera ganar con shards, y que esta grilla no vio:
- **Rotación más chica:** cada shard rota un archivo 1/N más chico.
- **Compactación acotada:** un shard a la vez, así que el pico de bytes y el tiempo de
  cada compactación bajan con N.
- **Pausas máximas más cortas en inserción**, porque ninguna operación individual
  toca un archivo de clase completo.

Ya está medido que la lectura histórica cuesta más con N. Lo que falta es el otro
lado de la balanza.

**Operating points.** Pocos, cada uno en el mismo estado lógico y con los mismos
conteos, partiendo de los fixtures de esta unidad (6 M entradas, rung exacto):

1. **Inserción de punta a punta** de K claves nuevas, las mismas en todos los lados,
   en dos puntos:
   - (a) **K sin rotación en ningún lado**, que mide el costo de inserción puro.
   - (b) **K que fuerza rotación**, por ejemplo K = 102 143. N1 rota una vez; en N8,
     cada shard rota alrededor de una vez.

   Se registran las rotaciones que hizo cada lado. **Si un lado rota y el otro no, el
   trabajo no es comparable y se declara así; no se resta nada.**
   - Métricas: throughput; latencia por inserción p50/p99/max; la pausa individual
     más larga; bytes escritos (censo antes y después).
2. **Compactación.** Primero, fuera del tiempo, un `apply_deletes` determinista del
   mismo 50 % de claves en todos los lados, para dejar generaciones esparcidas.
   Después se mide `compact_all`:
   - tiempo total;
   - pico de bytes en disco durante la compactación (archivos `.building`);
   - la compactación individual más larga;
   - archivos fusionados y bytes finales.
3. **Borrado histórico** (`apply_deletes` de un lote fijo), con el enrutado de TAREA 05
   y la misma restauración.

**Restauración de estado por proceso, fuera del tiempo.**
- La imagen de partida se copia a un directorio nuevo por muestra, con `cp
  --reflink=auto`.
- La fuente se hashea una vez, antes y después de la campaña.
- En cada copia se verifican la cantidad y el tamaño de archivos, y el censo, antes de
  arrancar el reloj.
- Cada muestra muta su copia, y la copia queda preservada para auditarla. El
  presupuesto de disco se declara antes.
- El harness informa los conteos que deben coincidir: claves insertadas o borradas,
  rotaciones, generaciones al final y entradas al final.

**Método.** El ya aceptado: pares fijos, bloques AB/BA/BA/AB, semillas por par, el
estimador y la verificación por lado, con `check_sample_record` contrastando crudo y
registro. **No** se busca resolver ±2 %. Se informan las diferencias con sus IC y se
decide con efectos grandes en las dos direcciones: costo de lectura contra beneficio
en escritura y compactación.

**Fuera de alcance:** locks, pinning, caché de archivos, KTH, formato final, host.

**Qué se decidiría con eso.**
- Si los beneficios en pausa máxima o pico de compactación son grandes y el costo de
  lectura es el medido acá, la decisión de N es un trade-off explícito para producto.
- Si los beneficios no aparecen, N=1 domina también ahí.
- Si el costo de lectura resultara decisivo, la palanca candidata sería cómo un lote
  abre archivos con N shards (agrupar por shard, o el caché). Tocar el caché no está
  autorizado; sería otra decisión.

## Qué falta para el PR

- **Sanitizers** (ASAN/UBSAN) sobre el enrutado de TAREA 05 y el perfil rung-exacto,
  en N=1 y N=8.
- **Mutaciones del perfil:** quitar el `if` de rung exacto y mover el archivo un
  peldaño. Del enrutado, M5 sigue sin cubrir, como ya se documentó.
- **Suite completa una vez**, sobre el commit consolidado, en la geometría enviada
  (N=1) y en N=8.
- **Consolidar los commits** de sharding en uno para el PR único, y la decisión de N
  final (producto).
- **Cobertura final** de las rutas nuevas.

## Rutas

- Datos y logs: `grid-06/`.
  - Protocolo y specs: `protocol.md`, `spec-pre.json`, `spec-grid.json`.
  - Fixtures y censos: `fixtures/N{1,2,4,8}`.
  - Corridas: `run-pre/`, `run-grid/`. Cada una tiene `schedule.json`, una
    `config.json` y un `report.json` por celda, y `raw/` con JSON, stderr y comando
    de cada muestra.
  - Familias: `family-{aa-final,control-final,grid}.json`.
  - Pruebas dirigidas: `tests/`.
  - Configure y build de cada N: `logs/`.
- Builds: `build-grid{1,2,4,8}/`, fuera del repo, ignorados localmente.
