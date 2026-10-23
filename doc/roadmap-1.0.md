# UTXO-Z 1.0 — roadmap de cierre

**Documento autoritativo del cierre de 1.0.** Si otro documento de `doc/` lo
contradice, manda éste. Los documentos de diseño y de campañas anteriores se
conservan como registro de cómo se llegó acá.

**Estado: READY-FOR-PR, no READY-RELEASE.**
- El código está implementado y validado localmente, según el criterio de §7.0.
- La revisión independiente de Codex está en curso (§7.3) y todavía **no hay
  aprobación**.
- La CI de GitHub (cross-platform) espera al PR.
- La versión, el tag, la publicación y el push están pendientes de autorización.
- La ronda 0 de TAREA 11 se interrumpió el 2026-10-23 a las 19:08 UTC por falta de
  espacio, y se retomó con unos 43 GiB libres (después, unos 90).
- El cierre de código está en `9bdeb73` y `f8c1020`.

## 1. Decisión de producto

Fernando autorizó tres cosas: N fijo en 8, cerrar el PR de UTXO-Z, e integrar KTH
después.

| Decisión | Valor |
|---|---|
| Clase shardeada | **sólo la de 48 bytes** |
| Cantidad de shards en producción | **8, fija**. No se elige en tiempo de ejecución, no se deduce de la cantidad de hilos, y no es una opción de compilación ni de Conan |
| Identidad de formato de producción | **`geometry_id = 4`** (§5) |
| Otros N (1, 2, 4, 16, 32) | sólo para tests y experimentos, con `UTXOZ_EXPERIMENTAL_SHARD_COUNT`. El build queda marcado `experimental_build`, escribe un id `0x8000_00xx` que ningún build de producción acepta, y **no instala nada** (no hay librería, headers ni config de CMake que empaquetar) |
| `UTXOZ_SHARD_COUNT` (la opción vieja) | **se rechaza**: `FATAL_ERROR` en CMake y `#error` en el header público si alguien la define. Nunca se ignora en silencio |
| W de `insert_batch` | configurable (`insert_batch_options::workers`) y acotado a los slots ocupados por el lote. En N=8, W=8 es válido: no se exige W < N |
| Caché de archivos | global, sin cambios: un shard caliente puede usarlo entero |
| `resolve_mutex_` | se conserva |
| Borrados históricos | en serie |
| Compactación | a lo sumo un shard a la vez |
| Concurrencia externa | **no se amplía**: el contrato de threading de `db_base` no cambia. El paralelismo de `insert_batch` es interno a una llamada |

### Lo que la decisión de N=8 no afirma

N=8 es una decisión de producto, tomada con la evidencia de abajo. **No** es el
óptimo universal, y **no** salió de los criterios predeclarados del diseño (los
umbrales ±2 % de §5 y §11 de `sharding-class-48-design.md`): esos criterios no se
cumplieron ni se usaron para decidir. Ver §6.

## 2. Alcance de 1.0

1.0 es lo que está en `origin/master` (`b144903`, que es v0.10.0 más 13 PRs mergeados
y nunca publicados) más la rama `feat/shard-class-48`: 63 commits sobre ese master, y
el cierre de §8.
- **Base real del PR:** `origin/master` = `b144903`, verificado con `git fetch`. La
  rama no está en el remoto.
- No se asume la versión por el nombre de la rama: la que va en el tag la decide
  Fernando al publicar.

**Entrega: cuatro PRs apilados**, cada uno con exactamente un commit sobre el
anterior, que se mergean en orden, uno por uno.

**Re-apuntar no alcanza después de un squash merge.** El merge escribe en `master` un
commit con otro SHA, así que la rama siguiente queda sobre un commit que `master` no
tiene, y un re-apuntado a secas volvería a mostrar el commit del PR anterior. Antes de
mergear el siguiente:
1. rebasarlo sobre el `master` mergeado;
2. verificar que es exactamente un commit sobre `master` y que su diff no cambió con el
   rebase;
3. guardar antes una referencia de respaldo de la rama.

Publicar la rama rebasada la reescribe, así que se hace sólo coordinado, nunca como
paso automático.

| # | Rama | Base | Contenido |
|---|---|---|---|
| 1 | `codex/1.0-sharding` | `master` | N=8 fijo, geometría 4 (breaking), sharding, routing histórico, rotación, compactación y recuperación por slot, censo y `verify_unique` por shard, fixtures, CI de formato y config |
| 2 | `codex/1.0-insert-batch` | PR 1 | `insert_batch`: estado compartido seguro, preasignación, ejecutor, resultados parciales; tests de concurrencia y asignación; el consumidor Conan llama un lote |
| 3 | `codex/1.0-benchmarks` | PR 2 | benchmarks (el blanco grande vuelve a compilar), harness de calibración, runners con verificador estricto y guarda de autorización, tests en Python, perfil de test rung-exact |
| 4 | `codex/1.0-docs` | PR 3 | README, este roadmap, contratos, metodología y resultados |

Algunas piezas no van donde su tema sugeriría, por dependencia:
- **El fix de los nombres de archivo por shard de `storage_overhead_report`** va en el
  PR 1, porque el blanco de benchmarks regular lo compila y sin él lee mal N=8.
- **El blanco grande de benchmarks** va entero en el PR 3. En `master` no compilaba
  (le faltaban los headers internos y llamaba a un `erase()` que ya no existe), así
  que el PR 1 se valida con ese blanco apagado, como en `master`.
- **El perfil rung-exact** va en el PR 3: es un perfil de test para comparar N, y sólo
  lo usan los harness.

**Fuera de 1.0:**
- la integración con KTH, que va después y en su propio repositorio;
- el *pinning* del caché (`mapping_handle`) y `is_pinned`;
- la generación activa en memoria;
- borrados históricos en paralelo y `resolve` concurrente;
- cualquier API de concurrencia externa nueva;
- migración de bases viejas. No existe ni existirá: una base de otra geometría se
  reconstruye desde la cadena.

## 3. Inventario de correcciones y cambios, `v0.10.0..HEAD`

Nada de esta tabla salió en un release: lo que está en master está después de
v0.10.0. Los SHA de la rama de desarrollo que citan estas tablas (por ejemplo
`1e3e9cd` o `9bdeb73`) identifican el trabajo en esa rama y no van a estar en el
historial publicado. Ese historial son los cuatro commits de §2, y cada fila cae en uno
de ellos según su tema. Los números de PR (#127 a #142) sí son los de `master`. Los estados posibles son:
- **en master** o **en la rama**: dónde está el código;
- **validado antes del cierre**: pasó en la última corrida completa previa al cierre
  (§7.1);
- **validado con geometría 4**: además pasó en la suite de producción posterior al
  cierre (§7.2).

**Cobertura después del cierre, por prueba y configuración** (no hay una afirmación
universal):
- **V1** corrió en producción (geometría 4, estadísticas `basic`) todo caso no
  `[.slow]` que esa configuración compila y no saltea. Eso incluye los tests nombrados
  en las tablas de §3.1 a §3.6, con estas excepciones:
  - **los 17 casos que necesitan estadísticas `lookup`**: 13 de `test_lookup_telemetry`
    y 4 de `test_shard_routing`. Se saltean en V1 y corrieron en V4 (ASan+UBSan,
    `lookup`), verificados por nombre;
  - **los 2 casos rung-exact**: requieren un perfil de test experimental y después del
    cierre no corrieron en ningún build;
  - **los casos `[.slow]`**: no corrieron;
  - **lo que es revisión estática** y no test, como el índice `size_t` (§3.6);
  - **la evidencia de TSan y de mutaciones de `insert_batch`** (§3.6, `batch-09/`) es
    anterior al cierre. Después del cierre, TSan corrió en V5 sólo sobre
    `[batch]`, `[concurrency]`, `[ownership]` y `[executor]`, con un control positivo;
    las mutaciones A1–A3 y G2 no se repitieron.
- **Nada de esto corrió en macOS, Windows ni la CI de GitHub.**

### 3.1 Formato y configuración

| Defecto o riesgo | Commit | Test o evidencia | Estado |
|---|---|---|---|
| Una base no registraba bajo qué formato se escribió. Un build con otro layout de `unordered_flat_map` la abría reinterpretando los bytes en vez de rechazarla | `dbdedcd` (#128, `feat!`) | `test_format_barrier`, `test_named_object_guard`, `test_reference_mode`, `test_compaction_invariant` | en master · validado antes del cierre · validado con geometría 4 (V1) |
| Un archivo de versión con el objeto nombrado inalcanzable no se rechazaba | `36528b9` (#127) | `test_named_object_guard`, `test_compaction_recovery` | en master · validado antes del cierre · validado con geometría 4 (V1) |
| Faltaba certificar que un build lee lo que escribió otro | `cc1278f` (#130) | `test_format_compatibility`, `test_fixture_generator` y los fixtures `tests/fixtures/epoch1-lp64`, de geometría 3 | en master. En el cierre, el set de geometría 4 (`geometry4-lp64`) pasa a ser el que se lee y el de geometría 3 queda como set legacy que hay que rechazar. `[compat]`: 44/44 en producción (V1b) |
| La clase 2 pasa a 96 bytes (geometría 2) | `0764ce8` (#132, `feat!`) | `test_container_selection`, `test_find_across_rotations`, `test_format_barrier` | en master · validado antes del cierre |
| La capacidad y el tamaño de archivo pasan a ser una sola decisión (geometría 3) | `701b6d1` (#134, `feat!`) | `test_capacity_policy`, `test_compaction_invariant`, `test_format_barrier` | en master · validado antes del cierre · validado con geometría 4 (V1) |

### 3.2 Capacidad, rotación y fallo de inserción

| Defecto o riesgo | Commit | Test o evidencia | Estado |
|---|---|---|---|
| El guard rotaba tarde. Boost baja su umbral de crecimiento cuando se borra (*drift*), así que un insert podía chocar con `bad_alloc` antes de rotar; se observó dos veces en un IBD real. Además, faltaba verificar qué deja un insert fallido | `fff2558` (#140) | `test_insert_transition`, `test_failpoint_reset` (con `21d483f`, #141) | en master · validado antes del cierre |
| El guard de archivo mínimo no salía de las tablas por slot | `d9043b7` | `test_capacity_policy`, `test_compaction_invariant` | en la rama · validado antes del cierre |

### 3.3 Routing histórico (sharding)

| Defecto o riesgo | Commit | Test o evidencia | Estado |
|---|---|---|---|
| `full_resolve` y `apply_deletes` buscaban una clave también en los archivos de los otros shards de su clase: costo y conteos de más | `1e3e9cd` | `test_shard_routing` | en la rama · validado antes del cierre |
| El ordinal de sondeo de cada pedido se cargaba con las búsquedas de otros | `9f567d1` | `test_shard_routing` | en la rama · validado antes del cierre |
| El layout por slot: `shard_of`, archivos por shard, catálogos y recorridos | `db963da`..`f22985f` (WIP, se consolidan en el PR) | `test_shard_of` (con vectores dorados), `test_shard_routing`, y la suite completa en N=1 y N=8 (`63353ac`: barrido de N=1 a N=32) | en la rama · validado antes del cierre |

### 3.4 Recuperación, durabilidad y compactación por shard

| Defecto o riesgo | Commit | Test o evidencia | Estado |
|---|---|---|---|
| La frontera de shard en la compactación (D3: un shard a la vez), y la metadata de un contenedor vacío (D4) | `f8f6c04` | `test_compaction_invariant`, `test_compaction_recovery`, `test_uniqueness` | en la rama · validado antes del cierre |
| El test truncaba una generación que no era la del testigo | `437b7ec` | `test_compaction_recovery` | en la rama · validado antes del cierre |
| `sync()` tiene que cubrir cada slot rotado, también dentro de un lote | `1e34fd5` (`dirty_mutex_` en `note_dirty`) | `test_insert_batch` ("sync after a batch that rotated…") | en la rama · validado antes del cierre |

### 3.5 Unicidad, censo y telemetría

| Defecto o riesgo | Commit | Test o evidencia | Estado |
|---|---|---|---|
| No había forma de verificar que ninguna salida esté guardada dos veces | `466f19f` (#139) | `test_uniqueness` | en master · validado antes del cierre |
| `verify_unique` no informaba el shard de cada entrada, ni marcaba una clave guardada en el shard equivocado (D5) | `f2552f3`, `f32d10a` (exit 2) | `test_uniqueness` | en la rama · validado antes del cierre |
| El censo contaba lo que hizo el proceso, no lo que está guardado | `2e39afa` (#137) | `test_census`, `test_open_for_inspection` | en master · validado antes del cierre · validado con geometría 4 (V1, V7) |
| No se medía cómo se resuelve cada lookup | `9aeb84f` (#138) | `test_lookup_telemetry` (13 casos, más 4 de `test_shard_routing`, que se saltean con estadísticas `basic` u `off`) | en master · validado antes del cierre |

### 3.6 Inserción por lote (`insert_batch`, API nueva)

| Defecto o riesgo | Commit | Test o evidencia | Estado |
|---|---|---|---|
| Una API de lote con un escritor por slot, equivalente al secuencial sin latch, con fallos parciales informados por índice | `1e34fd5` | `test_insert_batch` | en la rama · validado antes del cierre |
| Estado compartido en las inserciones: el latch (mutex), `dirty_versions_` (mutex), el despacho del log (`recursive_mutex`) y el failpoint de rotación (CAS) | `1e34fd5` | TSan: `[batch]`, `[concurrency]`, `[ownership]`, 0 reportes, con control positivo (`batch-09/`) | en la rama · validado antes del cierre |
| El ejecutor recibía una referencia a un temporal: un ejecutor asincrónico usaba memoria liberada | `7f21dfc` (contrato síncrono más un gate compartido) | `test_insert_batch` `[executor]`. Mutaciones G1/G2 detectadas en N8, TSan y ASan (`batch-09/r2/mutations/`) | en la rama · validado antes del cierre |
| Contabilidad que asignaba memoria después de guardar la entrada | `7f21dfc` (`make_room`, preparación) | `utxoz_batch_alloc_tests`, con `operator new` reemplazado. Mutaciones A1–A3 detectadas | en la rama · validado antes del cierre |
| Un índice `uint32_t` truncaba lotes de más de 2³² pedidos | `7f21dfc` (`size_t`) | revisión estática; no hay test de 2³² | en la rama |

### 3.7 Cierre de 1.0

| Cambio | Commit | Test o evidencia | Estado |
|---|---|---|---|
| N=8 fijo y geometría 4, con la tabla fijada en un `static_assert` | `9bdeb73` | `test_shard_of` y `test_format_barrier`; V1 y V3 | validado localmente |
| Rechazo de las geometrías 1, 2 y 3 (la 3 es nueva), en todo build | `9bdeb73` | `test_format_barrier`; `[compat][format]` "a database of another geometry…"; V7 negativo | validado localmente |
| Builds experimentales aislados (id `0x8000_00xx`) y no instalables | `9bdeb73`, `f8c1020` (CI) | V2 y V3; el paso de CI corrido en local | validado localmente; el job de CI no corrió en GitHub |
| El generador recorre por slot, y el set `geometry4-lp64` | `9bdeb73` | generado dos veces con los mismos digests lógicos; `[compat]` 44/44 sin salteos | validado localmente |
| Dos tests de compatibilidad que suponían un solo contenedor de clase 0 | `9bdeb73` | fallaban en la primera corrida de V1 (2 fallas) y pasan en la final | corregido |
| El consumidor Conan: N=8, producción y geometría 4 sin defines, e `insert_batch` | `9bdeb73` | V6 | validado localmente |
| La sonda de Boost apunta al set nuevo | `9bdeb73` | sintaxis (`bash -n`); no se corrió la sonda | pendiente de CI |

### 3.8 Lo que no se atribuye a 1.0

**Correcciones que ya salieron en 0.9.x o 0.10.0** (ver `doc/release-notes/release-notes.md`):
- en 0.10.0: #118 (el llamador es dueño de sus lookups), #123 (resoluciones
  serializadas), #124 (el llamador es dueño de sus borrados) y #115 (la ruta en forma
  nativa);
- en 0.9.1: #110 (la ausencia probada frente al lookup que no se pudo hacer);
- en 0.9.0: la compactación crash-atómica (#70), `sync()` (#80), el lock exclusivo
  (#77), la metadata atómica (#68), el catálogo que falla cerrado (#61) y el duplicado
  en la compactación (#58).

La rama local `fix/unresolved-vs-absent` es anterior a v0.10.0: su trabajo salió en
#110 (0.9.1) y lo reemplazó #118. No forma parte de 1.0.

**Correcciones de KTH:** ninguna está en este repositorio, y ninguna se atribuye acá.

## 4. API de `insert_batch` (resumen)

El contrato completo está en `include/utxoz/database.hpp` y en
`doc/insert-batch-kth-handoff.md`.
- **Para el llamador es una sola mutación.** La llamada es síncrona y hay un escritor
  por slot.
- **El ejecutor tiene que ser síncrono**: tiene que unir sus hilos (`join`), también
  si tira. Una copia de la tarea llamada tarde es inofensiva, pero eso es una red de
  seguridad, no soporte para ejecutores asincrónicos.
- **No es atómica.** El estado de cada pedido se informa por índice, y sólo
  `not_applied` se puede reenviar tal cual.
- **Memoria:** si no puede asignar su contabilidad, tira `bad_alloc` antes de mutar
  nada.
- **No sincroniza.** Un `sync()` posterior cubre lo que el lote rotó.
- **Slots:** el mismo (clave, clase) cae siempre en el mismo slot, y no hay validación
  global de unicidad ni de BIP30.
- **W:** es configurable y se acota a los slots ocupados.

## 5. Formato, compatibilidad y reconstrucción

| Base | Qué hace un build 1.0 de producción |
|---|---|
| Escrita por 1.0, N=8 (geometría 4) | la abre |
| Escrita por v0.10.0 o antes (sin identidad registrada) | la rechaza con `migration_required`. Para decidirlo, `open()` toma el lock de la base, como cualquier apertura, y lee el config. **No modifica datos, segmentos ni config**, y no mapea ningún segmento; el lock sigue su contrato de apertura y cierre |
| Escrita por master entre 0.10.0 y 1.0 (geometrías 1, 2 y 3, nunca publicadas) | la rechaza con `geometry_mismatch` |
| Escrita por un build experimental (ids `0x8000_00xx`, incluidos los de las campañas: `0x80000012` para N=8 y `3` en los N1 viejos de `grid-06`) | la rechaza con `geometry_mismatch` |

**Por qué el id 4.**
- Los ids 1, 2 y 3 ya nombraron geometrías anteriores.
- Ningún archivo de código, test, herramienta ni manifest usa el 4: sólo aparecen los
  experimentales `0x8000_001x` (revisado con `git grep` sobre la rama).
- El diseño (§12 de `sharding-class-48-design.md`) lo reservó para congelar 1.0.
- La geometría 4 es la 3 con la clase de 48 bytes partida en 8: cada shard tiene
  1 966 079 buckets en 167,5 MiB. Las otras cuatro clases quedan sin cambios.
- El cambio la escribe entera, entrada por entrada, en un `static_assert`
  (`geometry_4_slot_capacity`). Mover un tamaño, una capacidad o la cantidad de shards
  obliga a cambiar el id.

**Builds experimentales.**
- Cada N tiene su id en `0x8000_0000`, también N=1 (`0x8000000F`). Así ningún build
  experimental escribe geometría 3 ni se hace pasar por una base anterior a 1.0.
- N=8 no tiene id experimental: es producción.

**Reconstrucción.** Pasar a 1.0 implica reconstruir la base desde la cadena. No hay
migrador.

## 6. Rendimiento: lo medido y sus límites

Todo se midió en una máquina (Ryzen 9 9950X3D), con imágenes sintéticas de 6 M
entradas y **el perfil de test rung-exact**, que no es el de producción (segmentos
chicos en vez de 1340 MiB por clase). Nada de esto es una sincronización de KTH ni un
IBD.

| Campaña | Qué compara | Resultado | Límite |
|---|---|---|---|
| TAREA 06 (`sharding-n-grid-06.md`) | lecturas históricas, un hilo, N = 1/2/4/8 | **N=1 domina.** N=8 cuesta +51 % en `hist_hit` y +69 % en `hist_miss`; abre 8 veces más archivos por lote | el A/A no resuelve 2 % |
| TAREA 07 (`sharding-mutation-07.md`) | `insert_lifecycle` y `compact`, un hilo, N = 1/2/4/8 | **N>1 peor o inconcluso.** En inserción: N=2 inconcluso, N=4 +16,2 %, N=8 +11,9 %. En compactación, regresión nominal en todo N>1: N=8 +135 % | IC simultáneos nominales y 10 pares |
| TAREA 10 (`batch-contrast-10.md`) | `insert_batch` en N8 con W = 1/2/4/8, N1 W1, y `insert()` uno por uno | **Mediana de N8 W1 a W8: 380,560 → 121,503 ms (unas 3,13 veces).** W2 226,929, W4 162,232. N1 W1 264,253 ms; N8 `insert()` 250,897; N1 `insert()` 225,891 | 3 repeticiones, exploratorio; región medida de 0,1–0,4 s; los hilos se crean en cada llamada; lote fijo de 100 k; un solo L3. Prueba participación de los workers, no solapamiento |

**Lectura acotada.**
- **Lo que gana N=8:** sólo cuando la inserción se reparte entre workers, con
  `insert_batch`. En esta carga, N8 W8 fue más rápido que N1 W1 en las tres
  repeticiones.
- **Lo que pierde N=8:** en un solo hilo es más caro que N=1, en lectura histórica,
  inserción y compactación.
- **Qué no se puede decir:**
  - nada sobre la sincronización de KTH: no se midió, y no hay base para "3× más
    rápido sincronizando";
  - nada sobre el perfil de producción;
  - ninguna equivalencia ni ausencia de regresión.
- **Los umbrales de ±2 % del diseño** no se alcanzaron ni se evaluaron para decidir.

## 7. Validación

### 7.1 Antes del cierre (evidencia existente)

- **Suite completa en N=8.** `build-b9-8`: el código de `7f21dfc`, con el id
  experimental `0x80000012`, Release y el perfil de test por defecto.
  - Comando: `UTXOZ_SUITE_OUTDIR=… scripts/run_targeted_suite.py baseline-b9-8 build-b9-8`
  - Log: `batch-11/logs/baseline-b9-8.out`
  - Resultado: **412 casos, 393 OK, 0 fallas de Catch2, 19 salteados** (driver,
    `worst_exit=4`).
  - De los 19 salteados, 15 son los fixtures de geometría 3 (12 de compatibilidad y 3
    del generador), 2 el rechazo de las geometrías 1 y 2, y 2 rung-exact (perfil no
    compilado).
- **`insert_batch`:** `[batch]` y `utxoz_batch_alloc_tests` en N1, N8, ASan+UBSan y
  TSan, exit 0, sin reportes, con mutaciones detectadas (`batch-09/r2/`).
- *(Histórico, cerrado en P9, `9bdeb73`.)* Un comentario de
  `include/utxoz/census.hpp:202` mencionaba `UTXOZ_SHARD_COUNT=1`; ya no.

### 7.0 Criterio de aceptación de una corrida

El driver (`scripts/run_targeted_suite.py`) devuelve el **peor** exit de los casos que
corrió. Catch2 devuelve **4** para un caso que sólo se saltea, así que una corrida con
salteos termina en exit 4 aunque no falle nada.

Una corrida se acepta sólo si se cumplen las cinco condiciones:
1. **cero** aserciones fallidas (`catch2_failed=0`);
2. **cero** timeouts y **cero** errores de infraestructura;
3. exit **0**, o exit **4** sólo cuando el tally es consistente: cada caso con exit 4
   es un salteo puro (`failed` de casos = `skipped_cases`, y no hay ninguna línea
   `FAILED exit=` distinta de 4);
4. todos los salteos **enumerados por nombre y justificados**;
5. ningún exit 4 que no sea un salteo puro: eso sería una falla.

Ningún exit 4 se presenta como exit 0.

### 7.2 Después del cierre

Todo está en `batch-11/logs/`. Los hashes de los binarios de test están en
`final-binaries.sha256`. Cada corrida es del driver
`scripts/run_targeted_suite.py <label> <build> [selección]`, un caso a la vez y sin
`[.slow]`, ejecutado desde un cwd propio dentro de `batch-11/`.

| | Build y configuración | Comando o selección | Resultado | Log |
|---|---|---|---|---|
| Primer build | `build11-prod` (producción) | — | la librería compiló, los tests no (`geometry_id_for_shards`) | `build-prod.log` |
| V1 | `build11-prod`: producción, Release, estadísticas `basic` (la de fábrica) | suite completa | **exit real 4 (sólo salteos)**: tally `failed=19 = skipped_cases=19`, `catch2_failed=0`, 0 timeouts. **414 casos: 395 OK, 0 fallas.** 19 salteados: 17 "statistics are compiled out" y 2 rung-exact. La primera corrida dio 2 fallas (§3.7), corregidas | `v1-prod.out`, `v1-prod-final.out` |
| V1b | `build11-prod` | `[compat],[format],[generator]` | **exit 0** (`worst_exit=0`; el exit del proceso sólo se vio en consola, no está guardado en archivo). 44/44, 0 salteados | `v1b-compat.out` |
| V2 | `build11-exp1`: `UTXOZ_EXPERIMENTAL_SHARD_COUNT=1` | `[shard],[format],[routing],[batch],[compat],[generator],[unique],[census]` | **exit real 4 (sólo salteos)**: tally `failed=22 = skipped_cases=22`, `catch2_failed=0`. 131 OK, 0 fallas. 22 salteados, todos justificados: 15 fixtures de producción que el build experimental tiene que rechazar, 3 de estadísticas, 2 rung-exact y 2 que necesitan más de un shard. `cmake --install` instaló **0 archivos** | `v2-exp1-final.out`, `v2-install.log` |
| V3 | configure | `UTXOZ_SHARD_COUNT=8`, `EXPERIMENTAL=8`, `EXPERIMENTAL=3` y `#define UTXOZ_SHARD_COUNT` | los tres configure, exit 1; el define, `#error` | `v3-*.log` |
| V4 | `build11-asan`: ASan+UBSan, `-g`, estadísticas `lookup` | `[batch],[rotation],[capacity],[failpoint],[failpoints],[recovery],[compaction],[shard],[routing],[telemetry],[compat],[format],[transition],[unique]` y `utxoz_batch_alloc_tests` | **driver: exit real 4 (sólo salteos)**: tally `failed=2 = skipped_cases=2`, `catch2_failed=0`; alloc exit 0. **218 casos: 216 OK, 0 fallas**, 2 rung-exact, 0 reportes del sanitizer, alloc 3/3. Los 17 casos de telemetría que V1 saltea **corrieron y pasaron acá** (verificado por nombre) | `v4-asan.out`, `v4-asan-alloc.out` |
| V5 | `build11-tsan`: TSan | `[batch],[concurrency],[ownership],[executor]` y alloc | **exit 0** (driver y alloc). **39/39, 0 reportes**, alloc 3/3. Control positivo: la mutación G1 (`close()` sin esperar) da `heap-use-after-free`, exit 66; la fuente se restauró por hash | `v5-tsan.out`, `mutations/g1-tsan.out` |
| V6 | `conan create . --version=0.0.0-t11 -pr:h ci/conan-cpp23 -s build_type=Release -o with_tests=False -o with_large_benchmarks=False` | `test_package` | **exit 0** (visto en consola; el log no guarda el exit). exit 0. El consumidor imprime `full_db: ok (8 shards, geometry 4, insert_batch)` y `reference_db: ok`. Sólo en el caché local: no se subió | `v6-conan-create.log` |
| V7 | `utxoz_census` y `utxoz_verify_unique` sobre una copia del set de geometría 4 | — | **exit 0** en census y verify sobre geometría 4; sobre geometría 3, census exit 2 y verify exit 1, que son los rechazos esperados (vistos en consola). census: `geometry_id: 4`, `shards: 8` con detalle por shard. verify: `unique: true`, `placement_correct: true`. Sobre una copia de geometría 3: error 24 (`geometry_mismatch`) | `v7-*.out` |
| V8 | fixtures de geometría 4 | generados dos veces | los digests lógicos y por segmento coinciden; sólo cambian los bytes, por el `database_id` aleatorio. El set de geometría 3 quedó intacto (`git status` limpio) | `fixtures-final.*/` |
| V9 | diff | `git diff --check` y `git status` | limpio; sin builds, scratch ni evidencia versionados | — |
| CI (local) | el paso nuevo de `generated-headers` | la misma lógica en local | PASS | `ci-step-local.out` |

**Límites de esta validación:**
- Es en una sola máquina (Linux x86_64, GCC 16). macOS, Windows y la CI de GitHub
  **no corrieron**.
- No incluye los casos `[.slow]`.
- No hay benchmark ni IBD.
- `test_lookup_telemetry` se cubre en el build de ASan con estadísticas `lookup`, no en
  el de producción.

## 8. Pendientes exactos

### 7.3 Revisión independiente de Codex (en curso, sin aprobación todavía)

Lo que Codex informó hasta ahora, como evidencia propia y no de este documento:
- **58 pruebas independientes** de `[compat]`, `[format]`, `[generator]` y `[batch]`,
  todas en verde con exit 0, en `/tmp/codex-close11.rsPksR`.
- **Los negativos de CMake:** la opción vieja y `EXPERIMENTAL=8` se rechazan por la
  causa correcta, y `EXPERIMENTAL=1` instala 0 archivos. Está en
  `/tmp/codex-close11-config.PJmSjo`.
- `git diff --check` limpio.
- **Revisó** el cierre productivo `9bdeb73`, la CI, los fixtures, el consumidor y este
  roadmap.

Esto no es una aprobación: la aprobación la declara Codex al terminar.

### 8.1 Código: P1–P12, resueltos

| | Qué | Dónde |
|---|---|---|
| P1 | `test_shard_of` compila y pasa | `9bdeb73` |
| P2 | `UTXOZ_SKIP_UNLESS_SHIPPED_GEOMETRY` saltea sólo en builds experimentales | `9bdeb73` |
| P3 | la compatibilidad lee por slot, y se agregó el test de rechazo entre geometrías | `9bdeb73` |
| P4 | el set `geometry4-lp64` está promovido; Codex autorizó incorporar fixtures nuevos | `9bdeb73` |
| P5 | los tests del generador corren en producción (3/3) | `9bdeb73` |
| P6 | los rechazos de las geometrías 1, 2 y 3 corren en todo build | `9bdeb73` |
| P7 | las rutas de fixtures, con el set legacy aparte | `9bdeb73` |
| P8 | `static_assert` y comprobaciones de ejecución en `test_package` | `9bdeb73` |
| P9 | el comentario de `census.hpp` | `9bdeb73` |
| P10 | los tests rung-exact quedan como **salteo documentado** en producción: son de un perfil experimental | — |
| P11 | **CI:** el paso nuevo (la opción vieja rechazada, `EXPERIMENTAL=8` rechazado, un experimental que no instala nada) en `f8c1020`, y la sonda de Boost apuntada al set nuevo | `f8c1020`, `9bdeb73` |
| P12 | README | commit de docs |

**P11, lo que no se agregó, con la justificación:**
- **Un job de TSan:** no se agrega en este PR. La compuerta de TSan es la corrida local
  dirigida (V5).
- **Un job experimental que corra la suite:** no se agrega, por decisión de Codex en la
  revisión 1. El aislamiento lo verifica el paso nuevo de CI.

### 8.2 Pendiente

- **La revisión independiente de Codex** (máximo 3 rondas).
- **CI en GitHub** (macOS, Windows y la sonda de Boost): sólo puede correr cuando haya
  un PR o un push, y eso no está autorizado todavía.
- **Las decisiones de §9:** la versión, el tag y la forma del historial (Fernando), y el
  push y el PR (todavía sin autorizar).
- **Alcance del PR** (revisión 1 de Codex): conserva los scripts, harness, tests y docs
  de metodología reproducibles, repartidos en los cuatro PRs de §2. Los crudos, builds y logs masivos
  (`grid-06/`, `mut-07/`, `batch-*/`) quedan fuera del repositorio.

## 9. Release gate

1.0 se puede publicar sólo si se cumple todo esto:
1. P1–P12 resueltos o decididos explícitamente. **Hecho localmente (§8.1).**
2. V1–V9 aceptadas según el criterio de §7.0: exit 0, o exit 4 sólo con salteos
   puros, tally consistente y salteos justificados; cero fallas, timeouts y errores de
   infraestructura. **Cumplido localmente (§7.2), con los exits reales registrados por
   fila. Falta la CI de GitHub en Linux, macOS y Windows.**
3. Codex revisó y aprobó (máximo 3 rondas). **En curso (§7.3).** No incluye un job de
   TSan en CI: la compuerta de TSan es la corrida local dirigida (V5), declarada así.
4. Los cuatro PRs apilados de §2, cada uno con un solo commit, mergeados en orden. El
   PR 1 va sobre `origin/master`. Después de mergear cada uno, el siguiente se rebasa
   sobre el `master` resultante, con un respaldo previo. Antes de mergearlo se verifica
   que sigue siendo un solo commit y que su diff no cambió. El push de una rama
   rebasada se hace sólo coordinado.
5. La versión del tag la decide Fernando.
6. KTH se integra **después**, con su propia validación de punta a punta (sync real),
   que estos PRs no prometen.
