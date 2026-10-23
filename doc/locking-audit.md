# Auditoría de sincronización — `utxo_gate_` de KTH y `resolve_mutex_` de UTXO-Z

Sobre el worktree real de R2: `/home/fernando/dev/kth/wt-r2`, KTH `f143b7d8`
(#703) más los dos commits locales, y `/home/fernando/dev/utxo-z/wt-d0`,
UTXO-Z `b1449038`.

**Ningún código modificado.**

---

## 1. `utxo_gate_`: declaración y capacidades

    src/blockchain/include/kth/blockchain/interface/block_chain.hpp:1202
        mutable utxo_gate utxo_gate_;

    src/blockchain/include/kth/blockchain/utxo_gate.hpp

**Corrijo lo que dije antes.** Afirmé que KTH usa «una ventana exclusiva global».
Es incompleto: el gate expone **tres** capacidades, no una.

| capacidad | línea | semántica |
|---|---|---|
| `read()` → `utxo_read_lease` | 542 | **compartida**. Varios lectores a la vez; espera si hay escritor activo o esperando |
| `write()` → `utxo_write_window` | 572 | **exclusiva**. Espera a que salgan todos los lectores |
| `try_write_for(budget)` | 652 | exclusiva con plazo |
| `authorise_close()` → `utxo_close_authority` | 604 | exclusiva, **salta el chequeo de veneno**; permiso para cerrar, no para seguir usando |

Preferencia de escritor: `read()` espera mientras `writers_waiting_ != 0`, de modo
que un flujo de lectores no puede matar de hambre a una escritura.

## 2. Todos los call sites de producción

| línea | operación | capacidad | función |
|---|---|---|---|
| `block_chain.hpp:219` | — | `write()` | `begin_utxo_write()`, el punto de entrada público |
| `block_chain.hpp:261` | insert raw | `latch_observed()` | `apply_utxo_inserts_raw` al ver `needs_recovery` |
| `block_chain.cpp:215` | **open** | `write()` | `start()` — «lifecycle» |
| `block_chain.cpp:532` | **open** | `write()` | `start()` — «configure» |
| `block_chain.cpp:584` | **close** | `authorise_close()` | `close()` |
| `block_chain.cpp:1145,1160,1171` | **delete** | **`read()`** ×3 | `utxo_apply_deletes` |
| `block_chain.cpp:1505` | reorg | `write()` | `switch_to_branch` |
| `block_chain.cpp:1866` | **compact** | `write()` | `compact_utxo()` |
| `block_chain.cpp:1885` | compact | `latch_observed()` | `compact_utxo()` |
| `block_chain.cpp:1913,1930` | statistics | `write()` ×2 | `print_utxo_statistics()` |
| `block_chain.cpp:2009` | sizing | `read()` | `print_utxo_sizing_report()` |
| `block_chain.cpp:2018` | height stats | `read()` | `print_utxo_height_range_stats()` |
| `block_chain.cpp:2027` | count | `read()` | `utxo_count()` |
| `block_chain.cpp:2042,2050` | bloom | `write()` ×2 | `set_utxo_bloom` / `clear_utxo_bloom` |
| `block_chain.cpp:2234,2272` | mempool | `held_by_current_thread()` | detección de reentrada, no adquiere |
| **`block_chain.cpp:2545`** | **find** | **`read()`** | **`get_utxo()`** |
| `utxo_builder.cpp:597` | bloom | `write()` | `save_utxo_bloom` |
| `block_tasks.cpp:2338` | **batch** | `write()` | la ventana del lote de UTXO build |
| `interface/block_chain.cpp` (telemetría) | telemetry | `write()` | `utxo_lookup_telemetry()` |

## 3. El hot path: sí, el gate está adentro

`batch_validate.cpp:222` llama **`chain.get_utxo(op, height)` dentro del bucle por
input**, y `get_utxo` toma un `read()` lease por cada lookup individual.

Costo por lookup:

    read():      lock(mutex) · check poison · check reentrada (thread id)
                 · cv_.wait con predicado (no espera si no hay escritor)
                 · ++readers_ · unlock
    end_read():  lock(mutex) · --readers_ · unlock · cv_.notify_all()

**Dos adquisiciones de mutex y un `notify_all` por lookup.** En R2-D0 fueron
**4.274.836 lookups**, es decir ~8,5 M adquisiciones de mutex y 4,3 M notificaciones.

### Pero no es el cuello de botella medido

Un par lock/unlock sin contención cuesta del orden de 20 ns, y `notify_all` sin
esperadores otros 20. Unos 60 ns por lookup:

    4.274.836 x 60 ns ~ 0,26 s

sobre una corrida de **3.900 s**. **Un 0,007 %.** El gate está en el hot path y su
costo sin contención es despreciable frente al trabajo real.

**Lo que no está medido es su costo CON contención.** Con varios hilos de
validación golpeando el mismo `mutex_` y la misma línea de caché de `readers_`,
el ping-pong entre núcleos puede pesar mucho más que el costo sin contención. Eso
requiere un banco, y no lo tengo.

## 4. Qué invariante protege realmente

**No protege datos.** UTXO-Z documenta que `find()` puede correr junto a
`resolve()` sin lock del llamador. El lease no serializa lecturas entre sí.

**Protege el ciclo de vida de los mapeos.** El `file_cache` de UTXO-Z posee los
`managed_mapped_file` y los destruye al desalojar; una rotación dentro de
`insert()` desmapea el segmento activo entero; `close()` desmapea todo. Un lector
sosteniendo una referencia a memoria mapeada cuando eso ocurre es un **SIGSEGV**,
no una lectura rota.

Entonces:

| invariante | ¿la da el gate? |
|---|---|
| lifetime de los mapeos frente a close/rotación/desalojo | **sí, y es su razón de ser** |
| exclusión de mutaciones contra lecturas | **sí** |
| consistencia de estadísticas | parcialmente: `print_utxo_statistics` toma `write()` |
| consistencia de datos entre lecturas | **no**, y no hace falta |
| atomicidad de un lote | **no** — el lote es un bucle por entrada |
| publicación | **no** |

## 5. ¿Es redundante para mutaciones?

**No con la evidencia disponible.** El build de UTXO es una sola tarea, sí, pero:

- `utxo_apply_deletes` toma **`read()`**, no `write()` — la exclusión de esa
  mutación la aporta la ventana que el llamador ya sostiene, y el lease sólo
  suma la barrera de lifetime;
- `switch_to_branch` (reorg), `compact_utxo`, `set_utxo_bloom` y `close()` pueden
  ocurrir desde hilos distintos del build;
- la validación llama `get_utxo` desde los hilos de validación, que **no** son el
  hilo del build.

Un solo escritor por executor no está demostrado en el código; lo que hay es una
disciplina de ventanas. Quitar el gate sin demostrar el single-writer sería
cambiar una garantía verificada por una suposición.

## 6. Alternativas, evaluadas

| alternativa | qué pasaría |
|---|---|
| **sin exclusión en el hot path** | SIGSEGV cuando una rotación o un `close()` desmapee bajo un lector. **Inaceptable** |
| **barrera sólo de lifecycle** | es exactamente lo que el gate ya hace; el margen está en *cómo*, no en *si* |
| **epoch / reader-counter atómico** | **la dirección correcta**: el lector incrementa un atómico y verifica una bandera; sin mutex ni notify salvo que haya un escritor esperando. Preserva la invariante y saca el mutex del hot path |
| **shared ownership del store** (`shared_ptr` al segmento) | mueve el problema al conteo de referencias, que también es atómico; funciona pero cambia la propiedad del `file_cache` |
| **locks internos por shard en UTXO-Z** | **no resuelve esto**: la invariante es de lifetime de mapeos, no de contención entre shards. Añadiría un mutex global nuevo por operación, que es justo lo que hay que evitar |

**Preferencia, alineada con el objetivo de lock-less:**

1. KTH mantiene ownership y scheduling sin locks nuevos;
2. UTXO-Z sincroniza sólo lo indispensable;
3. la barrera global queda para open, close, snapshot y reconfiguración;
4. **ningún mutex global nuevo en find/insert/delete**;
5. si el gate resulta un cuello de botella medido, se reemplaza por un contador
   de lectores atómico — **y esa es una decisión aparte del sharding**.

---

## 7. `resolve_mutex_` de UTXO-Z

    src/detail/database_impl.hpp:519    mutable std::mutex resolve_mutex_;
    src/database_impl.cpp:3577          resolve()
    src/database_impl.cpp:4075          barrido de eliminaciones diferidas

| pregunta | respuesta |
|---|---|
| **por qué existe** | el `file_cache` no tiene sincronización propia y **posee los mapeos**. Dos resoluciones simultáneas: una desaloja un segmento que la otra está leyendo → SIGSEGV. Medido: **90 carreras de ThreadSanitizer y exit 139** antes de que existiera (#120) |
| **qué estado protege** | el `file_cache` y las referencias que entrega, no los datos |
| **¿puede eliminarse?** | **no**, salvo que el `file_cache` se rehaga con propiedad contada o el desalojo se difiera |
| **¿estado local por operación?** | sí en principio: un caché por resolución elimina el compartido, a costa de perder la reutilización entre resoluciones — que es justamente lo que da los 3.499 cache hits medidos |
| **¿uno por shard?** | **sí, y es una mejora real**: el caché se indexa por `(clase, versión)` y con sharding sería `(clase, shard, versión)`. Dos resoluciones en shards distintos no comparten entradas, así que un mutex por shard las deja correr en paralelo sin cambiar la invariante |
| **costo en el hot path** | **cero**: `find()` no lo toca. Sólo `resolve()` y el barrido de borrados |
| **efecto sobre semántica** | ninguno: es exclusión, no orden |

---

## 8. Matriz operación → ownership → sincronización

| operación | ownership hoy | sincronización actual | propuesta |
|---|---|---|---|
| `find` / `get_utxo` | hilos de validación | **`read()` lease por lookup**: 2 mutex + notify | **sin cambio en 1.0.0.** Candidato a contador atómico si un banco lo justifica |
| `resolve` / lote diferido | hilo del build | `read()` del llamador + **`resolve_mutex_` global** | **un `resolve_mutex_` por shard** — habilitado por el sharding, sin mutex nuevo |
| `insert` | hilo del build | ventana `write()` del llamador, una por lote | sin cambio |
| `delete` / `apply_deletes` | hilo del build | ventana del llamador + `read()` lease | sin cambio |
| lote (batch) | tarea `utxo_build` | una ventana `write()` por lote | sin cambio |
| telemetría | cierre y fin de build | `write()` sólo para la copia | sin cambio |
| census / verify_unique | herramientas offline | claim de directorio exclusivo | + comprobación de shard |
| compact | build o mantenimiento | `write()` global | **`write()` global, un shard por vez** |
| sync | dentro de otras operaciones | la de la operación | sin cambio |
| open | `start()` | `write()` ×2 | sin cambio |
| close | `close()` | `authorise_close()` | sin cambio |

**Ningún mutex global nuevo. El único cambio de sincronización que el sharding
propone es partir uno que ya existe.**

---

## 9. Veredicto sobre PRs de KTH

**Caso A: cero PRs de KTH por locking.**

El sharding **no cambia la API pública de UTXO-Z**: `find`, `resolve`, `insert`,
`erase`, `census` y `verify_unique` conservan su firma y su contrato. KTH no
necesita saber que la clase 48 está partida.

Lo único que KTH necesita, y cuando corresponda al release, es **la referencia de
la dependencia** — y eso no es un PR de locking sino el bump normal de versión.

**El gate no se toca en este trabajo.** Está en el hot path, su costo sin
contención es el 0,007 % de la corrida, y su costo con contención no está medido.
Cambiarlo es una decisión propia, con su propio banco y su propio riesgo, y
mezclarla con el sharding haría imposible atribuir cualquier resultado.

Si un banco futuro demuestra que el gate serializa, ese sería el **caso B**: un
único PR de KTH, un commit, alcance exclusivo de ownership y lifecycle. **No
antes de demostrarlo.**
