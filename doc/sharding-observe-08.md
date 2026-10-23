# TAREA 08 — observabilidad de la compactación por shard: lista, sin medir

**Estado: preparación cerrada, sin campaña.** No se corrió ninguna medición y no hay
token para TAREA08. Quedan conservados: el arnés (`--observe-compaction`), el runner,
los tests, `obs-08/spec-observe.json` y los builds `build-obs1` y `build-obs8`. Correrla
requiere una autorización escrita de Codex (`scripts/campaign_auth.py`) y compilar antes
`build-obs2` y `build-obs4`.

## Qué se quiere ver

Lo que el sharding promete en compactación, y que TAREA 07 no midió: que cada unidad
de trabajo sea más chica (un `compact_container` por shard) y que el pico de espacio y
de memoria baje. No se busca un ganador ni se repite una calibración.

## Cómo se observa, sin tocar la biblioteca

Es un modo diagnóstico del arnés existente, `utxoz_calib_mut --metric compact
--observe-compaction`. Reusa la misma copia privada, la misma verificación previa al
reloj y la misma validación final de contenido.

**Marcas por contenedor**, tomadas con el callback público `utxoz::set_log_callback`.
Las fronteras se verificaron en el código (`src/database_impl.cpp`):

- `compact_all()` loguea `Starting full database compaction...`, vacía el caché de
  archivos y recorre cada clase y cada shard **en serie**. Cada `compact_container`
  emite `Starting compaction for class C shard S...` como **primera** sentencia.
- **Ningún contenedor loguea su final.** El siguiente mensaje es la marca del próximo
  contenedor o, al terminar, `Full database compaction complete`, que llega después de
  otro vaciado del caché.
- Por lo tanto **cada valor es un intervalo entre marcas, no una duración exacta.**
  Cubre el `compact_container` completo del contenedor (cierre, fusiones y
  reapertura), más el retorno, el paso del lazo, y el formateo y despacho de los
  mensajes intermedios. El último intervalo cubre además el vaciado final del caché.
  Así se rotula en la salida (`interval_meaning`).
- **Costo del callback.** El formateo de cada mensaje ocurre siempre, haya callback o
  no (`src/detail/log.hpp`: el mensaje se formatea antes de `dispatch_log_message`).
  Lo que agrega el callback es su propio cuerpo:
  - compara el prefijo y, si el mensaje no es una marca, sólo lo cuenta;
  - si es una marca, la parsea sin asignar memoria, lee el reloj y el RSS (un
    `pread` de `/proc/self/statm` sobre un fd abierto antes) y la guarda en un vector
    reservado de antemano;
  - no hace I/O pesado, y vuelca todo después del reloj.
- **Falla cerrado.** La secuencia tiene que ser exactamente: una marca de inicio, una
  por contenedor en el orden de `compact_all` (clase 0, shards 0..N−1; después las
  clases 1..4), una marca de fin, y relojes que nunca retroceden. Una marca que falte,
  se duplique, venga fuera de orden, no esté la de fin, o un mensaje que empiece como
  marca y no parsee (texto cambiado), invalida la muestra con exit 3.

**Pico de RSS.** `clear_refs` 5, justo antes de `compact_all`, resetea el
*high-water mark* del proceso. `VmHWM`, justo después, es **el high-water mark que
reporta el kernel** para ese intervalo. Nosotros no lo muestreamos, pero eso no lo
vuelve un máximo exacto:
- es lo que la contabilidad de RSS del kernel registró, con su granularidad y su forma
  de actualizar los contadores, que puede ir por detrás de la memoria residente real;
- es del proceso, no un máximo físico del sistema.

Se informa como "HWM reportado por el kernel". Se verificó que este kernel acepta el
reset (`VmHWM` vuelve a `VmRSS`); si no lo aceptara, la salida lo dice
(`rss_hwm_available: false`). **RSS del proceso no es RAM activa lógica:** incluye
las páginas de archivo mapeadas que se tocaron.

**Pico de espacio en disco.** Un hilo muestreador, fijado a otra CPU
(`--sampler-cpu`), suma cada `--sample-us` los bytes asignados (`st_blocks`) y lógicos
del directorio de trabajo. Informa el máximo **entre las muestras tomadas**, cuándo
ocurrió y cuántas muestras hubo. Un transitorio más corto que el intervalo puede
perderse, así que no se presenta como el pico absoluto.

**Separado de la primaria.** El callback y el muestreador son trabajo que el tiempo
total de TAREA 07 no tenía. El tiempo de `compact_all` que esta salida reporta lleva
`observed: true` y no se compara con aquél.

## Pruebas

- **`--selftest-marks`.** El validador reconoce la secuencia completa y rechaza la
  incompleta, la duplicada, la desordenada, la que no tiene fin, la que retrocede en el
  tiempo y la que trae un mensaje incompatible. El parser distingue marca, no-marca y
  marca reescrita.
- **`test_observe_compaction.py`, 23/23.** Usa el **arnés real** sobre imágenes de
  300 000 entradas, en N1 y N8 (`build-obs1/8`), sin reloj. Comprueba:
  - compactación observada válida, con el contenido final que predice la regla;
  - exactamente una marca por contenedor en orden: 5 en N1 y 12 en N8, más inicio y fin;
  - intervalos crecientes, muestras de disco tomadas y RSS declarado;
  - **el caso sin compactación** (`--skip-delete-setup`): secuencia completa y
    generaciones sin cambio;
  - que observar fuera de compactación se rechace;
  - fuentes byte a byte iguales.
- **`test_mut_observe.py`, 12/12** (runner de repeticiones, arneses y autorizaciones
  falsos). Cubre:
  - corrida completa con limpieza;
  - sin autorización o vencida;
  - marcas inválidas, otra geometría, sin muestras de disco, sin observación o
    contenido erróneo, conservando la copia fallida;
  - fuente escrita;
  - límites del spec (más de 3 repeticiones, muestreador en la CPU medida, scratch de
    más de 4 GiB).
- `test_calib_mut.py` (20/20) y `test_mut_grid.py` (25/25) siguen verdes con el código
  nuevo.

## Un error en esta unidad, corregido

Compilé el arnés nuevo dentro de `build-grid1`, y eso sobrescribió el binario
`utxoz_calib_mut` N1 **medido en TAREA 07**. Lo restauré recompilando ese único target
desde la fuente medida (`ee657fc`). Quedó **bit-idéntico** al hash registrado en
`mut-07/protocol-02.md` (`d3b3681d…`); el de N8 no se tocó. Desde entonces esta unidad
compila sólo en builds propios (`build-obs1`, `build-obs8`), y los cuatro binarios
medidos de `build-grid*` verifican contra sus hashes.

## Plan congelable (sin ejecutar)

`obs-08/spec-observe.json`, `task_id` = `TAREA08`:
- **Sides:** N = 1, 2, 4 y 8, sobre las fuentes inmutables de `grid-06`.
- **Repeticiones:** 3 por N, con los lados rotados por repetición. Sin pares y sin
  estadística confirmatoria.
- **Compactación:** después del mismo borrado del 50 % de TAREA 07.
- **CPUs:** la medida en la CPU 2 y el muestreador en la CPU 3.
- **Muestreo de disco:** cada 2 000 µs.

Costo estimado con TAREA 07: unos 15 s por repetición, cerca de 3–4 min en total más
el hash de las fuentes. **Ventana sugerida: 15 min.**

**Prerrequisito antes de la ventana:** compilar `utxoz_calib_mut` en `build-obs2` y
`build-obs4`, dos targets, con las mismas opciones. No se compilan mientras KTH esté
ocupado. Después se congela el plan: hashes de spec, binarios y fuente, y timestamp.

Lo que reportaría, sólo como descripción, por N:
- el intervalo por contenedor de la clase 0 (máximo y mediana entre shards y
  repeticiones), junto a la cantidad de fusiones;
- el pico muestreado de bytes del directorio y en qué intervalo cayó;
- el pico de RSS del proceso.

No reportaría ninguna comparación temporal confirmatoria ni ninguna elección de N.

**Factibilidad.** No hay bloqueo: todo usa API pública y `/proc`. Límites declarados:
- intervalos, no duraciones;
- pico de disco muestreado, no absoluto;
- RSS de proceso, no RAM activa;
- el texto de los mensajes de log no es contractual, y si cambia la observación falla
  cerrado.
