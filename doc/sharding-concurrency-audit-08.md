# Auditoría estática de concurrencia — qué permite hoy el sharding y qué falta

Sólo lectura de código. No se ejecutó nada, no se cambió código productivo y no se
corrió ninguna campaña. Las citas son a `d427c15`; la revisión 2 corrige la versión
anterior (`85b3ce1`), y la revisión 3 integra la evidencia de KTH.

**Requisitos vigentes que esta propuesta respeta:**
- el presupuesto del caché de archivos es **global**, para que un shard caliente
  pueda quedarse con él; Fernando rechazó partirlo por shard;
- `resolve_mutex_` se conserva.

**Estado de un plan previo, sujeto a decisión:** `doc/file-cache-pinning-design.md`
deja el *pinning* (`mapping_handle`) fuera de 1.0.0 como follow-up. Es lo que dice ese
documento, no una decisión de Fernando registrada aquí. Si 1.0.0 incluye o no lectura
histórica en paralelo depende de la decisión sobre el alcance de la concurrencia en
1.0.0 (pregunta de producto, abajo).

## El punto de partida

El objetivo que aclaró Fernando es que shards independientes eviten contención y
dejen usar varios threads. Las campañas de TAREA 06 y 07 fueron de un solo thread, así
que **no evaluaron ese beneficio**. Esta auditoría establece cuánto de él existe hoy.

Conviene separar dos cosas:
- **Independencia de almacenamiento.** Cada shard tiene su archivo, su mapa, su
  segmento, su catálogo y su historia. Esto **ya existe**.
- **Independencia de concurrencia.** Que dos threads puedan operar sobre shards
  distintos sin coordinarse. Esto **no existe** para las mutaciones: lo impiden el
  contrato y el estado global compartido (abajo).

## El contrato publicado hoy

`include/utxoz/database.hpp`, comentario `@par Threading` de `db_base` (líneas
~166–231):
- **Una sola mutación a la vez, sin ninguna otra operación en vuelo.** Mutaciones son
  `insert()`, `apply_deletes()`, `compact_all()` y `close()`. Serializarlas es
  responsabilidad del llamador.
- **`resolve()` puede llamarse concurrentemente**, pero la biblioteca lo **serializa**:
  `resolve_mutex_` (`src/detail/database_impl.hpp:522`) se toma durante toda la
  llamada (`full_resolve`, `src/database_impl.cpp:3777`). Es concurrencia de llamada,
  no paralelismo.
- **`find()` puede correr junto a `resolve()`.** Lo prueba ThreadSanitizer en
  `tests/test_lookup_ownership.cpp` (casos con `reader`/`resolver`, ~1076–1141), y vale
  sólo **sin ninguna mutación en vuelo**.
- Las estadísticas también son operaciones: `get_statistics()` no es const.

## Matriz

"Seguro" quiere decir que el contrato lo permite y el código lo respalda. Que algo
sea plausible sin contrato no autoriza a usarlo.

| par | contrato hoy | estado compartido y locks reales | 1/2/4/8 threads hoy |
|---|---|---|---|
| **find / find** | no está en la lista de `@par Threading`: el contrato no lo nombra | `find` (`database_impl.cpp:1245`) hace un `find` const sobre `containers_[slot]` y anota en `probe_stats_` (contadores atómicos repartidos por thread, `statistics.hpp:195–252`) y en `lookup_stats_[slot]` | **ejercitado por tests:** `tests/test_statistics.cpp:127` (8 threads × 5 000 `find` acertados, conteos exactos) y `:190` (8 threads de misses, cada uno con su lote). **Ningún job de CI corre TSan:** `ci.yml` sólo tiene `none` y `address,undefined` (`:104–134`), aunque `conanfile.py:37` admite `thread`. No hay registro en el repo de que estos dos tests se hayan corrido bajo TSan. Falta nombrarlo en el contrato y correrlo bajo TSan |
| **find / resolve** | permitido sin mutación en vuelo | disjuntos: activos contra caché de historia; probado con TSan | seguro para 1 find + 1 resolve; con más threads, cada `resolve` espera a los demás |
| **resolve / resolve** | permitido, serializado | `resolve_mutex_` durante toda la llamada. Hay un único `file_cache_` (`database_impl.hpp:503`), por defecto de un archivo (`file_cache.hpp:72`). `get_or_open_file` devuelve una **referencia cruda** (`:63`, `:76`), y el mutex protege además estado mutable del caché: `gets_`, `access_frequency_`, `last_used`, `access_count` y `hits_` (`:87–97`). `evict_lru` elige por LRU global, cruzando shards, y desmapea (`:288–298`); respeta `is_pinned`, pero nada lo escribe | correcto con N threads, pero **sin paralelismo**: corren de a uno |
| **insert / insert, shard distinto** | **prohibido** | cada slot tiene su mapa, segmento, catálogo, versión, `rehash_watch_` y `container_stats_[slot]`, pero `insert_in_index` también escribe estado **global**: `entries_count_` (`.hpp:486`), `height_range_stats_` (`.cpp:1104`, `.hpp:538`), `integrity_latched_` (`.hpp:302`) al fallar, y en la rotación `dirty_versions_` (`std::set`, `.hpp:330`, vía `note_dirty`, `.cpp:340`) | **no seguro**. Además, "shard distinto" no alcanza: las clases 1–4 son un único slot cada una, y un valor de otro tamaño cae en la misma clase sin importar el shard |
| **delete / insert** | prohibido | `apply_deletes` (`.cpp:1337`) borra en los activos, escribe a través de los mapeos del `file_cache_` compartido, descuenta `entries_count_` (`.cpp:1394`) y anota `dirty_versions_` | no seguro |
| **rotate / read** | prohibido: la rotación ocurre dentro de `insert` | `new_version` → `close_container` (`.cpp:329`, `:319`) desmapea el segmento activo y deja `containers_[slot]` nulo un instante; un `find` concurrente lee memoria desmapeada (lo dice el propio contrato) | no seguro |
| **compact / read** | prohibido | `compact_all` (`.cpp:2706`) vacía `file_cache_` (`:2711`, `:2732`), y cada `compact_container` (`:2428`) cierra el activo, reescribe la historia y lo reabre | no seguro |

## Estado global mutable que hoy impide escritores por shard

**Ya es por slot**, y sería del worker dueño del slot: `segments_`, `containers_`,
`current_versions_`, `catalogs_[slot]` (metadata de versiones, `.hpp:502`),
`rehash_watch_`, `rotation_causes_`, `container_stats_[slot]` y `lookup_stats_[slot]`.
La metadata en disco también es por slot (`meta_0s{k}_v*`).

**Es global**, y hay que repartirlo o volverlo atómico:

| estado | dónde | qué haría falta |
|---|---|---|
| `entries_count_` | `.hpp:486` | un contador por slot, sumado al leer |
| `height_range_stats_`, `lifetime_stats_`, `not_found_stats_`, `fragmentation_stats_`, `deferred_stats_` | `.hpp:538–544` | por slot y agregados, o repartidos como `probe_stats_` |
| `dirty_versions_` (obligación de durabilidad) | `.hpp:330` | un conjunto por slot; `sync()` los recorre todos |
| `integrity_latched_`, `cleanup_pending_` (latch de recuperación) | `.hpp:302`, `:282` | un latch atómico con el primer error (raro, no está en el camino caliente) |
| `file_cache_` y `resolve_mutex_` | `.hpp:503`, `:522` | **sigue siendo global** (requisito). Para que dos threads lean historia a la vez hace falta que retirar del caché y desmapear sean cosas separadas: un *pin* contado por entrada, con el desmapeo diferido al último pin (`mapping_handle`). Y además proteger la contabilidad del caché, que el pinning no cubre |
| callback de log | registro: `g_log_callback` (`src/log.cpp:43`), escrito por `set_log_callback`, que según `logging.hpp:96–98` no debe llamarse con operaciones en curso. Invocación: sobre el thread que llama a la biblioteca | hoy **no hay invocaciones concurrentes** en lo que el contrato permite: `find` no loguea, y `resolve` loguea pero está serializado. Con workers internos habría invocaciones desde varios threads. La biblioteca puede **conservar la garantía actual** (nunca dos invocaciones a la vez) serializando el despacho o juntando los mensajes de los workers en el thread del llamador. Es técnico; sólo pasa a ser de producto si se quisiera permitir callbacks concurrentes |
| `database_lock lock_` | `.hpp:310` | **no impide:** es el reclamo del directorio entre procesos o instancias |
| failpoints | `durability.hpp` | atómicos; sólo para tests |

Rendimiento, no corrección: los arrays por slot son contiguos (`std::array<…,
slot_count>`), y dos workers escribiendo slots vecinos compartirían líneas de caché
(*false sharing*). Habría que alinear el estado por slot.

## Costo estructural contra paralelismo

TAREA 06 contó, para un lote fijo de 4 000 claves, 58 archivos abiertos en N=1 y 464
en N=8, con las mismas búsquedas por clave. El enrutado de TAREA 05 ya filtra cada
clave a su shard. Lo que queda ×N es **estructural en un lote mixto**:
- cada shard tiene alguna clave del lote, así que hay que leer la historia de cada
  shard (~G versiones);
- eso da ~G·N archivos de 1/N del tamaño cada uno;
- con un caché de un archivo, cada uno se vuelve a mapear en cada lote.

**Repartir el lote entre threads no reduce la cantidad de aperturas**: sólo puede
solaparlas en el tiempo. Lo que reduce la cantidad es otra cosa:
1. **un presupuesto de caché global mayor**, que deje mapeados los archivos entre
   lotes y convierta aperturas en aciertos;
2. **lotes más grandes**, que amortizan cada apertura entre más claves.

Hay una observación técnica relacionada: el presupuesto de hoy se cuenta en
**archivos** (`max_size`), no en bytes. Con N=8 los archivos son 8 veces más chicos,
así que el mismo presupuesto en archivos cubre 1/8 de los datos. Medirlo en bytes
mantendría la cobertura entre N sin partir el presupuesto por shard.

## Diseño mínimo para concurrencia útil (propuesta, sin implementar)

Ninguna variante decide de paso un mutex global ni uno por shard, ninguna saca locks
existentes y ninguna promete lock-free.

### Escrituras: propiedad por slot, dentro de lotes

- Una llamada de lote (por ejemplo `apply_block`, o lotes de `insert` / `apply_deletes`)
  reparte el trabajo por **slot** (clase y shard). Cada slot lo procesa un único
  ejecutor, que es dueño exclusivo de su mapa activo, su segmento, su rotación, su
  catálogo, su metadata, su obligación de durabilidad y sus contadores.
- Mientras dura una llamada, ningún slot tiene dos escritores. Así, la parte activa no
  necesita locks en el camino caliente.
- **Quién ejecuta** es una decisión aparte:
  - el llamador pasa su executor (o la biblioteca expone el trabajo por slot para que
    el llamador lo despache);
  - o la biblioteca usa threads propios.

  Un pool propio **no es necesario** para el diseño.
- **Lo que sigue siendo compartido** se resuelve sin monopolizar un lock global:
  - contadores repartidos por slot y sumados al leer;
  - el latch de integridad, atómico;
  - `dirty_versions_`, por slot;
  - el log, con el despacho serializado o en el thread del llamador.
- **La historia en las escrituras:** `apply_deletes` escribe a través de mapeos del
  caché global. Hasta que exista el *pinning*, esa parte tiene que tomar la misma
  exclusión que hoy tiene `resolve` sobre el caché, y no puede correr en paralelo con
  otra que use el caché. La parte activa (fase 0) sí puede ir en paralelo por slot.

### Lecturas históricas: caché global con *pinning*, sin sacar `resolve_mutex_` hasta probarlo

- Para que `resolve` reparta su lote entre threads hace falta lo que ya está diseñado
  como follow-up: `mapping_handle` con contador de pins, desmapeo diferido al último
  pin, y retirar del LRU separado de desmapear.
- Aun así, `resolve_mutex_` sigue protegiendo la contabilidad del caché: el mapa
  `cache_`, `access_frequency_`, `gets_`, `hits_`, `evictions_`, `last_used` y
  `access_count`.
- El paso intermedio seguro es **reducir lo que cubre el mutex** a esa contabilidad (los
  lookups y la admisión al caché) y leer los mapas fuera del lock con el pin tomado.
  Sólo se puede quitar del todo cuando haya pruebas, con TSan, de que lo que queda está
  protegido de otra forma.
- **El presupuesto y la admisión siguen siendo globales:** un shard caliente puede
  ocupar todo el caché.

### Lectura mientras se escribe

- Hoy está prohibido, porque la rotación y la compactación desmapean el activo. Si se
  quisiera, haría falta un gate por slot (lectores contados o épocas) y diferir el
  desmapeo del activo.
- Para un pipeline por fases (primero lecturas, después escrituras) **no hace falta**.

### Compactación

Hoy es serial **a propósito**: "two at once would give back the very footprint
sharding is meant to reduce". Paralelizarla multiplica el pico de espacio por la
cantidad de compactaciones simultáneas. Compactar un shard por llamada, o por
presupuesto, acota la pausa visible sin subir el pico; TAREA 08 mediría esa unidad.

## Cómo usa KTH hoy la biblioteca (evidencia del coordinador de KTH)

Informado por el coordinador de KTH sobre `kth-710` en modo sólo lectura, base
`875fe334` (#710 toca sólo headers). **No se auditó KTH desde esta sesión**, no se
miró mempool ni minería, y no se afirma que este sea el patrón de todos los usos.

- **Construcción de bloques:** `utxo_build_task` en `block_tasks.cpp` procesa los
  lotes **en secuencia**.
- **Validación:** el parsing y la validación usan `validation_pool(1)`; los scripts se
  paralelizan internamente. **No hay un pool para shards ni para escritura.**
- **El undo se captura antes de mutar,** por bloque: delta acumulado, `find_utxo_raw`,
  `utxo_resolve_raw` sobre los `not_resolved` (`utxo_builder.hpp:388–465`).
- **La aplicación es síncrona, dentro de una única ventana de escritura exclusiva y
  sin `co_await`.** El orden es:
  1. registro de transición durable / `env_sync`;
  2. `apply_utxo_delta`: retira los reemplazos BIP30 y después inserta;
  3. el undo;
  4. `apply_deletes`, un barrido acotado a 3;
  5. flush / sync y completar.
- **La ventana de escritura excluye a los lectores externos.** El IBD actual **no**
  requiere lectura y escritura externas concurrentes.
- **Compactación:** `compact_utxo` al final de la sincronización
  (`block_tasks.cpp:2684`).
- **Reorg:** se pausa entre lotes y abre su propia ventana de escritura.

**Qué se sigue de eso:**
- **Los lotes internos por fase son compatibles en lo conceptual** con este uso: KTH
  ya separa capturar (lecturas) de aplicar (una ventana de escritura exclusiva), y
  dentro de esa ventana una llamada de lote podría repartir el trabajo por slot.
- **El orden de la ventana no se toca:** transición durable, BIP30, inserts, undo,
  deletes, sync y la pausa del reorg. Un lote interno tiene que preservar
  exactamente ese orden y esa semántica.
- **No se supone que el executor de validación sea reutilizable** para escrituras.
  `validation_pool(1)` es de otra cosa.
- **Leer mientras se escribe no es un requisito** para el IBD actual.

## Recomendación de unidad inicial

- **Inserción por lote con W configurable** dentro de una sola llamada, con el
  ejecutor adaptable: el del llamador o uno propio. La forma final de la API no se
  inventa acá; se define al implementar.
- La parte activa se reparte por slot con un único ejecutor por slot.
- **La historia sigue serializada con el caché global** hasta que haya pruebas de
  vida de mapeos.
- **La compactación sigue serial,** por presupuesto o por partes, mientras no se
  apruebe otra cosa.
- Se valida contra la aplicación secuencial con TSan y se mide con W = 1, 2, 4 y 8.
  Nada de eso se ejecuta hasta que se apruebe.

## Pregunta de producto

Una sola:

**¿Se acepta como contrato objetivo de 1.0.0 "fases con paralelismo interno" (lotes
que la biblioteca reparte por shard dentro de una llamada, con una mutación a la vez
hacia afuera), en lugar de exigir una API externa concurrente?**

Recomendación: **sí**. Es compatible con cómo KTH aplica hoy un bloque, en una sola
ventana de escritura exclusiva y sin lectores concurrentes. Da paralelismo por shard
sin locks en el camino caliente, y respeta el caché global. La API externa concurrente
exigiría sincronización por slot y un gate de mapeos que el IBD actual no necesita.

Lo demás es técnico y se propone sin cargar a producto:
- estado por slot;
- contadores, `dirty_versions_` y latch;
- despacho del log;
- presupuesto en bytes;
- CI con TSan;
- cómo se pasa el ejecutor.

<details><summary>Preguntas de la revisión 2, ahora resueltas o técnicas</summary>

- Leer mientras se escribe: no lo requiere el IBD actual (evidencia de KTH).
- Quién pone los threads: no hay un pool reutilizable en KTH para esto. El ejecutor
  se deja adaptable y es técnico.
- Pico por compactar en paralelo: no se propone; la compactación sigue serial.

</details>

## Plan de pruebas y benchmark (no ejecutar todavía)

**Correctness:**
1. **Oráculo:** el mismo lote aplicado secuencialmente y con W = 1, 2, 4, 8 workers
   tiene que dar el mismo contenido final. Hay un digest por la regla ya hecho en
   `calib_mutation`.
2. **Lotes que cruzan shards y clases** en la misma llamada.
3. **Rotación simultánea** en varios shards.
4. **Failpoint en un worker a mitad de lote:** semántica de progreso parcial por clave,
   y que nunca se llame ausente a algo que no se probó.
5. **Recuperación** después de un corte en un worker.
6. **`resolve` repartido** igual a `resolve` serial.
7. **ThreadSanitizer** sobre todo lo anterior, sumando los tests concurrentes que ya
   existen: `test_statistics.cpp:127` y `:190`, y los de `test_lookup_ownership.cpp`.
   Hace falta un job de CI con `sanitizer=thread`, que el `conanfile.py` ya admite y
   `ci.yml` hoy no corre.
8. **Contabilidad del caché bajo concurrencia** cuando exista el *pinning*: desalojo de
   un archivo con pin, el mismo archivo pedido desde dos threads, y desalojo que cruza
   shards.

**Benchmark futuro**, con la autorización y la ventana de siempre y restauración por
copia:
- throughput de inserción por lote con W = 1, 2, 4, 8, en N = 8, contra N = 1 de un
  thread;
- `resolve` repartido, sólo después del *pinning*, informando **aperturas** y
  **tiempo** por separado: el paralelismo puede bajar el tiempo sin bajar las
  aperturas;
- pausa por shard en compactación (TAREA 08);
- CPUs declaradas: en este host, el CCD 0-7 y 16-23 comparte el L3 V-Cache.

**No se toca KTH**, ni sus locks, caché o mutex, hasta decidir A o B.

## Conclusión

El sharding de hoy da **independencia de almacenamiento**, no **independencia de
concurrencia**. Con 8 shards, 8 threads no pueden mutar en paralelo sin violar el
contrato y sin carreras en el estado global.

El camino que propongo es mutar por slot dentro de lotes, con el executor del
llamador, y leer en fases separadas. Respeta el caché global y no saca locks
existentes. El *pinning* de mapeos es lo que habilitaría después leer la historia en
paralelo. Repartir el trabajo puede solapar el costo de las ×N aperturas de un lote
mixto, pero no lo reduce: eso depende del presupuesto de caché y del tamaño de lote.
