# `file_cache`: modelo de mapeos, estado actual y follow-up de pinning

**Cierre documental.** El pinning queda **fuera del alcance de 1.0.0** y de este
PR de sharding; se registra aquí como trabajo posterior separado.

## Lo que rige para 1.0.0

| # | hecho | verificado en |
|---|---|---|
| 1 | **la generación activa de cada clase está siempre mapeada** en `segments_[Index]`, `std::array<unique_ptr<managed_mapped_file>, container_count>`. Con sharding: una por shard de la clase 48 | `database_impl.cpp:196,262,272` |
| 2 | **las generaciones históricas se abren bajo demanda** y las calientes quedan en el `file_cache`, con **LRU global** | `file_cache.hpp:101,159` |
| 3 | el presupuesto es global **a propósito**: así el shard realmente caliente puede quedarse con él. Un caché por shard lo fragmentaría y lo gastaría en shards fríos | decisión de diseño |
| 4 | `get_or_open_file` devuelve **referencias crudas**: `std::pair<utxo_map<...>&, bool>`. Nada mantiene vivo el mapeo | `file_cache.hpp:62` |
| 5 | **el desalojo cruza shards**: `evict_lru` elige por LRU global sin mirar de qué shard es la víctima, y `cache_.erase()` destruye el `unique_ptr` y desmapea | `file_cache.hpp:284-296` |
| 6 | **hoy eso lo protege `resolve_mutex_`**, que serializa las resoluciones enteras. Fue medido: 90 carreras de TSan y exit 139 antes de existir (#120) | `database_impl.hpp:519`, `database_impl.cpp:3577,4075` |
| 7 | **`is_pinned` existe y está muerto**: `evict_lru` lo respeta y **nadie lo escribe nunca**. El mecanismo está construido a la mitad | `file_cache.hpp:274,287-292` |

## Decisiones de alcance

- **`resolve_mutex_` global se conserva en 1.0.0.** Es correcto y suficiente.
- **`is_pinned` no se activa ni se elimina** como parte del sharding.
- **No se promete `resolve` concurrente en 1.0.0.**
- Un mutex por shard **no serviría**: la invariante rota es el desalojo cruzado,
  que un mutex por shard no evita. Esa conclusión mía anterior era falsa.

## Follow-up posterior a 1.0.0: `mapping_handle`

Un problema **preexistente**, independiente del sharding, que el sharding agrava
sólo en presión sobre el caché. La forma: separar **retirar del caché** de
**desmapear**, con un contador de pins por entrada —no un bool, porque dos
resoluciones pueden querer el mismo archivo— y el unmap diferido al último pin.
El LRU podría retirar sin bloquear al lector, y `resolve_mutex_` pasaría a cubrir
sólo la contabilidad del caché en vez de la resolución entera.

No se elimina el mutex por expectativa: el caché tiene además `access_frequency_`,
`gets_`, `hits_`, `evictions_`, `last_used` y `access_count`, todo estado mutable
compartido que el pinning no cubre.

Queda como issue separado.
