# Punto de operación histórico — A/A y control positivo, N=1

El punto estacionario aceptado mide 90 000 entradas en un contenedor: cabe en
caché y cada lookup lo contesta el contenedor activo. Calibró el instrumento ahí
y lo dijo. Esta unidad construye el punto que ese no podía alcanzar — un lookup
cuya respuesta **no** está en el activo — y le pone un control positivo de
trabajo real.

Plan predeclarado antes de la primera medida:
`hist-04/plan.md` (ruta completa abajo), sha256
`a730960ff5a95676aa7b44af80c09c3ad66559247332b00b2cfc19d6417041b3`, sellado
`2026-10-23T13:04:32Z`, más la enmienda `plan-02.md`,
`b117e9c79868c555da410f5211fb1a307e4e2d8564cbe7a6a0dbb319a0449b17`,
`2026-10-23T13:08:00Z` (abajo, "La enmienda").

Datos fuera del repo, en una ruta nueva: `/home/fernando/dev/utxo-z/hist-04/`,
652 MiB. Nada sobrescrito.

## El punto de operación

Leído del código, no supuesto. Clase 48, `sizeof(value_type)` = 84 B, perfil de
testing, `bucket_count` = 122 879 por generación, rotación a 133/160.

| | |
|---|---|
| población | 6 000 000 entradas, todas vivas |
| generaciones clase 48 | **59**: 58 llenas con 102 143 (carga 0,8312 = 133/160) + activa con 75 706 |
| frontera activa | `active_from` = 5 924 294, hallada preguntándole a `find()` por búsqueda binaria |
| historia | 5 924 294 entradas en 58 archivos = **580,0 MiB** |
| total lógico | 630,0 MiB en 63 archivos de versión, 5 clases, `geometry_id` 3 |
| etiqueta por entrada | altura `1000 + n/1000`, distinguible por clave |
| L3 de la CPU 2 | 96 MiB — el CCD con V-Cache del 9950X3D |
| **working set / L3** | **580,0 / 96 = 6,04×** |

Millones de sondas sobre 90 000 claves no llegan a eso por mucho que corran. Esa
es la razón del punto, y el censo por generación está en `hist6m/census.json`.

## La operación medida

Una op es un lookup histórico de punta a punta, por el contrato publicado:

    find(key, h)   -> not_resolved     las versiones activas no contestan
    resolve(lote)  -> found | absent   las viejas sí, y contestan

**Una ausencia cuenta sólo cuando `resolve` pone la clave en `absent`**, habiendo
leído toda versión que pudiera tenerla. `not_resolved` de `find` no es un miss:
dice que las activas no contestan, que es otra afirmación. El arnés rechaza
llamar miss a cualquier otra cosa, y el runner lo verifica.

El caché de archivos de la biblioteca guarda **un** archivo — su `max_size` por
defecto, que nada sobreescribe. Así que cada lote vuelve a mapear la historia.
Eso es el código como está, no una afinación.

| | |
|---|---|
| ops base por muestra | 160 000 (40 lotes de 4 000), el denominador |
| warmup | 8 000 (2 lotes), fuera del elapsed |
| sondas | uniformes, **sin reemplazo dentro del lote** |
| duración medida | 298–375 ms |

`ops` se eligió del costo del smoke previo (2399 y 2533 ns/op), sin mirar ningún
intervalo.

El tamaño de lote es parte del resultado: `resolve` amortiza los 58 open/map por
lote, así que una cifra sin el lote declarado no es reproducible.

## Resultado — A/A

Estimador predeclarado: bootstrap percentil de la mediana de ratios apareados,
3000 remuestreos, semilla 5. Familia de dos, Bonferroni **nominal**.

| métrica | mediana | IC 95 % | nominal simultáneo (97,5 %) |
|---|---|---|---|
| `hist_hit` | −4,262 % | [−4,945 %, +4,796 %] | [−4,955 %, **+4,883 %**] |
| `hist_miss` | +0,417 % | [−3,206 %, +4,037 %] | [−3,257 %, +4,155 %] |

**Peor límite superior nominal simultáneo: +4,883 %. No es compatible con un
margen de ±2 % en este punto.** Para comparar: en el punto estacionario el peor
límite fue +0,944 %.

## Resultado — control positivo

El lado B hace **5 % más consultas reales** de la idéntica distribución, dentro
del timing y fuera del denominador, sobre la **misma imagen**: 168 000 contra
160 000, intercaladas y no apiladas al final. Sin busy-wait, sin sleep, sin
corregir tiempos. El arnés cuenta el trabajo extra y el runner lo verifica contra
el porcentaje declarado.

| métrica | mediana | IC 95 % | nominal simultáneo (97,5 %) |
|---|---|---|---|
| `hist_hit` | +5,261 % | [+0,846 %, +10,249 %] | [+0,816 %, +10,258 %] |
| `hist_miss` | +7,730 % | [+2,555 %, +8,532 %] | [+2,530 %, +8,693 %] |

Criterio predeclarado: el IC excluye 1,0. **Lo excluye en las dos métricas, y lo
sigue excluyendo con el ajuste de familia.** El instrumento ve 5 % de trabajo
real extra en este punto, con las limitaciones de abajo: los mismos grupos por
orden y paridad de semilla aparecen en el control, y una detección no es potencia.

**+5 % de trabajo no es +5 % de tiempo**, y no se afirma que lo sea: costó lo que
costó. Lo que se sabe es cuánto trabajo extra se hizo.

## La diferencia por orden: un diagnóstico, no una causa

**Estado del A/A histórico: INCONCLUSIVE para un margen de 2 %.** Eso es lo que
dice el estimador predeclarado y no se reinterpreta.

Un hecho de los crudos, que se informa como diagnóstico: los ratios B/A se
agrupan según qué lado corrió primero. En el A/A de `hist_hit`:

    -4,9 +4,9 -5,3 +4,3 -4,5 +4,4 -5,0 +6,2 -4,9 +5,4
    -4,9 +6,0 -4,2 +4,7 -5,1 +5,3 -5,5 +5,6 -4,3 -6,9

| corrida | 2do vs 1ro (mediana) | pares AB | pares BA |
|---|---|---|---|
| A/A `hist_hit` | −4,95 % | −4,95 % | +5,08 % |
| A/A `hist_miss` | −3,72 % | −3,26 % | +4,24 % |
| control `hist_hit` | −4,41 % | +0,79 % | +10,30 % |
| control `hist_miss` | −2,78 % | +2,51 % | +8,53 % |

Lo que **no** se puede decir con estos datos:

- **Que sea un efecto de orden.** El orden está confundido con la **paridad de
  la semilla**: los pares impares corren AB y usan semilla impar, los pares
  corren BA y usan semilla par. Con este diseño no se puede separar "corrió
  segundo" de "semilla impar o par".
- **Cuál es la causa.** La versión anterior de este informe la atribuía a páginas
  no residentes por un warmup chico. Retirado: `munmap` no expulsa páginas del
  page cache, y el runner además **lee los 580 MiB del fixture completo** al
  hashearlo antes de la primera muestra. Nada de esto prueba residencia ni falta
  de ella, y el warmup no tiene atribución causal sin una prueba que la aísle.
- **Que el ancho "no sea ruido"**, frase también retirada. Lo que hay es una
  diferencia sistemática entre dos grupos de pares, de causa no identificada.

Tampoco se reemplaza el estimador. La versión anterior mostraba el punto medio
de los dos grupos como si "devolviera" el A/A y el control; elegir un estimador
después de ver los datos para aceptar un resultado es lo que el método prohíbe,
así que esa columna se quitó.

Declarado en el plan antes de medir y sin cambios: veinte corridas idénticas
seguidas dieron 1776–2344 ns/op, bimodal, rango 32 %. Es una observación de este
host con esta carga; no se corrigió y no se tocó el host.

## Validez operativa

Cuatro corridas de 40 muestras. En las cuatro:

| | |
|---|---|
| muestras | 40 por corrida, **0 fallas**, 20 pares efectivos por lado |
| crudos | 40 JSON + 40 stderr + 40 comandos por corrida |
| inválidas | **0** |
| errores reales | **0** |
| sondas contestadas por las activas | **0** — todas fueron genuinamente históricas |
| generaciones vistas | 59 en cada muestra |
| ops medidas | 6 400 000 por corrida A/A; 6 560 000 por corrida de control |
| store | idéntico antes y después: `3669b95f…` en las cuatro |

Verificación en la ronda 0: `recompute_aa_family.py` sin modificar, con
`run_hist_pilot.py` como `--runner`. Las dos familias salieron `verified-current`.
**Eso no certificaba el control.** El verificador reconstruía lo que cada crudo
debía decir sólo con los campos comunes a todo runner — sin `extra_pct`, sin
`batch` y sin mirar la imagen. Un lado B reescrito a 0 % de trabajo extra, con su
aritmética consistente y el hash del crudo rehecho, pasaba: probado, el
verificador de `178a098` lo acepta con exit 0.

Corregido en la revisión 1 con un contrato **opcional** que ofrece el runner y
que el verificador usa si existe:

- `expect_from_config(cfg, pair, side)`: la expectativa completa por lado. Es la
  misma función que usa el runner al medir, así que se verifica lo que se pidió y
  no una segunda descripción de eso.
- `verify_report(report, cfg)`: tipo de corrida coherente con el trabajo extra
  declarado, `extra_pct` de cada registro igual al que su lado debía, e imagen
  inmutable con los hashes del store antes y después iguales al del config.

El runner estacionario no ofrece el contrato y se verifica exactamente como
antes: la revalidación de los crudos de la confirmación A/A 02 da cifras y status
idénticos al `family.json` aceptado.

Revalidados con el contrato los crudos existentes, **sin medir nada**: A/A y
control salen íntegramente `verified-current`, con las mismas cifras de la ronda
0. Ahora sí el resultado del control queda verificado por lado.

Para `hist_hit` se verifican dos campos distinguibles por clave: el payload
codifica su índice y la altura codifica su bloque. Para `hist_miss` se verifica
que la ausencia fue **probada**. Es una **segunda pasada** después de parar el
reloj: establece que la imagen contesta bien esa secuencia, no que lo recibido
por el lazo medido fuera eso.

## La enmienda y las dos campañas vetadas

El guard de imagen del runner hasheaba todo el directorio, incluido
`.utxoz.lock`, que guarda el pid y la hora del reclamo y que la biblioteca
reescribe en cada open. La enmienda excluye **sólo** ese archivo y exige el resto
del store inmutable, byte a byte; cada caso tiene su test.

Las dos campañas que ese guard vetó **sí tuvieron muestras**: 40 cada una, todas
válidas para el arnés, preservadas en `hist-04/aa-hist_hit/` y
`hist-04/aa-hist_miss/`. Quedan rotuladas como **exploraciones previas vetadas**
en `hist-04/EXPLORATORY-vetoed.md`. Sus `report.json` no tienen estadística porque
el camino del veto saltea el bootstrap; no se calculó ni se miró un intervalo de
ellas y no se combinan con nada. Que no haya intervalo no las vuelve inexistentes.

Medido aparte: una muestra de medición reescribe cero bytes de los `.dat`
(comparación byte a byte de una generación de 10 MiB).

## Lo que no se afirma

1. **El control no es potencia.** Detectó 5 % una vez, en dos métricas. Eso no da
   la probabilidad de detectar 2 %, ni ninguna otra magnitud. Una detección no es
   una curva de potencia.
2. **Nada de esto garantiza falsos positivos.** Bonferroni es nominal: si cada
   intervalo cubriera a su tasa nominal, la afirmación familiar valdría al 95 %.
   Un bootstrap percentil de una mediana sobre veinte pares puede subcubrir, y
   más todavía cuando los ratios vienen en dos cúmulos.
3. **No es representatividad de producción.** El perfil de testing da 10 MiB y
   102 143 entradas por generación; producción da 1340 MiB y 13,07 M. La
   *estructura* — 59 generaciones, carga 0,8312, caché de un archivo, historia a
   6× L3 — sí es la real. Los *tamaños y la rotación* difieren, y con tamaños
   distintos el costo por lote y el número de mapeos cambian.
4. **`find_active_hit` no se midió.** En el perfil de testing el contenedor activo
   mide 10 MiB y no puede superar un L3 de 96 MiB; exigirle un working set mayor
   que L3 pide el perfil de producción, y varias generaciones de eso pasan el tope
   de 2 GiB. Queda fuera, explícitamente.
5. **No se eligió N.** Ni candidato, ni final, ni geometría.

## Lo que sigue

El plan concreto para comparar N = 1, 2, 4 y 8 está en
`doc/sharding-n-grid-plan-04.md`, para revisión y sin ejecutar. Tiene dos
decisiones previas que no son mías: un perfil de capacidad experimental que
mantenga la distribución comparable entre N, y qué hacer con un hallazgo en
`full_resolve` que dominaría cualquier medida con N > 1.

`insert` y `apply_deletes` siguen siendo otra unidad: mutan, así que necesitan
restaurar la imagen entre repeticiones.
