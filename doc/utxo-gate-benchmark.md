# Banco de `utxo_gate_` — resultados, interpretación y decisión

Medido en Labruna (32 cores, load 1,03), sin GitHub Actions, sobre el header real
`utxo_gate.hpp` de KTH `f143b7d8`. **Ningún código productivo modificado.**

---

## 1. El hallazgo que decide todo

Antes de interpretar un solo número: **`get_utxo()` se llama desde un único
hilo.** La validación de un lote tiene dos fases y sólo la segunda es paralela.

    src/node/src/sync/block_tasks.cpp:1962
        ::asio::thread_pool validation_pool(1);          // UN hilo

    src/blockchain/src/validate/batch_validate.cpp
        Fase 1 (serial):   reúne prevouts -> chain.get_utxo(op, height) por input
        Fase 2 (paralela): verificación de scripts y firmas, repartida en
                           hardware_concurrency() trabajadores
                           "Independent now that every prevout cache is populated"

La fase paralela **no hace lookups de UTXO**: para cuando arranca, la caché de
prevouts ya está poblada. Los `hardware_concurrency()` trabajadores verifican
firmas, no consultan el store.

**Por lo tanto la fila relevante de todo lo que sigue es la de 1 lector.** Las
filas de 8, 16 y 32 describen una concurrencia que hoy no existe sobre el gate.

---

## 2. Resultados crudos

### Gate aislado, sin trabajo bajo el lease

| variante | hilos | ns/op | ops/s | p50 (ns) | p95 (ns) | p99 (ns) |
|---|---|---|---|---|---|---|
| **A** lease por lookup | **1** | **52,43** | **19.071.660** | 30 | 40 | 40 |
| A | 2 | 55,44 | 18.037.599 | 60 | 250 | 350 |
| A | 4 | 119,91 | 8.339.393 | 210 | 1.800 | 3.110 |
| A | 8 | 182,78 | 5.471.107 | 890 | 4.960 | 7.700 |
| A | 16 | 275,09 | 3.635.144 | 170 | 17.270 | 25.140 |
| A | 32 | 294,80 | 3.392.085 | 140 | 50.820 | **102.030** |
| **B** un lease por lote de 100 | 1 | 68,70 (por lote) | 14.556.994 | 50 | 60 | 70 |
| B | 32 | 356,35 (por lote) | 2.806.254 | 3.740 | 54.420 | 105.670 |
| **C** contador atómico | 1 | 40,49 | 24.697.346 | 20 | 30 | 30 |
| C | 32 | **55,55** | **18.003.227** | 470 | 1.500 | **1.930** |

Bajo saturación, **A colapsa**: de 19,07 a 3,39 M ops/s, y su p99 pasa de 40 ns a
102 µs. C se mantiene casi plano y su p99 es **53× mejor**. B, medido por lote,
amortiza a 3,56 ns por lookup con 32 hilos.

### Latencia del escritor, con trabajo realista bajo el lease

Trabajo calibrado a **186,8 ns por lookup simulado**; un escritor pide la ventana
cada 300 µs.

**8 lectores:**

| lote | lookups/s | writer p50 (ns) | p95 (ns) | p99 (ns) | max (ns) |
|---|---|---|---|---|---|
| 1 | 472.953 | 1.690 | 11.810 | 14.950 | 23.950 |
| **10** | **4.333.013** | 3.820 | 10.540 | **12.760** | 18.710 |
| 50 | 5.408.409 | 11.340 | 12.490 | 16.410 | 24.181 |
| 100 | 4.113.596 | 20.760 | 22.850 | 30.110 | 35.270 |
| 500 | 6.916.810 | 95.110 | 101.161 | 140.730 | 209.641 |
| 2000 | 8.889.316 | 373.071 | 386.121 | 406.621 | 552.622 |

**32 lectores:**

| lote | lookups/s | writer p50 (ns) | p95 (ns) | p99 (ns) | max (ns) |
|---|---|---|---|---|---|
| 1 | 423.555 | 48.340 | 85.730 | 131.100 | 224.560 |
| 10 | 3.711.059 | 48.180 | 76.110 | 112.920 | 215.041 |
| **50** | **15.270.309** | 53.650 | 76.581 | **87.440** | 485.242 |
| 100 | 21.052.967 | 70.680 | 94.051 | 111.630 | 1.704.715 |
| 500 | 42.334.025 | 207.330 | 235.001 | 305.311 | 1.871.905 |
| 2000 | 65.929.703 | 736.882 | 781.953 | 2.167.437 | 2.514.598 |

Todos los valores de latencia en nanosegundos. Con 8 y 32 lectores, el lote de 10–50 da
**unas 9× a 36× más throughput**. La latencia del escritor no mejora en todos los
percentiles: es un compromiso.

- Con lote 10, p95, p99 y max bajan en los dos casos, pero el p50 sube con 8 lectores
  (1.690 → 3.820 ns).
- Con lote 50, el p50 sube en los dos (1.690 → 11.340 y 48.340 → 53.650 ns); con 8
  lectores también suben p95, p99 y max.

La razón es la preferencia de escritor: con lease por lookup el lector comprueba
`writers_waiting_` en cada iteración y se bloquea; con lote lo comprueba una vez
cada 10 o 50, y la contención sobre el mutex —que también demora al escritor— cae.

---

## 3. Interpretación

### Bajo la concurrencia real, el gate no es un cuello de botella

Con **un** hilo llamando `get_utxo`:

    52,43 ns por lease x 4.274.836 lookups de R2-D0 = 0,22 s
    sobre una corrida de 3.900 s = 0,006 %

**No hay contención lector–lector en la arquitectura actual**: no existe un
segundo lector del store. La **coordinación lector–writer permanece y es
deliberada** — es la que impide que un `close()`, una rotación o un desalojo
desmapeen bajo una referencia viva, y no se toca.

### Bajo concurrencia hipotética, A es malo y B lo arregla

Si algún día la fase 1 se paralelizara, A colapsaría —throughput de 19 a 3,4 M
ops/s, p99 de 40 ns a 102 µs— y B lo resolvería sin diseñar nada nuevo. Eso es
información valiosa **para ese día**, no para hoy.

### C no se adopta

El contador atómico escala mejor que todo lo demás, pero **A y B no fallan**, y
la regla es no diseñar reclamación lock-free sin necesidad. Queda como referencia
medida por si alguna vez hace falta.

---

## 4. Viabilidad de B, evaluada

Aunque no se adopte ahora, dejo verificado lo que se pidió, porque cambia si
mañana se paraleliza la fase 1:

| requisito | estado |
|---|---|
| ningún lease cruza un `co_await` | **compatible**: la fase 1 de `validate_block_batch` es sincrónica de punta a punta; no hay suspensión entre el primer y el último `get_utxo` |
| orden `validation_mutex_` → `utxo_gate` preservado | **sí**: B no cambia quién toma qué, sólo cuántas veces |
| ninguna referencia al mapeo sobrevive | **requiere cuidado**: `get_utxo` ya copia a `output_info`. Una API batch debe copiar igual, no devolver referencias |
| escritores no bloqueados durante scripts | **sí, y es el punto**: la fase 2 no toma el gate. El lease se soltaría al terminar la fase 1, antes de los scripts |
| resultados idénticos | sí: mismas llamadas, mismo orden |
| fallos parciales idénticos | sí: el bucle y su corte al primer fallo no cambian |
| cierre y compactación siguen esperando | sí: la preferencia de escritor sigue vigente |

**Forma preferida**, si alguna vez se hace: una **API batch interna** que reciba
una colección de outpoints y tome un solo lease, en vez de transportar la
capability por capas. El lote natural es la fase 1 completa de un bloque, y el
banco, en esta máquina, evaluó lotes de **10 y 50** como un compromiso entre
throughput y latencia del escritor: más throughput, con la cola más baja y el p50 que
puede subir en el lote 10, y el p50 más alto en el 50. No se definió una función de
utilidad ni se validó en una carga representativa, así que no se afirma que sea el
óptimo. Desde 100, el p50 del escritor sube mucho en los dos casos.

---

## 5. Decisión: **cero PRs de KTH**. `utxo_gate_` no se modifica.

Y no por margen estrecho, sino por arquitectura: **la fase que consulta el store
es serial por diseño**, y la fase paralela no lo consulta. El gate cuesta 0,006 %
de la corrida y no tiene con quién contender.

Cambiarlo ahora sería optimizar una contención que no ocurre, con el riesgo de
tocar la ruta que protege el lifetime de los mapeos.

**El lote de 10 a 50 queda como resultado preparado**, listo para el día en que
la fase 1 se paralelice.

**Lo que sí corresponde es dejar registrado el hallazgo**: si la fase 1 se
paraleliza alguna vez, el lease por lookup se vuelve un problema real y medido, y
la solución —lote de 10 a 50— ya está evaluada y no requiere diseño nuevo.

---

## 6. Límites de este banco

1. **El trabajo bajo el lease es simulado**, calibrado a 186,8 ns. Un lookup real
   con fallo de caché sobre 1,32 GB puede costar más, lo que alargaría las
   ventanas de B y empeoraría al escritor.
2. **El escritor pide cada 300 µs**, mucho más agresivo que la realidad: en KTH la
   ventana se toma una vez por lote de bloques, no en microsegundos.
3. **No se midieron ciclos ni instrucciones** ni contadores de línea de caché: no
   tengo `perf` disponible en este entorno. Las conclusiones se sostienen en
   throughput y latencia, que es lo que decide.
4. **No se confirmó con un workload de validación representativo.** No hizo falta:
   el hallazgo arquitectónico —un solo hilo— hace que la confirmación no pueda
   cambiar la decisión.
