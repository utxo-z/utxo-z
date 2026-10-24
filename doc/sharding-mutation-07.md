# TAREA 07 — beneficios de inserción y compactación, N = 1/2/4/8

> Verificada con los runners de su momento. Con los mergeados en #149 un reporte sin
> `provenance_after` no se certifica: ver `doc/roadmap-1.0.md`, "Evidencia anterior a #149".
>
> **Al cierre de 1.0:** N quedó fijo en 8 por decisión de producto
> (`doc/roadmap-1.0.md`). Estas mediciones siguen valiendo tal como están: en un hilo,
> N=8 resultó más caro que N=1, y así se lo declara en 1.0.

**Estado: medido** en la ventana exclusiva otorgada por Codex (token
`1ff458a6…`, `TAREA07`, hasta 16:05:00Z), según `mut-07/LAUNCH.md`, sin cambios de
código ni de método. pre 15:21:45–15:25:40Z (exit 0), grid 15:25:53–15:42:13Z
(exit 0), **TIMING_FINISHED 15:42:25Z**. Datos en `mut-07/run-pre-2/`,
`mut-07/run-grid-2/`, familias `family-aa.json` y `family-grid.json`, manifiesto
`CAMPAIGN.MANIFEST.sha256` (515 entradas, `f25ab45f…`).

## Resultados

Tiempo total de la región medida, B/A (B = N, A = N1), 10 pares por celda,
estimador predeclarado (bootstrap de la mediana de ratios apareados, 3000, semilla 5).

**A/A N1/N1** (familia de 2, simultáneo nominal 97,5 %):

| | mediana | simultáneo nominal |
|---|---|---|
| `insert_lifecycle` | −0,71 % | [−17,68 %, +11,28 %] |
| `compact` | −0,78 % | [−3,55 %, +2,46 %] |

Son los intervalos de un A/A en este escenario: ancho en inserción y estrecho en
compactación. No son una cota universal del ruido del instrumento, y no se usan para
validar ni para anular ningún contraste.

**Grilla** (familia de 6, simultáneo nominal 99,1667 %). La clasificación sale
**sólo** del IC simultáneo nominal de cada comparación pareada, por la regla
predeclarada:

| N | `insert_lifecycle` Δ [simultáneo nominal] | clasificación | `compact` Δ [simultáneo nominal] | clasificación |
|---|---|---|---|---|
| 2 | +1,19 % [−16,11, +24,64] | **inconclusa** | +23,82 % [+17,60, +29,48] | regresión nominal |
| 4 | +16,20 % [+4,22, +29,02] | regresión nominal | +58,71 % [+52,44, +70,37] | regresión nominal |
| 8 | +11,89 % [+6,63, +29,78] | regresión nominal | +135,18 % [+106,05, +147,27] | regresión nominal |

"Regresión nominal" quiere decir que el límite inferior del IC simultáneo nominal
queda por encima de 0, en este escenario. Bonferroni es nominal, y con 10 pares el
bootstrap puede subcubrir. Una celda inconclusa no es equivalencia: N2 en inserción
no muestra ni ventaja ni desventaja establecida.

**Validez:** 160 muestras, 0 fallas; cada una sobre su propia copia, verificada
antes del reloj; todas arrancaron del mismo contenido en todo N; todas terminaron
con el contenido que la regla predice (digest y conteo) y exit 0; las fuentes de
`grid-06` sin cambios en las 8 celdas (hash completo antes y después); scratch
limpio, sin paradas, token sin modificar.

**Diagnósticos** (medianas por N, sin inferencia ni causa):

| métrica | N | región ms | rotaciones | generaciones | MiB | lote p99 ms | lote máx ms | RSS MiB |
|---|---|---|---|---|---|---|---|---|
| insert | 1 | 182 | 30 | 59→89 | 630→930 | 3,67 | 4,09 | 325 |
| insert | 2 | 188 | 60 | 118→178 | 629→929 | 3,60 | 4,03 | 325 |
| insert | 4 | 231 | 120 | 236→356 | 630→930 | 3,35 | 4,02 | 325 |
| insert | 8 | 224 | 239 | 472→711 | 632→931 | 2,47 | 3,17 | 325 |
| compact | 1 | 5 037 | 29 | 59→30 | 630→340 | — | — | 40 |
| compact | 2 | 6 266 | 58 | 118→60 | 629→340 | — | — | 30 |
| compact | 4 | 8 469 | 116 | 236→120 | 630→340 | — | — | 29 |
| compact | 8 | 11 927 | 232 | 472→240 | 632→341 | — | — | 29 |

En N=8 un shard rotó 29 veces y no 30: es determinista (las mismas claves en todas
las muestras) y es parte del efecto de N, no se descuenta.

## Lo que esto dice, y lo que no

**Inferencia pareada (las primarias):**
- **Compactación total con `compact_all()`:** regresión nominal en N = 2, 4 y 8, con
  ICs simultáneos de [+17,6, +29,5] %, [+52,4, +70,4] % y [+106,1, +147,3] %, para el
  mismo resultado final: mismas entradas, generaciones a la mitad, ~340 MiB en todo N.
- **Inserción total:** inconclusa en N2; regresión nominal en N4 y N8, con límites
  inferiores de +4,2 % y +6,6 %.
- **En resumen:** en este escenario **no observamos un beneficio total concluyente**
  de N>1 en ninguna primaria, y sí regresiones nominales en 5 de 6 celdas. Esto no
  demuestra que N=1 sea igual o mejor en general. Vale para este escenario: perfil de
  testing, 6 M entradas, un hilo y `compact_all` serial.

**Diagnósticos (sin inferencia, sin causa, sin extrapolar):**
- En inserción, el p99 y el máximo del tiempo por lote de 10 000 bajan con N: p99 de
  3,67 a 2,47 ms y máximo de 4,09 a 3,17 ms, de N1 a N8 (medianas por N).
- Es una observación empírica en archivos de 10 MiB. No se extrapola a los archivos de
  producción y no se atribuye a ninguna causa.

**Lo que el sharding busca y esto no cubrió:**
- **No es sólo concurrencia.** El diseño también apunta a operaciones más chicas por
  shard (rotación, compactación) y a un tamaño de contenedor activo que importa para la
  RAM activa futura. El propio `compact_all` lo dice: compacta un shard a la vez porque
  "the transient peak of a compaction is a whole target file".
- **Unidad de trabajo más chica no es lo mismo que pausa observable más chica.** Con N
  shards cada `compact_container` trabaja sobre 1/N de la clase. Pero `compact_all` los
  encadena en serie dentro de una sola llamada, así que quien la invoca ve una única
  pausa total, y esa pausa es la que medimos. La pausa por shard, que sería la ganancia
  si un llamador compactara por partes, **no se midió**. Es una limitación relevante, y
  el diseño no puede cerrarse sólo con el total.
- Tampoco se midieron el pico de bytes durante la compactación, la RAM activa ni la
  concurrencia.

**Escenario:** perfil de testing (rung exacto, 10 MiB por generación), no producción;
btrfs, que es copy-on-write en sus propias escrituras para todo N; un hilo, CPU 2.

## Qué mide exactamente el reloj (auditoría estática)

Del código del arnés (`benchmarks/calib_mutation.cpp`) y de la biblioteca
(`src/database_impl.cpp`), más los tiempos por muestra ya registrados. No se midió
nada nuevo.

**`insert_lifecycle`.** El reloj cubre **sólo** el lazo de 3 M `db.insert()`. Adentro
quedan:
- **Las rotaciones,** porque ocurren dentro de `insert()`: `new_version` →
  `close_container`, que hace `save_metadata_to_disk` (escritura atómica de metadata,
  sin barrera; `file_metadata_io.hpp` lo dice: "making that path synchronous puts an
  fsync on a rotation"), `segment->flush()` (writeback programado, asíncrono) y
  `munmap`; después, `create_container` del archivo nuevo.
- Los fallos de página de lo que se escribe.

Fuera del reloj quedan:
- la copia y su `fsync`;
- el open;
- la verificación del contenido inicial (6 M entradas recorridas y 6 M calculadas
  desde la regla);
- la construcción de los 3 M payloads;
- la validación final (9 M recorridas y 9 M calculadas);
- los dos censos;
- `close()`.

**Durabilidad.** Dentro del reloj sí hay trabajo de persistencia: las rotaciones
escriben la metadata (con reemplazo atómico, sin barrera) y programan el writeback de
la versión retirada (`flush` asíncrono). Lo que **no** mide es la frontera durable
completa: el arnés no llama `sync()`, y el writeback que el kernel complete después
queda fuera.

Por eso 3 M inserts caben en ~180 ms mientras la muestra entera dura ~2,8–7,2 s:

| métrica | N | proceso s | copia s | región s | resto s | región / proceso |
|---|---|---|---|---|---|---|
| insert | 1 | 2,80 | 1,04 | 0,182 | 1,57 | 6,5 % |
| insert | 8 | 7,24 | 5,21 | 0,224 | 1,81 | 3,1 % |
| compact | 1 | 8,78 | 1,07 | 5,04 | 2,67 | 57,4 % |
| compact | 8 | 21,20 | 5,19 | 11,93 | 4,09 | 56,3 % |

(N2 y N4, en los crudos.) El "resto" es la verificación, la validación, los censos y
el cierre. El tiempo de copia crece con N, y con N crece también la cantidad de
archivos. Es una asociación; no se probó que sea la causa.

**`compact`.** El reloj cubre `compact_all()` entero:
- vaciar el caché de archivos;
- para cada clase y, dentro de la clase, para cada shard **en serie**,
  `compact_container`: cerrar el activo, fusionar generaciones y reabrir.
- Cada fusión publica su resultado con el protocolo de durabilidad de la compactación:
  barreras `fsync` sobre el destino, el sidecar y el directorio (`directory_barrier`
  después del destino, del retiro de fuentes y del sidecar).
- Las fusiones fueron 29 en N1 y 232 en N8: el contador `rotations_class48` en
  compactación coincide con generaciones antes menos después.

Es un hecho del código que hay más fusiones, y con ellas más barreras, cuando hay más
shards. **No se afirma que eso sea la causa de la regresión;** no se midió.

## Punto de observación mínimo para la pausa por shard (propuesta, sin implementar ni medir)

Ya existe en la API pública, y no requiere tocar la biblioteca:
- `utxoz::set_log_callback` (backend `custom`, el de los builds de la grilla) recibe
  cada mensaje de log con su nivel.
- `compact_container` emite `log::debug("Starting compaction for class {} shard {}...")`
  al empezar cada shard, y `compact_all` emite "Full database compaction complete" al
  terminar.

Un modo diagnóstico del arnés que registre un `steady_clock` en cada uno de esos
mensajes da la duración de cada `compact_container`, sin cambiar nada productivo.

Cuidados:
- **Va en corridas separadas de la primaria.** Formatear y despachar los mensajes es
  trabajo dentro del reloj, y crece con la cantidad de shards.
- **Los mensajes son una interfaz no contractual.** Si cambian, el diagnóstico se
  rompe, y el arnés debería fallar cerrado si no los ve.
- Esto es verificación estática del código. **No se ejercitó.**

## La corrida no coordinada, excluida

`mut-07/run-pre` se arrancó antes, sin la ventana acordada, mientras corría la cola
de tests de KTH, y Codex la detuvo a las 14:46Z. Se preserva tal cual, rotulada en
`mut-07/INTERRUPTED-uncoordinated.md`; no se usa, no se atribuye y no se combina con
nada. De ahí salió el guard de autorización con el que corrió esta campaña.

## Qué mide la unidad

Partiendo de las imágenes inmutables de TAREA 06 (6 M entradas, rung exacto, N = 1, 2, 4
y 8). Cada muestra trabaja sobre una copia propia, verificada antes del reloj.

- **`insert_lifecycle`**: 3 000 000 inserts nuevos, los mismos en todo N. Cada contenedor
  de cada geometría rota ~30 veces, y eso es parte del efecto de N: no se descuenta nada.
  Primaria: el tiempo total. Diagnósticos: tiempos por lote de 10 000 (p50, p99, máximo),
  rotaciones y bytes.
- **`compact`**: `compact_all()` después de borrar, fuera del tiempo, las claves pares
  originales (50 %). Primaria: el tiempo total. Diagnósticos: generaciones, archivos y
  bytes antes y después, y RSS.
- **Insert estable** (ningún N rota) no es primaria: en N=8 caben ~24 k inserts, unos
  milisegundos de región medida.
- **Corridas:** A/A N1/N1 y luego N2/N4/N8 contra N1, 10 pares por celda. Familias de 2 y
  de 6.
- Protocolo `mut-07/protocol.md` más la enmienda `protocol-02.md`, y `LAUNCH.md` con los
  comandos exactos.

## Restauración y seguridad del scratch

- La copia se hace con read/write propio y `fsync`, no con reflink. Comprobado con
  `btrfs filesystem du`: 0 bytes compartidos con la fuente. btrfs sigue siendo
  copy-on-write en sus propias escrituras, igual para todo N.
- La copia se valida contra un digest de contenido calculado **desde la regla** con la que
  se construyó la imagen. Una copia de una fuente mutada fallaría.
- Cada muestra usa un directorio nuevo con su propio token. El runner borra una copia sólo
  después de guardar la muestra, sólo dentro de su propio scratch, sólo si tiene la marca
  de propiedad con ese token, y conserva las fallidas mientras el presupuesto alcance.
- Presupuesto: scratch ≤ 4 GiB y ≥ 10 GiB libres, controlados antes de cada muestra.

## Pruebas

- `test_mut_grid.py`: 25/25 con arneses falsos y autorizaciones falsas creadas por el test.
  Cubre:
  - fuente mutada;
  - herencia de directorio;
  - contenido inicial distinto;
  - contenido final o conteo erróneo;
  - geometría ajena;
  - limpieza que no toca lo ajeno (fuera del scratch, marca con otro token);
  - espacio y presupuesto;
  - crudo contra registro;
  - y el guard: sin token, inexistente, ilegible, vencido, otro host, otra tarea, sin
    código, fecha mal formada, más de un día, revocado a mitad de campaña, hash registrado.
- `test_calib_mut.py`: 20/20 con el **arnés real** sobre imágenes de 300 000 entradas en N1
  y N8, sin reloj. Comprueba:
  - las corridas válidas validan;
  - los autotests fallan con exit 3 y el mal uso con exit 2;
  - cada corrida usa su propio directorio;
  - todas arrancan del mismo contenido;
  - las fuentes quedan byte a byte iguales.
- El runner real con el spec real y sin autorización sale con exit 2 sin crear nada.
- Las fuentes de `grid-06` no se escribieron durante TAREA 07: su última escritura es del
  diagnóstico de TAREA 06. El hash completo lo hace el runner al principio y al final de la
  campaña autorizada.

## Objetivos aún no cubiertos para elegir N (producto)

Fernando fijó 1.0.0 con sharding; esto no elige N ni cancela el sharding. Lo que falta
para que la elección tenga evidencia de los dos lados:
1. **Pausa por shard en compactación** (punto de observación de arriba), y una política
   de compactación por partes si se quiere que un llamador vea pausas de 1/N. Hoy
   `compact_all` es serial y entero.
2. **Pico de bytes y de memoria durante la compactación,** por N.
3. **RAM activa y tamaño del contenedor activo** en los tamaños de producción
   (1340 MiB por generación), no en los 10 MiB de testing.
4. **Concurrencia,** si alguna vez se implementa inserción o compactación en paralelo
   por shard. Hoy no existe.
5. **Lectura histórica:** el costo medido en TAREA 06 se asocia a ×N archivos abiertos
   por lote, según los contadores; la palanca (agrupar el lote por shard, o el caché)
   queda para decisión.
6. **Un punto de operación más cercano a producción** para inserción, donde la
   dispersión A/A de ±15 % actual no permite resolver diferencias chicas.

## Pendiente

- **Validaciones de release pendientes:**
  - sanitizers (ASAN/UBSAN) sobre el enrutado de TAREA 05, el perfil rung-exacto y los
    arneses;
  - mutaciones del perfil rung-exacto (M5 del enrutado sigue sin cubrir);
  - suite completa una vez, sobre el commit consolidado, en N1 y N8;
  - cobertura de las rutas nuevas;
  - consolidación de los commits en el PR único;
  - la decisión de N, que es de producto.
