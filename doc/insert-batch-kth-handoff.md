# `full_db::insert_batch` — resumen contractual para integrar en KTH

Para el coordinador de KTH. Es un resumen del contrato que documentan
`include/utxoz/database.hpp` (`insert_request`, `insert_status`, `insert_outcome`,
`insert_batch_options`, `insert_batch_result`, `full_db::insert_batch`) y
`doc/insert-batch-09.md`. Si alguna vez difieren, manda el header. Este documento no
cambia KTH, ni su paquete, ni su lockfile.

## Qué es

```cpp
insert_batch_result full_db::insert_batch(std::span<insert_request const> requests,
                                          insert_batch_options const& options = {});
```

Inserta **una fase preparada** de entradas, cada una con su propia altura
(`insert_request { key, value, height }`), con **W workers configurables**
(`options.workers`; 0 y 1 = en el hilo llamador). Sólo existe en modo full.

## Hilos

- **Para el resto del programa es una sola mutación.** Rige el contrato de threading de
  `db_base`: no puede correr junto a otra mutación. La llamada vuelve cuando ninguna
  tarea está corriendo.
- **Un escritor por slot.** El slot es la clase más el shard. Dentro de un slot, los
  pedidos se aplican en su orden original, duplicados incluidos.
- **Workers W.** Se usan `min(max(1, W), slots con trabajo)` tareas. Con N = 1 y una sola
  clase hay un único slot, y W > 1 no reparte nada.

## Ejecutor (`options.run_tasks`)

- Es opcional. Si queda vacío y `workers > 1`, el lote crea hilos propios por llamada y
  los une (`join`) antes de volver.
- **El contrato es SÍNCRONO.** `run_tasks(count, task)` no puede volver, ni normalmente
  **ni tirando**, hasta que hayan vuelto todas las llamadas que hizo a `task`.
  - Un ejecutor que pasa `task` a otros hilos **tiene que unirlos** antes de volver,
    también en el camino de la excepción.
  - `task` vale hasta que `run_tasks` vuelve.
- Cada índice corre a lo sumo una vez, lo llame quien lo llame. Si el ejecutor no corrió
  una tarea, porque no arrancó nada o porque tiró, la corre el hilo llamador. El lote
  sale completo igual.
- **Copias tardías:** una llamada que llega después de que el lote terminó, por una
  *copia* de `task`, no corre nada y no toca nada. Es una **red de seguridad, no
  soporte para ejecutores asincrónicos**: la tarea que esa llamada quería correr ya la
  corrió el llamador. Usar la referencia `task` (no una copia) después de que
  `run_tasks` volvió es comportamiento indefinido.

## Resultado parcial y reintento

**No es atómico.** `outcomes[i]` dice qué pasó con cada pedido, por índice:

| Estado | Qué significa |
|---|---|
| `inserted` | Guardado. |
| `duplicate` | La clave ya estaba en su contenedor activo. No cambió nada. |
| `failed` + `error` | Rechazado. Con `entry_corrupt` el estado de esa entrada es desconocido y la instancia queda con latch: hay que reabrir y verificar. |
| `not_applied` | No se intentó. Sólo estos pedidos se pueden **reenviar tal cual**, una vez que el store esté usable. |

- Si algo falló o no se aplicó, `error` queda puesto: nunca hay éxito global con una
  parte aplicada.
- **Latch de integridad.** Una falla que puede dejar un slot inutilizable para a todos
  los workers antes de su *siguiente* pedido. Son:
  - una rotación que no puede crear su archivo;
  - un mapa mutado antes de una excepción;
  - una excepción no clasificada.

  Lo que ya estaba en vuelo termina y se informa como terminó, nunca como
  `not_applied`. Después de un latch, qué pedidos de otros slots llegaron a intentarse
  depende del timing: el estado guardado es el que dicen los outcomes.
- Una falla que no pone el latch (por ejemplo `value_too_large`) queda en su pedido, y el
  resto sigue.

## Memoria

- **Antes de mutar:** todo lo que el lote necesita para su contabilidad se asigna antes
  del primer pedido. Si falla, `insert_batch` **tira `std::bad_alloc` sin haber cambiado
  nada** y sin latch, y se puede reintentar el lote entero.
- **Desde el primer pedido no tira.** Contar una entrada guardada y sumar los contadores
  no asignan memoria.

## Durabilidad

`insert_batch` **no sincroniza**. Un `sync()` posterior cubre todos los slots que el lote
rotó. La durabilidad, como con `insert()`, es del llamador y va en el orden del
llamador.

## Lo que el lote NO hace

- **Unicidad:** el slot lo deciden la clase (por el tamaño del valor) y, en la clase con
  shards, la clave. El mismo **(clave, clase)** cae siempre en el mismo slot. Una clave
  mandada con valores de dos clases distintas queda guardada dos veces, como con dos
  `insert()`. **No hay validación global de unicidad**, ni de BIP30.
- **Dependencias entre bloques:** una fase puede traer varios bloques, cada pedido con su
  altura. Pero el lote no reordena nada entre slots, ni conoce gastos, borrados, undo o
  BIP30. Todo eso queda del lado del llamador y en su orden. Si un pedido depende de
  algo que no es una inserción, hay que **cortar el lote en esa barrera**.
- **Equivalencia con el secuencial:** sin latch, el resultado es el mismo que insertar
  uno por uno en orden. Esto vale para el contenido, las rotaciones, las estadísticas y
  la metadata.

## Estado

- La funcionalidad se aceptó en TAREA 09 (`7f21dfc`/`cd5d87a`).
- **Rendimiento** (TAREA 10, `doc/batch-contrast-10.md`): en N8, la mediana de W1 a W8
  bajó de 380,560 a 121,503 ms, unas 3,13 veces. Son 3 repeticiones, exploratorias,
  sobre una carga sintética con el perfil de test. **No** es una medida de la
  sincronización de KTH, y **no** dice que W8 sea óptimo en toda máquina. Con un solo
  worker, N8 fue más lento que N1 (TAREAs 06 y 07).
- **N=8 está fijo** en producción (`geometry_id = 4`): ver `doc/roadmap-1.0.md`.
- La integración con KTH no está aprobada.
- Codex entrega este resumen al coordinador después de revisar el bench.
