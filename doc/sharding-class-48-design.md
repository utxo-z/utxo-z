# Sharding de la clase de 48 bytes — diseño v4

> **Estado al cierre de 1.0 (TAREA 11).** Lo que manda es `doc/roadmap-1.0.md`.
> Lo que cambió respecto de este diseño:
> - **N=8 quedó fijo en producción**, con `geometry_id = 4`. Fue una decisión de
>   producto de Fernando, no la salida de la función de decisión de §5 y §11.
> - **Los umbrales obligatorios de ±2 % no se cumplieron y no se usaron para
>   decidir.** En un hilo, N=8 resultó más caro que N=1:
>   - TAREA 06: +51 % y +69 % en lectura histórica;
>   - TAREA 07: +11,9 % en inserción y +135 % en compactación, como regresión
>     nominal.
> - **Lo que N=8 gana es el reparto entre workers de `insert_batch`.** TAREA 10, N8 de
>   W1 a W8: 380,560 → 121,503 ms de mediana, unas 3,13 veces, con 3 repeticiones y
>   exploratorio. Es el perfil de test, no una sincronización de KTH.
> - Las cifras de §6 son aritméticas (tamaño de tabla y de archivo), no medidas en el
>   perfil de producción.
> - El resto de este documento queda como registro del diseño.

Reemplaza a v1 (`1520d40e…`), v2 (`a19fba51…`) y v3 (`9481116d…`).

## Alcance fijo de 1.0.0

| decisión | |
|---|---|
| clases shardeadas | **sólo la 48** |
| `file_cache` | **sin cambios**: LRU global, referencias crudas |
| `resolve_mutex_` | **global, conservado** |
| pinning / `mapping_handle` | **fuera de alcance**, follow-up separado |
| `is_pinned` muerto | **no se activa ni se elimina** |
| compactación | **máximo un shard a la vez** |
| paralelismo nuevo | **ninguno prometido**; no se promete `resolve` concurrente |
| generación activa en memoria | fuera de este PR |
| PRs | **uno de UTXO-Z**, un commit lógico · **cero de KTH** |

`utxo_gate_` no se modifica: el banco demostró que `get_utxo()` corre desde un
único hilo y que el gate cuesta el 0,006 % de la corrida. Ver
`utxo-gate-benchmark.md`.

Línea base: R2 (IBD a la altura 966.397) y R3c v3. La geometría D0 queda cerrada.
La generación activa en memoria es posterior a 1.0.0.

## Correcciones acumuladas

| versión | afirmación | veredicto |
|---|---|---|
| v1 | «15.728.639/8 es exacto, no se pierde nada» | **falsa** — se pierden N−1 buckets |
| v1 | «independencia por construcción» | **mal formulada** — es independencia *esperada*, y hay que medirla |
| v1 | `shard = byte 31 del txid` | **descartada por medición**: χ²=1556 con 7 gl |
| v1 | «compactación 1340 → 168 MiB» | **incompleta** — sólo con un shard por vez |
| v1, v2 | «la clase 0 daba atomicidad por accidente» | **falsa** — nunca la dio; ver §3 |
| v2 | función B, FNV sobre 28 bytes | **superada**: 10,04 ns/llamada, 8× más cara que la elegida |
| v1–v4 | «el archivo del shard es el de la clase dividido por N» | **falsa para el perfil de test**; ver §4bis |

---

## 1. `shard_of` — especificación normativa

### Dominio

    representación canónica del outpoint: los 36 bytes almacenados en raw_outpoint
      bytes  0..31  txid en ORDEN INTERNO (el inverso del hex de display)
                    byte 0 = raw_outpoint[0]
      bytes 32..35  output index, leído SIEMPRE como uint32 little-endian,
                    byte a byte, nunca por memcpy ni por reinterpretación

    N (shard_count): potencia de dos, 1 <= N <= 32. Fuera de ese dominio es un
    error de programación, verificado por valid_shard_count().

### Pseudocódigo normativo

    constantes:
      shard_lo  = 24                     // primer byte del campo de mezcla
      idx_mult  = 0x9E3779B97F4A7C15
      mix_a     = 0xBF58476D1CE4E5B9
      mix_b     = 0x94D049BB133111EB

    valid_shard_count(n) = n != 0 AND (n AND (n-1)) == 0 AND n <= 32

    le64_at(k, o) = k[o] | k[o+1]<<8 | ... | k[o+7]<<56      // ocho bytes, LE explícito
    le32_at(k, o) = k[o] | k[o+1]<<8 | k[o+2]<<16 | k[o+3]<<24

    shard_of(k, n):
        h = le64_at(k, 24) XOR (le32_at(k, 32) * idx_mult)   // 64 bits, wrap
        h = h XOR (h >> 30);  h = h * mix_a                  // splitmix64
        h = h XOR (h >> 27);  h = h * mix_b
        h = h XOR (h >> 31)
        return uint32(h) AND (n - 1)

Todas las operaciones son sobre `uint64_t` con envolvimiento definido. Los
desplazamientos son lógicos. `uint32(h)` toma los 32 bits bajos.

### Por qué los bytes 24..31

`hash_outpoint` construye su semilla con **los bytes 0..7** del txid. Usar los
bytes 24..31 deja los dos campos disjuntos, de modo que shard y bucket dependen
de partes distintas del digest. **No es independencia demostrada a priori** —los
bytes pertenecen al mismo SHA-256— sino *independencia esperada bajo el modelo
seudorrandom de SHA-256*, **verificada empíricamente** en §2.

### Propiedades

| propiedad | cómo se garantiza |
|---|---|
| `constexpr` | sin memoria dinámica, sin llamadas no-constexpr; los `static_assert` de los vectores compilan |
| `noexcept` | ninguna operación puede lanzar |
| independiente de endianness | cada lectura es byte a byte con pesos explícitos; nunca `memcpy` a un entero |
| independiente de `sizeof(size_t)` | todo es `uint64_t` y `uint32_t` de ancho fijo |
| independiente del compilador | sólo XOR, suma, multiplicación y desplazamiento sobre unsigned |
| independiente de Boost | no toca el hash del mapa |
| N=1 | `& 0` = 0 siempre: un único shard, comportamiento idéntico al actual |

**Punto fijo conocido:** un outpoint de 36 bytes en cero produce `h = 0` y por lo
tanto shard 0 para toda N. Es el outpoint nulo, que no existe en el conjunto
vivo; se documenta para que nadie lo tome por un defecto.

### Vectores dorados

Fijados con `static_assert` donde el compilador lo permite; el resto en una tabla
del test. Ver `sharding-analysis/golden-vectors.txt` para los 22 casos completos
con N ∈ {1,2,4,8,16,32}. Extracto:

    txid          idx          N=1 N=2 N=4 N=8 N=16 N=32
    ceros           0           0   0   0   0   0    0
    ceros           1           0   1   3   7   15   15
    ceros  4294967295           0   1   1   1   9    9
    unos            0           0   1   3   3   11   27
    rampa           0           0   0   0   4   12   28
    rampa           3           0   1   1   1   1    1
    rampa       65535           0   1   1   5   13   13
    0x7F            0           0   1   3   7   7    7

Cubre: txids de ceros, unos, rampa 0..31, 0x80, 0x7F, 0x01, 0xFE; índices
0, 1, 2, 3, 7, 255, 256, 65535 y UINT32_MAX; y los 16 primeros outputs de un
mismo txid, que se reparten 0-3-2-2-1-4-4-0 sobre ocho shards.

### Costo aislado, medido

Sobre **43.802.458 claves reales**, tres repeticiones, la última reportada:

| función | costo | desviación máx. N=8 | χ² N=8 (7 gl) |
|---|---|---|---|
| FNV-1a sobre 28 bytes | **10,043 ns** | 0,0720 % | 8,05 |
| **elegida: le64(24..31) + index, splitmix64** | **1,247 ns** | 0,0973 % | 12,90 |

**8,05× más barata con calidad equivalente.** Con `find()` costando del orden de
100–200 ns —un fallo de caché sobre una tabla de 1,32 GB— la función elegida
consume alrededor del 1 % del presupuesto y la de FNV entre el 5 y el 10 %. **El
umbral de regresión de `find()` es 2 %: la función cara no cabía.**

---

## 2. Distribución completa, N = 2, 4, 8, 16

Sobre las 43.802.458 claves reales de la clase 48, leídas con `for_each_entry`
desde una copia de trabajo de `R2-D0-audit/utxoz-copy`. La evidencia preservada
quedó intacta (34/34) y la copia se eliminó.

### Función elegida

| N | mín | máx | desv. máx | σ | χ² (gl) | χ² conjunta shard×hash (gl) |
|---|---|---|---|---|---|---|
| 2 | — | — | 0,0217 % | — | 2,06 (1) | 5,09 (7) |
| 4 | — | — | 0,0780 % | — | 9,58 (3) | 22,62 (21) |
| 8 | — | — | 0,0973 % | — | 12,90 (7) | 56,45 (49) |
| 16 | — | — | 0,1439 % | — | 20,92 (15) | 107,10 (105) |

Ninguna χ² es significativa al 0,01. Las conjuntas son consistentes con
independencia entre shard y hash del mapa.

### Función A, descartada — y por qué

| N | desv. máx | χ² (gl) |
|---|---|---|
| 2 | 0,2788 % | **340,50** (1) |
| 4 | 0,8583 % | **1295,23** (3) |
| 8 | 1,3218 % | **1556,24** (7) |
| 16 | 2,1137 % | **5094,33** (15) |

La causa, medida: **22.715.870 txids distintos** para 43,8 M outputs, media 1,93
outputs por txid y **máximo 31.243**. A nivel de txid el byte 31 es aceptable
(χ²=20,46 con 7 gl); a nivel de output la χ² se infla **entre 76 y 238 veces**,
porque la función A manda todos los outputs de una transacción al mismo shard.

### Cómo se eligió, y no sólo porque A falló

Tres candidatas medidas sobre las mismas claves:

| | uniformidad | independencia | costo | veredicto |
|---|---|---|---|---|
| **A** byte 31 del txid | **falla** (χ²=1556) | pasa | ~0,3 ns | descartada por uniformidad |
| **B** FNV-1a sobre bytes 8..35 | pasa (χ²=8,05) | pasa | 10,04 ns | descartada por **costo** |
| **C** le64(24..31) ⊕ index, splitmix64 | pasa (χ²=12,90) | pasa | **1,247 ns** | **elegida** |

B es marginalmente mejor en uniformidad y **ocho veces peor en costo**. Con el
umbral de 2 % sobre `find()`, la diferencia de uniformidad entre χ²=8 y χ²=13
—ambas no significativas— no compra los 8,8 ns. **C se elige por dominancia en
el criterio que decide, no por descarte.**

### Distribución por altura y por payload

| N | desv. máx por decil de altura | desv. máx por payload |
|---|---|---|
| 2 | 1,129 % | 0,018 % |
| 4 | 2,697 % | 0,084 % |
| 8 | 5,620 % | 0,162 % |
| 16 | 8,383 % | 0,385 % |

La desviación por decil de altura crece con N porque cada celda tiene menos
muestras; no indica sesgo estructural. Por payload es despreciable, como debe
ser: la clase 48 sólo tiene 41 y 43 bytes con volumen.

### Margen de rotación por shard

| N | umbral/shard | generaciones del más cargado | de la media | margen del más cargado |
|---|---|---|---|---|
| 2 | 6.537.215 | 4 | 4 | 16,2305 % |
| 4 | 3.268.607 | 4 | 4 | 16,2198 % |
| 8 | 1.634.303 | 4 | 4 | 16,1837 % |
| 16 | 817.151 | 4 | 4 | 16,1237 % |

**El shard más cargado necesita las mismas 4 generaciones que la media en toda
N.** El desbalance no fuerza generaciones extra, y el margen apenas cae del
16,2305 % al 16,1237 % entre N=2 y N=16.

---

## 3. Semántica de fallos: demostrada, no supuesta

### La afirmación de v1/v2 era falsa

Dije que «la clase 0 daba atomicidad por accidente de implementación». **No es
cierto.** La biblioteca **no tiene inserción por lote**: la firma es

    result<bool> insert(raw_outpoint const& key, output_data_span value, uint32_t height)

y KTH itera en `utxoz_database::apply_inserts`, **retornando al primer fallo y
dejando aplicadas las anteriores**. El progreso parcial dentro de un lote es la
semántica actual, y ocurre por encima de la biblioteca.

### Demostración empírica, clase 0 sin shards

Replicando literalmente el bucle de KTH sobre una base real:

| escenario | lote | falla en | visibles | tras reopen |
|---|---|---|---|---|
| lote completo, sin fallo | 5 | — | 5 | **5, igual** |
| falla en el primero | 5 | 0 | **0** | **0, igual** |
| falla en el segundo | 5 | 1 | **1** | **1, igual** |
| falla en el medio (5 de 10) | 10 | 5 | **5** | **5, igual** |
| duplicado dentro del lote | 4 | 2 | 3 | **3, igual** |
| único elemento, falla | 1 | 0 | **0** | **0, igual** |

**Regla observada: un fallo en la posición k deja exactamente k entradas
visibles, y eso sobrevive al reopen.** El error observado fue `value_too_large`;
el duplicado devuelve `false` sin error.

### Qué hace el sharding con esto

**Nada.** El bucle está por encima de la biblioteca, cada `insert` toca
exactamente un contenedor —hoy la clase 0, mañana un shard de la clase 0— y el
punto de fallo sigue siendo una llamada individual. Un lote que falla en la
posición k deja k entradas aplicadas, con o sin shards, repartidas en archivos
distintos que la API no expone.

**La semántica observable no se degrada, y no hace falta rollback, journal ni
staging.** Tampoco hace falta un cambio de contrato: no hay contrato que cambiar.

Lo que sí corresponde es **documentar la semántica que ya existe** —hoy no está
escrita en `database.hpp`— y eso es una mejora de documentación, no una
degradación. Cualquier cambio deliberado del contrato sería una decisión de
Fernando; **este diseño no propone ninguno.**

### Escenarios pendientes de demostrar con shards

Los que no pude ejercitar sin la implementación, y que el PR correspondiente debe
cubrir: excepción de capacidad, rotación preventiva fallida, `emplace` que falla
antes y después de mutar, y fallo de sync/publicación. Para cada uno, el criterio
es **igualdad observable con N=1**, que es exactamente lo que los PRs 3 y 4
permiten comprobar antes de activar N=8.

---

## 4ter. Consecuencia normativa: quién genera claves

`shard_of` lee **únicamente** los bytes 24..35 del outpoint (ocho del txid más el
índice de salida). Es la propiedad que §1 afirma y que los vectores dorados
fijan en tiempo de compilación, y tiene un corolario práctico que hay que dejar
escrito porque ya costó tiempo:

> Cualquier generador de claves que no varíe esos doce bytes produce claves
> distintas que caen **todas en el shard 0**.

El generador habitual de los tests —`memcpy` del contador sobre los primeros
ocho bytes, el resto en cero— es exactamente de ese tipo. Con él:

- un bucle que llena «hasta que cada shard haya rotado» no termina nunca;
- un test que simplemente llena cree ejercitar la clase y toca un solo
  contenedor;
- una medición de distribución sobre esas claves da un histograma degenerado que
  no dice nada sobre la función.

No es un defecto de `shard_of`: un txid de Bitcoin es un hash y no tiene cola de
baja entropía. El generador irreal era el de los tests. Queda `spread_key(n)` en
`tests/shard_paths.hpp`, que varía todos los bytes con un splitmix64 y conserva
el contador en los primeros ocho para garantizar la distinción.

La otra cara: un test *sobre un contenedor* —su cuenta de buckets a través de
una rotación, su guarda de capacidad, su compactación— necesita lo contrario,
que todas las claves caigan en el mismo shard. Para eso está
`key_in_shard(clase, shard, n)`, que busca por prueba en vez de construir una
inversa de `shard_of`, porque una inversa dejaría de coincidir en cuanto
cualquiera de las dos cambiara.

Antes del sharding las dos frases eran la misma; ahora hay que elegir cuál dice
cada test.

---

## 4bis-dos. La concesión de 8 KiB es una decisión, no una prueba

La cota que guarda las tablas de capacidad tiene dos mitades y sólo una es
derivada:

- **derivada**: el costo por slot es `sizeof(value_type)` más 16 bytes por grupo
  de quince, que es el layout documentado de `unordered_flat_map`. Vale donde
  valga ese layout y no hace falta medir nada para saberlo.
- **decisión**: el término fijo —gestor del segmento, índice de objetos con
  nombre, sello, redondeo del allocator— no se puede acotar para toda plataforma
  con ninguna expresión escrita acá. Se fijan **8 KiB**, unas cuatro veces lo que
  usan los segmentos de esta máquina.

De modo que las `static_assert` son un **diagnóstico conservador y barato**, no
una garantía de portabilidad. La garantía está en otra parte y a propósito: un
archivo demasiado chico para el mapa que se le pide debe fallar de forma definida
en tiempo de ejecución, validado por creación y rotación reales, que es lo que
ejercitan los casos de capacidad bajo el perfil `divide`.

Una versión anterior usaba `85067·buckets/1000 + 4096`, ajustada a corridas de
`utxoz_sizing` en esta máquina. El ajuste era bueno —coincide con la expresión
derivada a seis dígitos— pero un número obtenido ajustando mediciones de una
plataforma no es una cota.

---

## 4bis. El tamaño de archivo no se divide igual en los dos perfiles

Corrección de implementación, medida con `utxoz_sizing --class 48`.

El piso de un segmento —el tamaño mínimo con el que el mapa llega a
construirse— es lineal en los buckets sobre los peldaños reales de la escalera
de Boost:

    piso(b) ≈ 85,0668 · b + 2 106 bytes

ajustado sobre seis peldaños entre 491.519 y 15.728.639 buckets, con residuo
máximo de unos 2 KB. Los 2 KB del término independiente son el costo fijo del
segmento, y **no se dividen** cuando la clase se parte.

Dividir el archivo por N divide la carga y la holgura juntas, mientras el costo
fijo se queda entero. Los dos perfiles reaccionan distinto porque tienen
holguras muy distintas:

| perfil | archivo | piso medido | holgura | holgura relativa |
|---|---|---|---|---|
| producción, N=1 | 1.405.091.840 | 1.337.987.188 | 67.104.652 | 5,02 % |
| test, N=1 | 10.485.760 | 10.454.650 | 31.110 | 0,30 % |

Con el 5 % de producción la división sobrevive a todo N admitido:

| N | buckets/shard | piso medido | archivo/N | ¿entra? |
|---|---|---|---|---|
| 2 | 7.864.319 | 668.995.700 | 702.545.920 | sí |
| 8 | 1.966.079 | 167.252.084 | 175.636.480 | sí |
| 32 | 491.519 | 41.816.180 | 43.909.120 | sí |

Con el 0,30 % del perfil de test, la holgura se agota:

| N | buckets/shard | piso medido | archivo/N | margen |
|---|---|---|---|---|
| 8 | 15.359 | 1.308.275 | 1.310.720 | 2.445 B |
| 16 | 7.679 | 654.955 | 655.360 | 405 B |
| 32 | 3.839 | 328.283 | 327.680 | **−603 B: no entra** |

**Corrección de atribución.** El `bad_alloc` que motivó investigar esto ocurría
a N=8, y a N=8 el archivo dividido **sí** supera el piso —por 2.445 bytes—. La
causa real de ese fallo era otra y está registrada aparte: `capacity_for(Index)`
indexaba la tabla *por slot* con el número de clase, así que la clase de 10.240
bytes se construía con la capacidad del shard 4 de la clase 0 dentro de un
archivo de 10 MiB.

La separación de reglas por perfil se sostiene igual, pero por N=32, no por N=8:
allí el piso supera al archivo y ninguna elección de política lo salva. Y a
N=16 el margen es de 405 bytes sobre 655.360, que es un 0,06 % — un número sobre
el que no conviene apoyar una suite.

Los dos perfiles responden preguntas distintas y el sharding no cambia cuál:
producción fija la capacidad y toma el archivo que se midió para ella, así que
un shard con una fracción de las entradas toma esa fracción del archivo; el
perfil de test fija el archivo en diez megabytes y toma la capacidad que entra,
así que un shard sigue siendo de diez megabytes y simplemente guarda menos.

Implementado como `shard_file_rule::divide` y `shard_file_rule::keep` en
`capacity_policy.hpp`, con un `static_assert` contra `measured_segment_floor()`
que convierte el error en fallo de compilación en vez de un `bad_alloc` de Boost
en la primera creación.

---

## 4. Compactación serial y recuperación

| N | archivo/shard | pico 1 shard | 2 simultáneos | todos |
|---|---|---|---|---|
| 1 | 1340 MiB | 1340 MiB | — | 1340 MiB |
| 8 | **167 MiB** | 167 MiB | 335 MiB | 1340 MiB |

### Pico completo, no sólo el destino

El pico real coexiste: **destino + fuentes vivas + sidecar + catálogo**. Para
N=8, una compactación que fusiona dos generaciones de 167 MiB en una:

    destino          167 MiB
    fuentes vivas    334 MiB   (dos, todavía no retiradas)
    sidecar            < 1 MiB
    catálogo           < 1 MiB
    ------------------------
    pico             ~502 MiB   contra ~4020 MiB del equivalente sin shards

### Decisiones

- **un solo sidecar**, no N. Gana un campo `shard_id`. La razón es la invariante
  de un shard simultáneo: con un solo compactador activo, un solo sidecar
  describe sin ambigüedad qué está pendiente, y su ausencia significa que no hay
  nada pendiente. N sidecars permitirían representar un estado —dos
  compactaciones a medias— que la invariante prohíbe, y representar lo
  imposible es cómo se cuelan los defectos.
- **no se inicia una compactación mientras exista recuperación pendiente.**
  `refuse_if_recovery_pending()` ya lo hace para toda operación; la compactación
  hereda la regla sin código nuevo.
- **fairness reconstruible, no persistida**: round-robin sobre los shards que
  superan el criterio de fragmentación, empezando por el de **mayor versión
  activa**. Es una función del estado en disco, así que no hace falta persistir
  un cursor ni recuperarlo.
- **si el cleanup del shard A falla y B necesita compactar**: A queda con
  `cleanup_pending_`, la instancia latchea con `recovery_required` y B **no
  compacta**. La base entera se niega hasta reabrir. Es más restrictivo de lo
  estrictamente necesario y es deliberado: un shard roto no hace utilizable a la
  base.
- **reopen en cada frontera**: antes de escribir el destino, con el destino
  escrito y sin publicar, publicado y sin retirar fuentes, y con las fuentes
  retiradas. Cuatro pruebas por frontera.

---

## 4quater. Banco de mutaciones

Nueve mutaciones, cada una con la guarda que debe atraparla nombrada de
antemano. Una mutación que nadie atrapa es un hueco en la suite; el control
negativo es la otra mitad de la afirmación, porque un banco donde «todo falla»
no mide discriminación, sólo fragilidad.

Medido a N=8, corriendo la suite completa con `--stop-on-failure`.

| id | qué se altera | qué la atrapó |
|---|---|---|
| M1 | `shard_mix_offset` 24 → 25 | compilación |
| M2 | se anula el término del índice de salida | compilación |
| M3 | `slot_count` pierde los shards | compilación |
| M4 | todo shard usa el nombre sin shard | «a file whose header claims a creation in progress…» |
| M5 | el perfil de test divide su archivo | «compaction over a numbering with a hole in it» |
| M6 | N=8 declara la geometría 3 | «the shard count is part of the geometry…» |
| M7 | `insert` ignora la clave y usa el shard 0 | «compaction refuses a source whose map cannot be reached» |
| M8 | la política de merge pierde su slot | «a failed compaction leaves the database exactly as it was» |
| M10 | se desactiva el chequeo de colocación | «an entry filed in the wrong shard is reported…» |
| M9 | **control**: `!=` por `<` en una cota de bucle | nada — correcto |

**Limitación del método.** Con `--stop-on-failure` se aprende *una* guarda, no
todas. Basta para responder «¿la suite lo nota?», que es la pregunta, pero no
sirve para afirmar que una guarda concreta es la que lo nota. Eso hay que
preguntarlo aparte, y preguntarlo encontró un hueco: M7 apunta al chequeo de
colocación de `verify_unique` y fue reportada por un test de compactación. Al ir
a confirmar el chequeo resultó que **nadie lo miraba** — ni la herramienta lo
imprimía ni había test que lo ejercitara. De ahí salieron el reporte de
colocación, su inclusión en el veredicto de salida, el test que planta una
entrada en el shard equivocado, y M10 para probar que ese test es una guarda.

**Una versión anterior de este banco no medía nada.** Preguntaba «¿falla el test
llamado X?», y ninguno de sus filtros coincidía con un test real; Catch2 sale con
código distinto de cero cuando no encuentra coincidencias, así que todo veredicto
leía «atrapada». Sólo los de compilación eran válidos. Queda anotado porque el
modo de fallo —un banco que dice que sí a todo— es exactamente el que un banco de
mutaciones existe para descartar.

Dos notas sobre el diseño del banco, ambas por errores propios corregidos antes
de correrlo:

- La primera versión de M8 quitaba el constructor de `full_merge_policy`. Eso no
  es una mutación: sin constructor el tipo vuelve a ser agregado, `{*this, slot}`
  sigue compilando y sigue pasando el slot. La mutación real es el defecto que el
  constructor existe para impedir — que el slot quede en 0 en silencio.
- La primera versión de M9 cambiaba una constante de mezcla. Eso **sí** mueve
  claves entre shards y los vectores dorados deben atraparlo, así que no servía
  como control. El control tiene que ser demostrablemente neutro.

---

## 5. Los doce umbrales de adopción

> **No se aplicaron.** Los obligatorios de latencia (±2 %) no se cumplieron en lo
> medido, y N=8 se adoptó por decisión de producto. Ver `doc/roadmap-1.0.md` §6.

Predeclarados. **O** = obligatorio para 1.0.0, **I** = informativo.

| # | dimensión | umbral | unidad | dirección | tipo |
|---|---|---|---|---|---|
| 1 | latencia `find()` single-thread | **≤ +2 %**: límite **superior** del IC 95 % ≤ +2 % | ns/llamada | menor es mejor | **O** |
| 2 | latencia `insert()` single-thread | **≤ +2 %** | ns/llamada | menor es mejor | **O** |
| 3 | latencia `erase`/`apply_deletes` | **≤ +2 %** | ns/entrada | menor es mejor | **O** |
| 4 | `resolve()` por resolución | **≤ +0 %** (se espera mejora) | ns/resolución | menor es mejor | **O** |
| 5 | pico de compactación | **≤ 50 %** de 1340 MiB | MiB | menor es mejor | **O** |
| 6 | bytes físicos por entrada | **≤ +2 %** de 143,9080 | B/entrada | menor es mejor | **O** |
| 7 | bytes reservados por entrada | **≤ +5 %** de 177,6481 | B/entrada | menor es mejor | **O** |
| 8 | coherencia de archivos de la clase 48 | ver §5bis | — | — | **O** |
| 9 | balance entre shards | desv. máx **≤ 0,5 %** y χ² no significativa al 0,01 | % | menor es mejor | **O** |
| 10 | recuperación | **100 %** de las fronteras reabren al estado correcto | fronteras | mayor es mejor | **O** |
| 11 | estabilidad de shard entre plataformas | **0** discrepancias en los vectores dorados | casos | menor es mejor | **O** |
| 12 | RSS pico | ≤ +5 % de 7.136 MB | MB | menor es mejor | **I** |
| 13 | address space mapeado | ≤ +10 % | bytes | menor es mejor | **I** |
| 14 | throughput concurrente | sin umbral: **no alcanzable en 1.0.0** | ops/s | mayor es mejor | **I** |
| 15 | `generations_probed`/lookup | sin regresión | ratio | menor es mejor | **O** |
| 16 | `files_opened`/lookup | sin regresión | ratio | menor es mejor | **O** |

**Función de decisión.** N se elige entre los que superen **todos los
obligatorios**, minimizando en orden: (5) pico de compactación, (6) físico,
(8) archivos. **Si ninguno los supera, el sharding no entra en 1.0.0** y se
reporta como tal.

El umbral 1 es el que puede matar el diseño: sin paralelismo disponible, una
regresión en la ruta caliente no tiene compensación.

---

## 4quinquies. Decisiones D3, D4, D5 como quedaron implementadas

### D3 — compactación entre shards

Progreso secuencial y durable por contenedor, detención al primer error, sin
atomicidad global nueva. Los shards anteriores al fallo conservan lo que
fusionaron; los posteriores quedan intactos.

En el shard que falla hay **dos** estados, no uno, y no se prometen como si
fueran el mismo:

- **fallo antes de la publicación** — el duplicado se encuentra mientras se
  construye el target: las fuentes siguen todas ahí;
- **fallo de cleanup después de la publicación** — el target es durable y sus
  fuentes no se retiraron: la instancia se latchea con `recovery_required`, y lo
  que resuelve eso es reabrir, no reintentar en el lugar.

Probado en `test_compaction_invariant.cpp`, con el conjunto de claves y no sólo
el conteo de archivos. Dos precisiones que costaron un fallo cada una:

- el **multiconjunto** cambia legítimamente al recuperar, porque un target
  publicado con sus fuentes sin retirar tiene cada entrada dos veces; lo
  invariante es el conjunto de claves **distintas**;
- `all_keys` descarta el error de `for_each_key`, y una instancia latcheada
  rechaza el recorrido, así que el keyset se captura **antes** de armar el
  failpoint y se lee comprobando el resultado.

El caso del duplicado deliberado **no** afirma unicidad: el conjunto se duplicó a
propósito y una herramienta que lo llamara único estaría equivocada. El caso del
fallo de cleanup sí la afirma, porque ahí nada se duplicó a propósito.

### D4 — contenedores vacíos y metadatos

Se acepta la ausencia de metadatos derivados en un contenedor vacío. Un registro
se crea cuando una versión recibe su primera entrada, y con la clase 48 como un
solo contenedor eso nunca era alcanzable porque toda inserción de la clase iba
allí.

Lo que se exige y se prueba: la ausencia se distingue del daño
(`metadata_read_error::absent` contra cualquier otro), la primera inserción crea
el registro, sobrevive al reopen con las claves alcanzables, y un registro
truncado se rechaza con error definido sin impedir que la base abra ni encuentre
sus entradas. No se cambió eager/lazy.

### D5 — contrato de `verify_unique`

El éxito exige `unique && placement_correct`. Texto y JSON reportan las dos
dimensiones. Una clave **única** en el shard equivocado sale con **exit 2**: es
el cambio de contrato, porque el veredicto anterior habría sido 0. Con un shard
la colocación es siempre correcta, así que el estado de salida no cambia para la
geometría que se envía.

Probado ejecutando la herramienta contra una **copia** del directorio, con el
nombre del archivo escrito a mano en el test en vez de pedido a la misma función
que lo genera: un oráculo construido con el formateador productivo coincidiría
con una implementación equivocada igual que con una correcta.

---

## 5bis. Dos criterios reescritos

### Equivalencia de latencia: el límite superior, no el inferior

Para afirmar que una latencia cumple **≤ +2 %** hay que exigir que el **límite
superior** del intervalo de confianza de la regresión esté en ≤ +2 %. Ni el
límite inferior ni la ausencia de significancia sirven: ambos son compatibles con
una regresión mayor que el umbral y sólo dicen que no se pudo distinguir.

Si el instrumento no resuelve 2 %, el resultado es **INCONCLUSIVE**, no «cumple».
R3a medía una resolución de 7–8 % con 60 pares; certificar 2 % con ese
instrumento no es posible, y la salida es recalibrar —más pares, menos varianza—
o reportar inconcluso. El margen no se relaja después de ver los candidatos.

Esto aplica igual a `find()`, `insert()`, `erase`/`apply_deletes` y `resolve()`.

### Archivos: coherencia, no una cota

`≤ 2N + 2` era inválido contra su propia línea base: la clase 48 llegó a 37
generaciones en una corrida, de modo que con N=1 la cota pedía ≤ 4. Lo que
importa no es cuántos archivos hay sino que el conjunto sea coherente, y eso se
comprueba sin elegir un número:

- cada archivo de datos de un contenedor corresponde a una generación de su
  catálogo, y cada generación del catálogo a un archivo;
- ningún archivo de metadatos describe una versión que no está;
- un contenedor vacío puede no tener metadatos — es derivado y se reconstruye en
  la primera inserción (ver §4quinquies);
- los transitorios del protocolo (`.dat.building`, `.merge`, `.merge.tmp`) sólo
  existen durante la operación que los nombra, y la recuperación los resuelve.

**Archivos persistidos, mappings activos y caché histórico son tres cosas
distintas** y se cuentan por separado. Los mappings activos son `N + 4` —un
contenedor abierto por slot—; el caché histórico es aparte y acotado por su
propia política. Y no se cuentan VMAs de `/proc` como si equivalieran uno a uno
con un `managed_mapped_file`: un mapping puede aparecer dividido en varias
entradas, de modo que esa cuenta mide el mapa de memoria del proceso, no los
segmentos abiertos.

---

## 6. Qué justifica el sharding en 1.0.0, cuantificado

**No se usa el paralelismo futuro para aprobar una regresión presente.** Los tres
beneficios disponibles hoy, con su número:

| beneficio | hoy | con N=8 | mejora |
|---|---|---|---|
| **pico de compactación** (destino+fuentes+sidecar) | ~4.020 MiB | **~502 MiB** | **−87,5 %** |
| **unidad de mapeo histórico** | 1.321 MB por archivo abierto | **165 MB** | **−87,5 %** |
| **mapa activo por lookup** | 1.321 MB | **165 MB** | **−87,5 %** |

Los tres son aritméticos y se siguen del tamaño de la tabla, no de una
suposición. El tercero **no** implica mejora de latencia: con claves uniformes el
conjunto de trabajo total no cambia. Lo que cambia es la presión de TLB por mapa,
y eso está en el umbral 1 como riesgo, no como promesa.

**El beneficio principal y demostrable es el pico de compactación**, y es
operacional: la compactación final de R2-D0 tardó **70,9 segundos** y es el
momento de mayor presión de espacio de toda la corrida.

El cuarto beneficio —base de formato para el paralelismo y para la generación
activa en memoria— es real pero **no cuenta para la aprobación**.

---

## 7. Plan de PRs

> **Superado.** Lo que sigue es el plan de v3, de diez PRs incrementales. La
> decisión posterior fue **un único PR de UTXO-Z consolidado en un solo commit
> lógico**, cero o uno de KTH, y el pinning del file_cache como issue aparte. Se
> conserva la tabla porque el orden que describe sigue siendo el orden en que el
> trabajo se hizo y en que conviene leer el diff, pero no describe la entrega.
>
> Dos puntos concretos donde el plan quedó desactualizado:
>
> - **`geometry_id = 4`: se fijó en el cierre de 1.0 (TAREA 11).** Antes, los N
>   mayores que uno usaban ids experimentales en `0x8000_0000` y N=1 conservaba el
>   3. Ahora producción es N=8 con id 4, y todo otro N es un build experimental, no
>   instalable, con id `0x8000_00xx` (N=1 incluido).
> - **N=8 ya no es hipótesis: es la decisión de producto.** La opción
>   `UTXOZ_SHARD_COUNT` se eliminó y se rechaza. Ver `doc/roadmap-1.0.md`.


| # | PR | archivos productivos | ¿formato? | comportamiento observable | N | tests y mutaciones | revertible aislado |
|---|---|---|---|---|---|---|---|
| **0** | análisis de distribución | **ninguno** | no | ninguno | — | el análisis mismo | n/a — **hecho** |
| **1** | `shard_of` puro | `src/detail/shard.hpp` (nuevo) | **no** | **ninguno**: nadie la llama | — | vectores dorados con `static_assert`; uniformidad sobre claves reales; mutaciones 1–4 | **sí** |
| **2** | rechazo de geometría 3 + `geometry_id=4` + `shard_count` en config y stamp + parser de nombres + tests de formato integrados | `format_identity.hpp`, `store_config_io.hpp`, `segment_stamp.hpp`, enumerador | **sí** | **una base de geometría 3 se rechaza** | 1 | ida y vuelta del parser; rechazo de `cont_0_v…` como shard 0; mutaciones 6, 9 | sí, pero invalida bases escritas con él |
| **3** | estado por shard, N=1 | `database_impl.hpp/.cpp`, `version_catalog.hpp`, `file_cache.hpp` | no | **ninguno** con N=1 | 1 | toda la suite sin cambios de resultado | sí |
| **4** | lookup e inserción enrutados, N=1 | `database_impl.cpp` | no | **ninguno** con N=1 | 1 | ídem; mutación 5 | sí |
| **5** | compactación serial por shard | `database_impl.cpp`, `merge_sidecar.hpp` | **sí** (campo `shard_id` en el sidecar) | ninguno con N=1 | 1 | reopen en las cuatro fronteras; mutaciones 7, 8 | sí |
| **6** | census y verify_unique por shard | `census.cpp`, `uniqueness.cpp` | **sí** (esquema del JSON) | reportes con campo `shard` | 1 | las cuatro comprobaciones de unicidad | sí |
| **7** | **activar N=8** | `capacity_policy.hpp` | no *(el formato ya cambió en el PR 2)* | **cambia todo** | **8** | suite completa con N=8; benchmarks; los umbrales obligatorios | sí |
| **8** | benchmarks y criterio | `benchmarks/` | no | ninguno | — | piso A/A propio, N ∈ {1,4,8,16} | sí |
| **9** | integración KTH, docs, changelog | `conanfile.py` de KTH, `database.hpp` | no | documenta la semántica de fallos existente | 8 | — | sí |

**Corrección al plan de v2.** «Activar N=8 es una línea» era una mala idea: dejaba
el cambio de formato lejos del cambio de comportamiento. En v3 **el PR 2
concentra `geometry_id=4`, el rechazo de la geometría 3, `shard_count`, el parser
y los tests de formato**, de modo que a partir de ahí toda base escrita es
sharded-aware aunque N valga 1. El PR 7 sólo cambia el número, y para entonces
formato, estado, enrutamiento, compactación y herramientas ya están revisados y
probados.

### Mutaciones

1. `shard_of` devuelve siempre 0 → falla el test de uniformidad
2. usa los bytes 0..7 en vez de 24..31 → falla la independencia shard/bucket
3. omite el output index → falla la prueba de outputs del mismo txid
4. `% N` en vez de `& (N−1)` con N potencia de dos → **control negativo: no debe fallar**
5. la rotación de un shard mueve la versión de otro
6. el enumerador acepta `cont_0_v00013.dat` como shard 0
7. la compactación retira archivos de otro shard
8. dos shards compactan a la vez
9. `geometry_id` no se bumpea

---

## Lo que este diseño no promete

- **no promete paralelismo**: un solo mutex en la biblioteca, ventana global en KTH
- **no promete mejora de caché ni de latencia**: el umbral 1 es un riesgo, no una promesa
- **no promete mejora de densidad**: espera empeorarla levemente
- **no cambia el contrato de fallos**: documenta el que ya existe
- **no reabre la geometría D0**
- **no incluye la generación activa en memoria**

---

## 11. Criterios para elegir N — predeclarados

> **No se usaron para elegir N.** Ver la nota de §5 y `doc/roadmap-1.0.md`.

Fijados **antes** de correr el banco de N. Se aplican mecánicamente.

### Obligatorios: un N que falle cualquiera queda descartado

| # | criterio | umbral | medición |
|---|---|---|---|
| O1 | `find()` single-thread | **≤ +2 %** contra N=1, con el límite **superior** del IC 95 % por encima del piso A/A | banco in-process, bloques pareados |
| O2 | `insert()` single-thread | **≤ +2 %** contra N=1 | ídem |
| O3 | `erase` / `apply_deletes` | **≤ +2 %** contra N=1 | ídem |
| O4 | `resolve()` histórico por resolución, **con el mutex global** | **≤ +0 %**; se espera mejora por archivos más chicos | ídem |
| O5 | bytes físicos por entrada | **≤ +2 %** de 143,9080 | censo |
| O6 | bytes reservados por entrada | **≤ +5 %** de 177,6481 | censo |
| O7 | balance entre shards | desv. máx **≤ 0,5 %** y χ² no significativa al 0,01 | censo por shard |
| O8 | generaciones de la clase 48 por shard | **≤ 5** | censo |
| O9 | coherencia de archivos de la clase 48 (ver §5bis) | listado |
| O10 | pico de compactación serial | **≤ 50 %** de 1340 MiB | medido |
| O11 | mapeos permanentes | **≤ N + 4** (uno por shard más las otras clases) | `/proc/self/maps` |
| O12 | vectores dorados de `shard_of` | **0** discrepancias | test |

### Informativos: se reportan, no descartan

| # | criterio | referencia |
|---|---|---|
| I1 | RSS pico | 7.136 MB de R2-D0 |
| I2 | espacio de direcciones mapeado | — |
| I3 | costo aislado de `shard_of` | 1,247 ns medidos |
| I4 | `generations_probed` y `files_opened` por lookup | R2-D0 |

### Función de decisión

Entre los N que superen **los doce obligatorios**, se elige minimizando en este
orden estricto:

1. **O10**, pico de compactación
2. **O5**, bytes físicos por entrada
3. **O9**, archivos

Empate en los tres: gana el N menor, por simplicidad operativa.

**Si ningún N supera los obligatorios, el sharding no entra en 1.0.0** y se
reporta como tal.

---

## 12. Política de identidades experimentales

Durante el desarrollo **no se fija `geometry_id = 4`**. Las bases experimentales
son descartables y nunca adquieren promesa de compatibilidad.

| regla | |
|---|---|
| espacio de IDs | `0x8000_0000` en alto, igual que el `0x80000002` de D1 |
| asignación | **un ID distinto cada vez** que cambie N, `shard_of` o el layout |
| datadirs | **nuevos siempre**; jamás reutilizar el de otra variante |
| apertura | ninguna base se abre silenciosamente bajo otra variante: el ID no coincide y se rechaza |
| al cambiar el formato | se reconstruye desde cero |
| `geometry_id = 4` | **se fija recién al congelar el diseño para 1.0.0** |

Asignación durante el banco de N:

    0x80000010  N=2      0x80000011  N=4
    0x80000012  N=8      0x80000013  N=16

Ninguno sobrevive al congelamiento: el artefacto de 1.0.0 lleva `geometry_id = 4`
y rechaza todos los anteriores, experimentales y de producción por igual.

**Aplicado en el cierre de 1.0.** Producción: N=8, id 4. Los builds experimentales
(`UTXOZ_EXPERIMENTAL_SHARD_COUNT`) usan `0x8000000F` (N=1), `0x80000010` (N=2),
`0x80000011` (N=4), `0x80000013` (N=16) y `0x80000014` (N=32). `0x80000012` (el N=8
experimental de las campañas) ya no lo escribe ningún build.
