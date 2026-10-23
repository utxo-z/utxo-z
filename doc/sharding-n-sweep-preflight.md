# Barrido de N: preflight de medición

**Nada de este documento reporta una medición.** Es la configuración, los
comandos y la estimación. Donde falta un dato, dice que falta.

## Qué es `err%`, según la fuente

nanobench **4.3.11** (`~/.conan2/p/nanob5231d515baf5d`). La columna `err%` es
`Result::medianAbsolutePercentError(Measure::elapsed) × 100`, y su
implementación es:

```cpp
auto med = calcMedian(data);
for (auto& x : data) { x = (x - med) / x; if (x < 0) x = -x; }
return calcMedian(data);
```

Es decir: la **mediana** de `|x − mediana(x)| / x` sobre los epochs de **una
misma corrida del proceso** (`mNumEpochs = 11` por defecto).

De ahí se siguen tres cosas que importan:

1. **No es una desviación estándar.** Es una dispersión robusta basada en
   medianas. Relacionarla con σ exige un supuesto distribucional, y el supuesto
   más simple —normalidad— es justo el que la evidencia previa de labruna
   contradice (ver abajo).
2. **No es entre procesos.** Mide variación entre epochs dentro de un proceso.
   La magnitud que decide una comparación A/B es la razón entre **procesos
   distintos**, que esta columna no observa.
3. **No es la resolución del instrumento.** La resolución sale del piso A/A
   medido con el mismo método con que se compara, no de una columna de resumen.

Por lo tanto: **no hay σ medido**. Los `err%` entre 0,0 % y 29,5 % del smoke son
dispersión intra-proceso de un smoke y nada más.

## Qué método usa este proyecto, y cuál no

El instrumento existente es `scripts/bench_analyze.py` del worktree `wt-bench`:

- `bootstrap_median_ci(ratios, resamples, confidence, seed)` — intervalo
  percentil por bootstrap sobre la **mediana de razones pareadas**, remuestreando
  pares completos para no romper el apareamiento;
- `classify_family(observed_median, half_width, family_floor, …)` — veredicto por
  sesión, exigiendo que la cota **simultánea** supere un piso familiar medido en
  el control A/A, no el piso por benchmark.

R3a corrió «A/A (5/5) y A/B runtime (5/20), una sola imagen, 60 pares» con
`bootstrap_median_ci(rr, 3000, 0.95, 5)`.

**Corrección de una ronda anterior.** Escribí que «7–8 % de resolución con 60
pares implica σ ≈ 29,6 %», invirtiendo con 1,96. Eso es inválido: supone un
modelo normal de una media independiente, y R3a usa medianas, bootstrap y una
regla familiar. La inferencia queda retirada, y con ella la cifra de «~3 400
pares», que venía de la misma cuenta.

La fórmula `n ≥ (1,96·σ/m)²` sólo vale como **ilustración** de que el número de
repeticiones crece con el cuadrado de la dispersión, bajo supuestos que acá no
se cumplen: muestras independientes, aproximación normal de la media, σ conocido.
No se usa para elegir `n`. El `n` sale del piso A/A medido con bootstrap.

## Sobre la varianza del host

El banco imprime un aviso de `cpufreq` (escalado activo, gobernador
`powersave`). **Ese aviso no es evidencia de causa** y no se usa como tal: no
muestra que dos procesos midieran a frecuencias distintas ni explica una
dispersión determinada.

La investigación previa sobre labruna encontró ciclos y elapsed estables,
recuentos de instrucciones iguales y bimodalidad **persistente incluso fijando
CPU**. La frecuencia no quedó demostrada como causa. Esa hipótesis está
descartada y no se recicla acá.

**No se cambia el host**: ni gobernador, ni `sysctl`, ni SMT. Lo autorizado y lo
único que se usa es fijar la CPU del propio proceso con `taskset`.

Recuperar los artefactos de R3a/R3b y su análisis de bimodalidad es trabajo
pendiente, y es la entrada correcta para decidir si +2 % es alcanzable.

## Configuración del piloto A/A — NO EJECUTADO

Métricas primarias, todas estacionarias, con estado inicial restaurado entre
repeticiones:

| métrica | workload | estado inicial |
|---|---|---|
| `find` hit | claves presentes, orden fijo por semilla | imagen restaurada |
| `find` miss | claves nunca almacenadas, orden fijo | imagen restaurada |
| `insert` | lote de tamaño fijo sobre imagen restaurada | imagen restaurada |
| `apply_deletes` | lote de tamaño fijo, todas presentes | imagen restaurada |
| `resolve` | lote fijo, caché declarada (frío y caliente por separado) | imagen restaurada |

Reglas:

- **setup/build/warmup separados de la medición**, y sólo la medición se
  cronometra;
- **la misma imagen N=1 en los dos lados** del A/A;
- longitudes, semillas y conteos de operaciones **fijos y predeclarados**;
- **orden intercalado predeclarado** (A B B A …), para que la deriva térmica caiga
  en los dos lados;
- CPU del propio proceso fijada con `taskset`; sin builds compitiendo durante la
  medición;
- se guardan los crudos, los hashes de binarios y fuentes, y `elapsed`,
  `cycles` e `instructions` cuando el host los dé;
- se verifica que el workload ejecutó lo declarado (conteos de operaciones,
  `erased`/`absent`/`unresolved` del lote) antes de mirar un tiempo.

Comando del piloto, 20 pares como máximo:

    # misma imagen en los dos lados: un único build N=1
    cmake -S . -B build-bench1 -G "Unix Makefiles" \
      -DCMAKE_MAKE_PROGRAM=/run/current-system/sw/bin/make \
      -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$TC \
      -DUTXOZ_BUILD_TESTS=OFF -DUTXOZ_BUILD_BENCHMARKS=ON \
      -DUTXOZ_BUILD_LARGE_BENCHMARKS=OFF -DUTXOZ_SHARD_COUNT=1
      # (comando histórico: en 1.0 UTXOZ_SHARD_COUNT se eliminó y se rechaza; hoy
      #  sería -DUTXOZ_EXPERIMENTAL_SHARD_COUNT=1, un build experimental no instalable)
    cmake --build build-bench1 --parallel 8
    sha256sum build-bench1/benchmarks/utxoz_benchmarks

    for r in $(seq 1 20); do
      for side in A B; do          # ambos lados son el MISMO binario: A/A
        taskset -c 2 ./build-bench1/benchmarks/utxoz_benchmarks \
          > aa-pair$r-$side.json 2>&1
      done
    done

Análisis con el instrumento existente, no uno nuevo:

    python3 wt-bench/scripts/bench_analyze.py   # bootstrap_median_ci + classify_family

**Criterio de cierre del piloto.** El piso familiar A/A es la resolución. Se
podrá certificar ≤ +2 % cuando el **límite superior** del IC de la regresión esté
en ≤ +2 %; ni el límite inferior ni la ausencia de significancia alcanzan. Si el
piso A/A no deja espacio para eso, el resultado es **INCONCLUSIVE**, con la
evidencia del piloto y el siguiente método acotado propuesto. El margen no se
relaja después de ver candidatas.

## Duración, medida

- build del banco regular: ~2 min por configuración (medido);
- una corrida del banco regular: **2 s** (medido, `exit 0`, N=1 y N=8);
- `--smoke` del target large con `--max-entries=40000`: **2 s a N=1, 5 s a N=8**
  (medido, `exit 0`, 28 KB de huella);
- piloto A/A de 20 pares: 40 corridas × 2 s ≈ **80 s** más setup.

El barrido de candidatas N∈{2,4,8,16} **no se planifica todavía**: primero el
piloto A/A decide si el margen es alcanzable.
