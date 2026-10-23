# TAREA 09 — inserción paralela por lote, un dueño por slot

**Estado: implementada y probada funcionalmente, con las correcciones de la revisión 1.
El rendimiento se midió después, en TAREA 10** (`doc/batch-contrast-10.md`). Lo que sigue es el estado de esta unidad: no se corrió ninguna campaña cronometrada. Para medir hacían
falta la revisión de Codex y una ventana coordinada con KTH.

## API

```cpp
insert_batch_result full_db::insert_batch(std::span<insert_request const> requests,
                                          insert_batch_options const& options = {});
```

- `insert_request { key, value, height }`: cada entrada lleva su propia altura.
- `insert_batch_options { workers = 1, run_tasks }`: `workers` es W. W1 es la línea base
  secuencial: 0 y 1 corren en el hilo que llama, un slot tras otro. `run_tasks` es
  opcional: un ejecutor del llamador. Si no se da y `workers > 1`, el lote usa hilos
  propios.
- `insert_batch_result`:
  - `outcomes[i]` por entrada, en el orden de la entrada: `not_applied`, `inserted`,
    `duplicate` o `failed`, con su `error`;
  - los totales de cada estado;
  - `error`: el latch, si quedó puesto, y si no la primera falla;
  - `workers_used` y `tasks_run_inline`.

Sólo existe en modo full. Un store cerrado, inutilizable o de sólo inspección rechaza
el lote: todas las entradas quedan `not_applied` y `error` dice por qué.

## Contrato

**Para el llamador es una sola mutación.** Rige el contrato de threading de
`database.hpp`: no puede correr junto a otra mutación. La llamada es síncrona y vuelve
cuando ninguna tarea está corriendo. Los hilos propios del lote se unen (`join`) antes de
volver. Los hilos de un ejecutor son del ejecutor; ver **Ejecutor**.

**Un solo escritor por slot.** El slot es la clase más el shard; las clases que no
son 48 tienen un único slot.
- Cada entrada va al slot que le corresponde por `get_index_from_size` y
  `slot_for_key`, igual que en `insert()`. El slot lo deciden la clase (por el tamaño
  del valor) y, en la clase con shards, la clave: el mismo par (clave, clase) cae
  siempre en el mismo slot. Una clave mandada dos veces con valores de clases distintas
  cae en dos slots y queda guardada dos veces, como con dos `insert()`. El lote no
  verifica unicidad entre clases ni nada de BIP30.
- Cada slot pertenece a una sola tarea. La tarea recorre sus entradas en el orden
  original, con duplicados incluidos.
- Los slots se reparten de mayor a menor carga, siempre a la tarea menos cargada, entre
  `min(max(1, W), slots usados)` tareas.

**Equivalente al secuencial, si el lote no pone el latch.** El contenido, el tamaño,
las estadísticas por clase, el censo por shard y las estadísticas por altura salen
iguales a los de insertar las mismas entradas una por una con `insert()`, con fallas
incluidas, siempre que ninguna ponga el latch. Una falla sin latch queda en su entrada,
como en un lazo de `insert()` que sigue de largo. Los tests lo comparan con W = 1, 2, 4
y 8, en N1 y en N8, también después de reabrir.

Cuando el latch detiene el lote, qué entradas de *otros* slots llegaron a intentarse
depende del timing. El estado guardado es el que dicen los resultados por entrada, no el
de un lazo secuencial cortado en la misma entrada.

**Una fase, no un bloque.** El lote contiene sólo inserciones de una fase ya
preparada. Se le pueden pasar varios bloques juntos, porque cada entrada conserva su
altura. Pero el lote no reordena nada entre slots ni conoce dependencias. Quedan a cargo
del llamador:
- BIP30;
- los gastos y los borrados;
- el undo;
- la sincronización y la durabilidad.

Si una entrada depende de una operación que no es inserción (un gasto, un borrado, un
chequeo), el llamador corta el lote en esa barrera.

**No es atómico.** Puede aplicarse una parte. Cada entrada informa lo que pasó con ella.
- Si hubo alguna falla, el resultado no da éxito global: `error` queda puesto.
- Nunca se informa `not_applied` para una entrada que se aplicó.
- `not_applied` significa que la entrada no se intentó: o el lote se rechazó antes de
  empezar (store cerrado, inutilizable o de sólo inspección, arriba), o se detuvo por
  el latch antes de llegar a ella.

**Fallas.**
- **Valor demasiado grande:** la entrada queda `failed` (`value_too_large`) antes de
  repartir y no se toca nada. Las demás siguen.
- **Falla que no pone el latch:** queda `failed` sólo esa entrada, y su tarea sigue con
  la siguiente, como haría un llamador que insertara una por una e ignorara el error.
- **Falla que pone el latch de integridad:** es toda falla que puede dejar el slot
  inutilizable. Entran acá:
  - la rotación que no puede crear el segmento nuevo (`file_open_failed`), que deja el
    contenedor cerrado;
  - el mapa mutado antes de una excepción (`entry_corrupt`);
  - cualquier excepción no clasificada.

  Con cualquiera de ellas:
  - El latch se escribe una sola vez, bajo `latch_mutex_`.
  - La tarea que falló lo lee (también bajo el mutex) y levanta un `atomic<bool> stop`.
    Cada tarea consulta `stop` antes de cada entrada, así que todas paran antes de la
    siguiente. Lo que queda sin intentar en cualquier slot queda `not_applied`.
  - El store queda inutilizable, como con `insert()`.
  - Una entrada `failed` con `entry_corrupt` tiene estado desconocido. El test lo
    verifica contra el store reabierto.
- **La cancelación no es instantánea:** las entradas que otras tareas ya están
  aplicando cuando se pone el latch (en vuelo) terminan. Cada una se informa tal como
  quedó (`inserted`, `duplicate` o `failed`), nunca como `not_applied`.

**Memoria** (revisión 1). Todo lo que el lote necesita para su propia contabilidad se
asigna en la preparación, antes de intentar la primera entrada:
- los resultados por entrada y el ruteo;
- los acumuladores por tarea: una fila por rango de altura que el lote puede tocar;
- el gate del ejecutor y la tarea;
- lugar (`make_room`) en cada mapa compartido donde la contabilidad de una entrada
  guardada inserta una clave nueva: los rangos de `height_range_stats_`, la
  `value_size_distribution` de cada slot y el registro de metadata de la generación
  vigente de cada slot.

Si algo de eso falla, `insert_batch` tira `std::bad_alloc` sin haber cambiado nada, y el
store no queda con latch. Desde la primera entrada no tira. Contar una entrada guardada
(en el worker) y sumar los acumuladores (en el hilo llamador) no asignan memoria, así
que una entrada guardada no puede perder su cuenta.

`make_room` sólo agranda el mapa. El `reserve` de Boost fija la capacidad pedida y
achica el mapa si la cifra es menor, así que no se usa a secas. `make_room` verifica
`max_load() - size()` en vez de suponerlo.

Lo que un worker todavía puede asignar es lo de una rotación. Una rotación que falla
pone el latch (`file_open_failed`). `note_dirty` asigna un nodo del `std::set`, y se
llama *antes* de cerrar la generación que se retira. Si falla, la generación sigue
abierta, la rotación se rechaza y `rotate_for` pone el latch. Ninguna generación se
retira sin su obligación de sync anotada.

**Índices.** El ruteo guarda el índice de cada entrada como `size_t`, sin estrecharlo.
Antes era `uint32_t` y truncaba lotes de más de 2³² entradas.

**Sync.** Los slots que rotaron dentro del lote quedan en `dirty_versions_`, y el
`sync()` siguiente los cubre. El test lo comprueba con un lote que rota, `sync` y reapertura.

**Ejecutor** (revisión 1). **El contrato es síncrono.** `run_tasks` no puede volver, ni
normalmente ni tirando, hasta que hayan vuelto todas las llamadas que hizo a `task`.
`task` vale hasta que `run_tasks` vuelve; un ejecutor que lo pasa a hilos tiene que
unirlos (`join`), también en el camino de la excepción. Esto reemplaza lo que decía la
entrega anterior ("puede volver apenas los arrancó"). Eso era inseguro: el ejecutor
recibía una referencia a un `std::function` temporal, y lo que compartían las tareas
vivía en el stack.

Lo que hace el lote de su lado:
- **Las tareas comparten un gate por propiedad.** Cada copia de `task` tiene un
  `shared_ptr` al gate, no una referencia al frame del lote. El gate guarda bajo un mutex
  qué índices se reclamaron, cuántas tareas están corriendo y si está cerrado. Cada
  índice corre a lo sumo una vez, la llame quien la llame.
- Después de que `run_tasks` vuelve o tira, el llamador corre las tareas que nadie
  reclamó.
- Después **espera a que ninguna tarea reclamada esté corriendo** (`condition_variable`)
  y **cierra el gate**. Recién entonces suma y arma el resultado. Con eso, una llamada
  perdedora o tardía nunca se cruza con la suma.
- Una llamada que llega después, por una *copia* de `task`, encuentra el gate cerrado y
  no toca nada del lote. Es una red de seguridad, no soporte para ejecutores
  asincrónicos: la tarea que esa llamada quería correr ya la corrió el llamador.
- Usar la referencia `task` (no una copia) después de que `run_tasks` volvió es
  comportamiento indefinido. El lote no lo puede detectar, y el contrato lo prohíbe.
- Un ejecutor que no arranca nada, que tira o que no puede crear hilos degrada al
  secuencial.
- `tasks_run_inline` cuenta las tareas que corrió el hilo llamador.

El mutex del gate se toma dos veces por tarea, no por entrada.

## Estado compartido: qué se tocó y por qué

| Estado | Antes | Ahora | Camino caliente |
|---|---|---|---|
| `entries_count_` y `height_range_stats_` | se escribían en cada inserción | un acumulador por tarea, con filas preasignadas, que se suma al final sin asignar | sin locks |
| `value_size_distribution` y metadata por slot | ya eran por slot | se les hace lugar antes de escribir | sin locks |
| `integrity_latched_` | `optional` sin sincronizar | `latch_mutex_` para escribirlo y para leerlo en el lote | sólo al fallar; el chequeo por entrada es un `atomic<bool>` |
| `dirty_versions_` | `std::set` sin sincronizar | `dirty_mutex_` en `note_dirty` | sólo cuando hay rotación |
| despacho del callback de log | sin sincronizar | `recursive_mutex` en `dispatch_log_message` (rama custom) | sólo al loguear |
| `consume_forced_rotation` (failpoint) | lectura y escritura no atómicas | lazo CAS | sólo en tests |

`insert()`, `find()` y `resolve` no cambian de comportamiento.
- `insert()` sigue escribiendo los contadores directamente (`acc == nullptr`). Tiene
  el mismo tipo de asignación después de guardar que la revisión 1 le quitó al lote:
  un rango de altura nuevo, un tamaño de valor nuevo, y el primer registro de metadata
  de una generación nueva al abrir. Es previo a esta unidad y no se tocó, porque
  agregaría trabajo a cada inserción del camino secuencial. Queda pendiente, como
  decisión.
- `resolve_mutex_` se conserva.
- El caché global de archivos se conserva y el lote no lo toca, porque la inserción
  sólo escribe la versión vigente.
- Los borrados históricos siguen en serie.
- No hay API de borrados en paralelo.

## Pruebas

- `tests/test_insert_batch.cpp`, `[batch]`: 14 casos (9 de la entrega y 5 de la
  revisión 1).
- `tests/test_batch_allocation.cpp`: un binario aparte, `utxoz_batch_alloc_tests`, con 3
  casos. Está aparte porque reemplaza el `operator new` global.
- `tests/support/batch_workload.hpp`: la carga y las comparaciones que los dos
  comparten.

**La carga `mixed()`:**
- 20 000 entradas de clase 0 repartidas en los shards;
- 3 000 de clase 1 y 3 000 de clase 2;
- 26 000 de clase 3, que rota;
- 2 500 de clase 4, que rota al menos 3 veces;
- 500 claves con 50 duplicados en orden;
- una misma clave en dos clases;
- alturas variadas, intercaladas de forma que se conserva el orden de cada clave.

**Casos:**
- equivalencia con el secuencial para W = 1, 2, 4 y 8, con reapertura;
- duplicados y la misma clave en dos clases;
- `value_too_large`;
- lote vacío;
- ejecutor propio, ejecutor que no arranca nada, ejecutor que tira después de que una
  tarea terminó en su hilo, y ejecutor que tira mientras una tarea que arrancó puede
  seguir corriendo;
- latch de integridad (`fail_insert_after_mutating`), comparando lo informado con lo
  guardado después de reabrir;
- falla de rotación (`fail_after_segment_create`): en clase 4 se insertan las primeras
  797 y la 798.ª da `failed` (`file_open_failed`), que pone el latch. El resto de
  clase 4 queda `not_applied`;
- store cerrado;
- `sync` después de un lote que rota, con reapertura.

**Revisión 1, casos nuevos:**
- *Copia llamada después de que el lote volvió:* el ejecutor guarda una copia y no corre
  nada. El lote corre todo en el llamador. La copia se llama después desde otro hilo,
  con todos los índices y uno de más, y otra vez con el store cerrado. No cambia nada.
- *Tarea en vuelo cuando el ejecutor vuelve:* un hilo con una copia queda bloqueado
  dentro de la primera línea de log de la tarea 0. El ejecutor vuelve, violando el
  contrato, con la tarea comprobadamente en vuelo, y un tercer hilo la libera a los
  200 ms. El lote tiene que haber esperado: `released` es verdadero cuando vuelve, y el
  resultado es completo e igual al secuencial. Sólo la tarea 0 loguea en esta carga;
  si no, el log serializado haría esperar al llamador por otro camino (ver G1 abajo).
- *Dos hilos que llaman todas las tareas (W = 2 y 8):* cada tarea corre una vez y
  `tasks_run_inline` es 0.
- *Ejecutor que tira sin unir un hilo que tiene una copia:* el lote sale completo.
- *Falla de memoria en la preparación* (`fail_batch_prepare`): `std::bad_alloc`, las
  cifras quedan iguales a las del store vacío, y no hay latch. El mismo lote, mandado
  otra vez, sale igual al secuencial, también después de reabrir.
- *Sin asignación después de guardar* (binario aparte): con el watch armado, el
  `operator new` reemplazado falla y cuenta toda asignación hecha dentro de una
  `no_allocation_region`. Esas regiones son la contabilidad posterior al `emplace` en el
  worker y la suma en el llamador. La carga tiene:
  - 6 000 entradas en 6 000 rangos de altura nuevos, con tamaños de valor de todo el
    rango de las clases 0 a 2;
  - un segundo lote, mitad repetido y mitad nuevo;
  - `mixed()`.

  Con W = 1, 2, 4 y 8: 0 asignaciones prohibidas y cifras iguales al secuencial. Hay un
  control del seam: armado, una asignación adentro de una región falla y se cuenta;
  desarmado, no marca nada.

**Resultados, revisión 1** (logs en `batch-09/r2/`):

| Build | `[batch]` (14 casos) | `utxoz_batch_alloc_tests` (3 casos) |
|---|---|---|
| `build-b9-1` (N1) | exit 0, 1420 aserciones | exit 0, 78 aserciones |
| `build-b9-8` (N8) | exit 0 | exit 0 |
| `build-asan9-8` (ASan + UBSan) | exit 0, 0 reportes | exit 0, 0 reportes |
| `build-tsan9-8` (TSan) | exit 0, 0 reportes | exit 0, 0 reportes |

La revisión 1 cambia el camino de inserción y los failpoints. Por eso se corrió también
el subconjunto que lo usa:
`[capacity],[rotation],[statistics],[failpoint],[failpoints],[telemetry],[transition],[concurrency],[ownership]`,
cada uno sin `[.slow]`. Dio 411 casos con exit 0, en N1 (407 pasan, 4 se saltean) y en
N8 (392 pasan, 19 se saltean); los salteos dependen de N o del perfil. No se repitió
`[recovery]` ni `[durability]`, porque la revisión no los toca.

**Mutaciones, revisión 1.** Después de cada una, la fuente se restauró y se verificó
por hash (`246d3e0b…`).

| Mutación | Build | Resultado |
|---|---|---|
| G1: `close()` no espera a las tareas en vuelo | N8 | `released == false` y SIGSEGV (exit 139) |
| G1 | TSan | `heap-use-after-free`, exit 66 |
| G2: la tarea apunta al gate sin ser dueña | ASan | `heap-use-after-free` en la copia tardía, exit 1 |
| A1: sin lugar para los rangos de altura | N8 | asignación en la suma: `terminate` (exit 134) |
| A2: sin lugar para los tamaños de valor | N8 | asignación en el worker: latch, el caso falla (exit 42) |
| A3: sin lugar para el registro de metadata | N8 | 1 asignación prohibida: el store nuevo no tiene registro para la generación vigente; el caso falla (exit 42) |

**La primera versión del test de G1 no detectaba la mutación.** Los fallbacks del
llamador logueaban (duplicados y rotaciones de `mixed()`) y se encolaban detrás del log
bloqueado, así que el llamador esperaba igual. Se le dio una carga en la que sólo la
tarea 0 loguea, y ahí G1 se detecta en los dos builds.

**Resultados, entrega original** (exit 0 en todos):

| Build | Qué | Resultado |
|---|---|---|
| `build-b9-1` (N1), `build-b9-8` (N8) | `[batch]` ×3 | 9/9 |
| `build-asan9-8` (ASan + UBSan, N8) | `[batch]` ×3 | 9/9, 0 reportes |
| `build-tsan9-8` (TSan, N8) | `[batch]` ×3 | 9/9, 0 reportes |
| `build-tsan9-8` | `[batch],[concurrency],[ownership]` | 34/34, 0 reportes |
| control positivo TSan | acumulador anulado (`acc = nullptr`) | data race reportado, exit 66 |
| b9-1, b9-8, asan9-8 | suite de riesgo¹ | 122 pasan y 2 se saltean² |

¹ `[batch],[capacity],[rotation],[statistics],[failpoint],[failpoints],[telemetry],[concurrency],[durability],[recovery],[transition]`,
sin `[.slow]`.

² Los 2 casos salteados son del perfil rung-exact, y este build no tiene ese perfil.

**Una falla de test, corregida.** En la primera corrida de la suite de riesgo con ASan
falló un caso. La aserción del ejecutor que tira suponía que su hilo reclamaba la tarea 0
antes que el llamador. Es una carrera del test, no de la biblioteca: el reclamo único
hace que cualquiera de los dos órdenes sea correcto. El caso se partió en dos:
- uno determinista, donde la tarea termina antes del `throw`;
- uno concurrente, que acepta los dos órdenes.

Después de eso, `[batch]` corrió 3 veces en los cuatro builds, sin fallas.

**Mutaciones** en N8, todas detectadas. Después de cada una, la fuente se restauró y se
verificó por hash.

| Mutación | Resultado |
|---|---|
| M1: orden invertido dentro del slot | fallan 4 casos |
| M2: no sumar los acumuladores | fallan 3 casos |
| M3: no sumar las estadísticas por altura | falla la equivalencia |
| M4: no parar ante el latch | falla el test del latch; el de rotación termina en SIGSEGV |
| M5: sin el fallback del ejecutor | se cuelga; SIGKILL a los 90 s |

**Límites declarados:**
- TSan instrumenta el código propio y los headers de Boost. libstdc++ no está
  instrumentada fuera de sus headers.
- Ningún job de CI corre TSan: `ci.yml` sólo corre `none` y `address,undefined`.
- Los 136 warnings del build ASan son `-Wstringop-overflow` de `boost/json`, que vienen
  de `test_sizing_tool` (falsos positivos de GCC 16). Ninguno viene de este cambio.
- Los builds b9-1, b9-8 y TSan compilan sin warnings. En la revisión 1 el build ASan
  recompiló todo y dio 5 warnings, todos el mismo `-Wstringop-overflow` de
  `boost/json` en los TUs de test.
- El test de asignación cubre las regiones marcadas, no el proceso entero. Una
  asignación nueva agregada después del `emplace`, fuera de esas regiones, no la vería.
- No hay test con 2³² entradas. El índice es `size_t`, así que no hay nada que proteger
  con un guard.

## Pendiente

- Decidir si `insert()` (el secuencial) recibe el mismo trato de memoria que el lote.
  Hoy puede asignar después de guardar.
- Auditoría de Codex antes de integrar con KTH.
- Una campaña de rendimiento (W × N), sólo con autorización y en una ventana
  coordinada.
- Un borrado en paralelo, que queda fuera de esta unidad.
- ~~El N final y la geometría siguen abiertos.~~ Se cerraron en 1.0: N=8 y geometría
  4 (`doc/roadmap-1.0.md`).
