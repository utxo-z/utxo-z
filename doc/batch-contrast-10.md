# TAREA 10 — contraste multihilo breve de `insert_batch`

> **Medido el 2026-10-23, de 18:00:41 a 18:02:54 UTC**, en la ventana que concedió
> KTH: una sola corrida, 21/21 muestras válidas, y `--verify` con "complete". Los
> crudos están en `batch-10/run-contrast/`.
>
> Medianas en ms:
>
> | celda | mediana |
> |---|---|
> | N1-W1 | 264,253 |
> | N8-W1 | 380,560 |
> | N8-W2 | 226,929 |
> | N8-W4 | 162,232 |
> | N8-W8 | 121,503 |
> | N1 `insert()` | 225,891 |
> | N8 `insert()` | 250,897 |
>
> De N8 W1 a W8, unas 3,13 veces. Es exploratorio (3 repeticiones), con el perfil de
> test y sin inferencia. Los límites se listan abajo y en `doc/roadmap-1.0.md` §6.
> Lo que sigue es el protocolo tal como se congeló.

**Estado: READY para una ventana, pendiente de autorización (revisión 1).** No se corrió ninguna
medición cronometrada. La ejecución necesita una autorización escrita de Codex para
`TAREA10` (`scripts/campaign_auth.py`), coordinada con KTH #711. El token de TAREA07
no sirve: el runner lo rechaza (probado, exit 2).

## Qué se quiere ver

El beneficio principal de `insert_batch`: cuánto acorta la fase de inserción repartir
los slots entre W workers, con el mismo N, la misma API y la misma imagen. Es
exploratorio: 3 repeticiones por celda, sin estimador ni intervalo, y sin elegir N.

## Matriz

| Celda | Lado | API | W | CPUs |
|---|---|---|---|---|
| N1-W1 | N1 | `insert_batch` | 1 | `2` |
| N8-W1 | N8 | `insert_batch` | 1 | `2` |
| N8-W2 | N8 | `insert_batch` | 2 | `2,3` |
| N8-W4 | N8 | `insert_batch` | 4 | `2-5` |
| N8-W8 | N8 | `insert_batch` | 8 | `0-7` |
| N1-seq | N1 | `insert()` uno por uno | 1 | `2` |
| N8-seq | N8 | `insert()` uno por uno | 1 | `2` |

**Qué comparación se puede hacer y cuál no:**
- **N8 W1 contra N8 W2/4/8** es la comparación principal: misma N, misma API, mismas
  entradas. Lo único que cambia es W.
- **N1-W1 contra N8-W1:** mismo W y misma API, distinto N.
- **`-seq` contra W1 del mismo lado:** cuánto cuesta la API de lote frente a `insert()`.
  Es un diagnóstico de overhead, nunca una línea base de los workers. Como cambia la
  API, esa diferencia **no** se llama "beneficio del sharding".
- No hay N2 ni N4, ni se compilan: no se elige geometría en esta unidad.

**Aprovechamiento hasta los slots usados.** Con `value_size` 33 todas las entradas caen
en la clase 0:
- N1 tiene **un** slot, así que W > 1 no tiene a quién repartir. Por eso N1 sólo
  corre W1.
- N8 tiene 8 slots, así que W = 8 es el techo.

El harness informa cuántos workers usó cada llamada y cuántos slots ganaron entradas.
Si no se usaron `min(W, slots)` workers, la muestra queda inválida.

## Carga

Es el escenario de TAREA 07, sin cambios:
- la imagen de 6 000 000 de entradas de `grid-06/fixtures/N1` y `N8`;
- 3 000 000 de claves nuevas, con la regla de `calib_mutation.cpp` (claves, payloads de
  33 bytes y alturas `2 000 000 + i/1000`, es decir 1 000 salidas por altura).

`insert_batch` recibe `--batch-size 100000` pedidos por llamada: una fase preparada de
100 alturas, cada pedido con su altura. Son 30 llamadas. Sólo hay inserciones: nada de
BIP30, borrados ni `sync()`.

**Lo que mide `elapsed_ns`:** todas las llamadas a `insert_batch`, o el lazo de
`insert()` en `-seq`. Incluye lo que la API hace adentro de cada llamada:
- el ruteo y la preparación;
- los hilos propios que arranca (`run_tasks` vacío, así que crea `W−1` hilos por
  llamada);
- el join y la suma;
- un diagnóstico liviano del harness: una lectura de reloj por llamada (cada
  `pause_every` inserciones en `-seq`) y, en lote, copiar los contadores de cada llamada
  a un vector reservado antes del reloj.

**Lo que queda afuera:**
- la copia de la fuente;
- la verificación de la copia contra la regla;
- armar los pedidos;
- la validación final, el censo y las estadísticas.

**Ninguna muestra llama a `sync()`.** Lo que no se mide es la frontera durable
integral: las barreras de `sync()`. Las rotaciones que ocurren durante las inserciones
**sí** se miden, con lo que hacen adentro: cerrar la generación, escribir su metadata y
el writeback asincrónico que eso dispara. La copia se `fsync`ea antes del reloj, como en
TAREA 07.

## Afinidad

Cada celda corre con `taskset -c` sobre un conjunto declarado de **W núcleos físicos**.
Los hilos del lote heredan esa afinidad.

Antes de crear nada, el runner verifica los conjuntos contra
`/sys/devices/system/cpu` y `/sys/devices/system/node`:
- cada CPU está en línea;
- no hay dos CPUs del mismo núcleo físico (hermanas SMT);
- todos los conjuntos comparten **el mismo L3** y **el mismo nodo NUMA**.

Si algo falla, se rechaza con exit 2 y no se crea nada.

En esta máquina (Ryzen 9 9950X3D, 16 núcleos, SMT 2, un nodo, dos L3: núcleos 0–7 y
8–15), los conjuntos están todos en el L3 de los núcleos 0–7, sin hermanas (16–31).
Están anidados (`2` ⊂ `2,3` ⊂ `2-5` ⊂ `0-7`), y la CPU 2 es la de TAREA 07. W8 ocupa el
L3 entero, incluidas las CPUs 0 y 1.

El harness además informa la afinidad que efectivamente tuvo
(`spread.affinity_cpus`). Si no es la de la celda, la muestra no vale.

## Participación: lo que pasó, no lo pedido

Todo esto sale del harness. Lo pesado (censo, estadísticas, CPU) se lee fuera del reloj.
Dentro del reloj sólo está el diagnóstico liviano descrito en **Carga**.
- **Por llamada**, lo que devuelve `insert_batch`: `workers_used` y `tasks_run_inline`.
  Con los hilos propios del lote, una llamada con varios workers corre exactamente una
  tarea en el hilo llamador. Si corre más, faltó un hilo y la muestra no vale
  (`calls_short_of_threads`).
- **Por slot**, las entradas que ganó cada (clase, shard), del censo antes y después. Es
  el progreso real por slot.
- **CPU contra pared:** el tiempo de CPU (usuario + sistema) del proceso sobre la región
  medida, al lado de `elapsed_ns`. Si lo supera, el proceso tuvo hilos corriendo a la
  vez. Pero no dice cuáles, ni certifica que fueran los workers de inserción.

**Lo que esto no prueba:** `workers_used`, las tareas fuera del llamador y las
ganancias por slot prueban **participación**: cada tarea corrió en su propio hilo y
cada slot recibió lo suyo. **No prueban solapamiento** (que los workers corrieran a la
vez). Para eso haría falta otro control, que esta unidad no agrega: no se suma
instrumentación pesada a la primaria. Una mejora con W, si aparece, se lee como
indicio de paralelismo, no como prueba.

## Validación (cada muestra y entre muestras)

**El harness, en cada muestra:**
- el contenido inicial de la copia es la regla;
- el contenido final es la regla, por digest y conteo;
- `size()` y el censo coinciden;
- las estadísticas cuentan exactamente K inserciones;
- los slots ganaron K entradas en total;
- en lote, cero `failed`, `duplicate` y `not_applied`, ninguna llamada con error, y
  `min(W, slots)` workers.

**El runner, sobre el crudo de cada muestra** (`validate_sample`, revisión 1). Lo que
se espera sale del config, la celda y la repetición (`expected_for`). Lo que no cumple
el contrato deja la muestra inválida, y un crudo nunca se arregla después.
- **Identidad de la muestra:** schema; métrica; N; geometría; población;
  `value_size` 33; fuente; K; `pause_every`; la **semilla igual a la repetición**; y
  las marcas de lote (30 llamadas, o K/`pause_every`).
- **Tipos, con nada silencioso:**
  - los enteros son enteros, nunca `bool`;
  - `NaN` o `Infinity` en el crudo lo hacen inutilizable;
  - `elapsed_ns` es positivo;
  - `cpu_ns` no es negativo;
  - los digests y el token son 16 dígitos hex: un `None` igual a otro `None` no pasa.
- **Contenido:** el final es el esperado; 0 operaciones fallidas; y las entradas
  finales son población + K.
- **Slots:** exactamente N + 4 identidades (clase, shard) únicas, las esperadas. Cada
  ganancia es un entero no negativo, y sólo la clase 0 gana: una ganancia negativa o en
  otra clase no se compensa con la suma. La suma da K.
- **Lote:**
  - `workers_requested`, `batch_size` y `calls` son los del spec;
  - `workers_used_min = workers_used_max = min(W, slots usados)`: con esta carga, cada
    llamada toca todos los slots;
  - `tasks = calls × workers`;
  - `tasks_on_calling_thread = calls`;
  - `inserted = K`; duplicados, fallidos, no aplicados y llamadas con error en 0;
  - ninguna llamada sin todos sus hilos.
- **`-seq`:** todos esos contadores de lote en 0.
- **Afinidad:** la del harness es exactamente el conjunto de la celda.

**Entre muestras:**
- un único digest inicial y un único digest final en **todas** las celdas, N1 y N8
  incluidos: es la misma imagen lógica;
- las mismas rotaciones dentro de cada lado, sea cual sea la API o W.

**Al final:** cada fuente, byte a byte igual a como estaba.

## Guardas, logs y salida

- Autorización externa para `TAREA10`, leída antes de crear nada y releída antes de cada
  muestra. Sin ella, vencida, o de otra tarea: exit 2 y no se crea nada. El runner nunca
  escribe una autorización.
- El scratch es propio (`batch-10/scratch-parent/batchrun_*`), con un tope de 4 GiB.
  Antes de cada copia se exige que queden al menos 10 GiB libres; hoy hay unos 24 GiB.
- Sólo se borra una copia si es comprobadamente de esta corrida (marca de dueño con el
  token). La de una muestra fallida se conserva si entra en el presupuesto.
- El progreso no muestra tiempos:
  - `START` con la hora de reloj;
  - `alive` cada pocos segundos, sólo para indicar que sigue vivo;
  - `END` con el exit, la validez y la limpieza.
- Hay un timeout de 900 s por muestra.
- Exit: 0 completo, 2 rechazado antes de correr, 3 incompleto.
- La salida va a `run-contrast/`:
  - `config.json`, con su propio hash (`config_sha256`) y los hashes de: el spec, los
    módulos que corren (`run_batch_contrast.py`, `run_mut_grid.py`, `run_hist_grid.py`,
    `run_hist_pilot.py`, `campaign_auth.py`), la fuente del harness, cada binario, cada
    fuente de datos, la autorización y la topología;
  - `schedule.json`;
  - `raw/*.json|stderr|cmd`;
  - `report.json`, con cada `elapsed_ns` y `cpu_ns` crudos por celda y ningún número
    derivado.
- **Re-verificación** (revisión 2): `run_batch_contrast.py --verify run-contrast`
  vuelve a juzgar la corrida desde sus archivos. No depende de nada transcrito a mano.

  **Lo que se compara contra la evidencia guardada:**
  - el config, contra su propio hash;
  - el spec del config, contra `spec.json`, por hash y por contenido;
  - `schedule.json`, que tiene que ser **exactamente** el plan canónico que se deriva
    del spec (repetición r, las celdas rotadas r−1);
  - los registros: cada par (repetición, celda) del plan, una sola vez, en el orden del
    plan y sin pares ajenos;
  - cada crudo, contra el hash registrado, re-parseado con el mismo `validate_sample`;
  - los chequeos entre muestras y los valores por celda, recalculados;
  - `sources_after`: exactamente los lados del config, y el hash "después" de cada uno
    igual al hash "antes" del config. No alcanza con el `bool` `unchanged`, que además
    tiene que coincidir con esa comparación.

  **Lo que se re-hashea de los archivos actuales** (todos chicos):
  - los módulos del runner;
  - la fuente del harness;
  - los dos binarios del harness.

  **Lo que no se re-hashea:** las fuentes de datos, que pesan gigabytes. Para ellas,
  `--verify` compara la evidencia que la corrida guardó (sus hashes antes y después),
  no los archivos de hoy. Para la campaña real, Codex verifica binarios y fuentes por
  su cuenta.

  **Certifica sólo una corrida consistente y completa:** todos los pares del plan,
  todos válidos, sin problemas entre muestras, sin parada, y cada fuente igual.
  - Exit 0 sólo en ese caso.
  - Una corrida consistente pero incompleta, por ejemplo con una muestra fallida bien
    informada, da exit 3 con "INCOMPLETE: not certified".
  - Una inconsistente da exit 3 con la lista de problemas.

  La corrida se re-verifica a sí misma al terminar. Las funciones se pueden importar:
  `verify_run`, `validate_sample`, `expected_for` y `plan_of`.

## Pruebas, sin tiempos

- `scripts/test_batch_contrast.py`, **82/82, exit 0** (revisión 2; 74 en la revisión 1, 39 antes):
  - topología sobre `/sys` falsos: SMT, L3 cruzado, L3 distinto entre conjuntos, dos
    nodos NUMA, CPU fuera de línea, tamaño distinto de W, y una CPU sin nodo (no pasa
    como `None`);
  - topología real: los conjuntos propuestos pasan, y `2,18` se rechaza;
  - el runner con harness y autorizaciones falsas para una tarea de test:
    - una corrida completa con el progreso sin tiempos, y la rotación del orden;
    - sin autorización, vencida, de TAREA07, o con la topología mala: no se crea nada;
    - cada forma de muestra mala: afinidad, hilos faltantes, workers, estadísticas,
      slots, contenido, exit 3 del harness, cantidad de llamadas;
    - un caso negativo por cada familia de la revisión 1, 24 en total: la semilla
      distinta de la repetición, `pause_every`, `value_size`, las marcas de lote,
      duplicados, fallidos, no aplicados, llamadas con error, `tasks` ≠ llamadas ×
      workers, tareas en el llamador ≠ llamadas, workers mínimos de menos, contadores de
      lote en `-seq`, un slot repetido, un slot de menos, una ganancia `bool`, una
      ganancia negativa compensada, una ganancia en otra clase compensada, CPU negativa,
      CPU no entera, CPU `NaN`, `elapsed` `bool` o cero, digests `None` iguales, y un
      digest vacío;
    - `--verify`: una corrida intacta verifica (exit 0); un crudo editado después, un
      config editado, un valor del reporte que no sale del crudo, o un módulo distinto
      del que corrió, no (exit 3);
    - revisión 2, cada uno con exit 3:
      - schedule, registros, valores por celda y fuentes vacíos;
      - schedule y registros truncados a dos repeticiones;
      - registros truncados con el schedule intacto;
      - un registro duplicado;
      - un lado ausente en `sources_after`;
      - `unchanged` verdadero con un hash distinto;
      - el spec del config distinto de `spec.json`, aun recalculando el hash propio del
        config;
      - una muestra fallida bien informada: consistente, pero no certificada;
    - contenido o rotaciones distintos entre celdas;
    - una fuente escrita;
    - los límites del spec, incluidos `workers` `bool` o 0, `timeout` infinito y
      repeticiones `bool`.
  - **El smoke real:**
    - los harness reales de `build-b10-1/8`, sobre imágenes de **300 000** entradas,
      con 150 000 inserciones y lotes de 10 000;
    - las 7 celdas, una vez cada una, por el runner: todas válidas, y `--verify` sin
      problemas;
    - un solo contenido final en las 7;
    - N8 usó 1, 2, 4 y 8 workers, con una tarea por llamada en el llamador;
    - N1 tiene un solo slot con inserciones;
    - las 8 particiones de N8 ganaron entradas;
    - las fuentes quedaron intactas.

    Los tiempos del smoke no se leyeron ni se informan.
- `scripts/test_calib_mut.py`, **32/32, exit 0**, sobre `build-b10-1/8` recompilados en
  la revisión 1:
  - `insert_lifecycle` y `compact` siguen como estaban;
  - `insert_batch` W1 y W2 dan el mismo contenido que uno por uno;
  - clave existente y contenido distinto: exit 3;
  - 0 workers, y más workers que CPUs permitidas: exit 2.
- Rechazos del spec congelado, sin autorización y con la autorización de TAREA07: exit 2,
  nada creado.
- La geometría de `build-b10-1/8` es la de las imágenes de `grid-06`: `geometry_id` 3 y
  2147483666, iguales. Se verificó con una imagen de 300 000, sin abrir las de 6M.

## Binarios

- `build-b10-1` y `build-b10-8` son propios, con las mismas opciones que
  `build-grid1/8`: Release, `-m64`, `TESTING_RUNG_EXACT=ON`, estadísticas `basic`, log
  `custom`.
- Sólo se compilaron `utxoz_calib_mut` y `utxoz_calib_hist`. En la revisión 1 se
  recompiló `utxoz_calib_mut`, porque el harness ahora escribe en `spread` los totales
  de las llamadas, para que el runner los valide desde el crudo.
- Los binarios medidos de `build-grid*` no se tocaron. Sus hashes siguen siendo
  `d3b3681d…` (N1) y `7385f2f8…` (N8).
- No se compilaron N2 ni N4.

## Lanzamiento (sólo con autorización de Codex)

```
cd /home/fernando/dev/utxo-z/batch-10
python3 -I /home/fernando/dev/utxo-z/wt-shard/scripts/run_batch_contrast.py \
    --spec spec-contrast.json --out-dir run-contrast \
    --authorization <autorización TAREA10 de Codex> > run-contrast.stdout 2>&1
echo $? > run-contrast.exitcode
```

**Costo estimado** (con TAREA 07: 3–11 s por muestra, dominado por la copia):
- 21 muestras;
- el hash de las dos fuentes, al principio y al final.

Unos 5 minutos en total. **Ventana sugerida: 15 min.**

Los hashes congelados están en `batch-10/protocol.md`.

## Límites declarados

- **El tamaño de lote está fijo.** Son 100 000 pedidos (100 alturas), y una sola
  elección. Cada llamada crea sus `W−1` hilos, y ese costo está dentro de lo medido: es
  el ejecutor por defecto de la API, no el de KTH. Un ejecutor de pool, como el que
  podría pasar KTH, no se mide acá.
- **La región medida es corta.** En TAREA 07 eran unos 0,2 s de inserción, así que hay
  pocas llamadas y mucho ruido relativo. Tres repeticiones no permiten inferencia.
- **Las alturas son sintéticas**: 1 000 salidas por altura.
- **Hay un solo L3 y un solo `value_size`.** No se prueba cruzar L3, ni otra mezcla de
  clases. Con otras clases habría más slots y otro reparto.
- **CPUs 0 y 1 en W8.** Para tener 8 núcleos sin SMT dentro de un L3 hacen falta las
  CPUs 0 y 1, que pueden atender más interrupciones que las demás.
