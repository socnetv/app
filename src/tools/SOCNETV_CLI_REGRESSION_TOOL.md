# SocNetV CLI Regression Tool

This is a headless regression tool used to verify
algorithmic correctness during the ongoing architectural refactor of
SocNetV.

The tool provides deterministic execution of the distance / centrality
kernel without loading any UI components.

It was originally designed to protect the refactor of:

    Graph::graphDistancesGeodesic()
    → DistanceEngine

---

## Purpose

The CLI enables:

- Headless dataset loading (no MainWindow / GraphicsWidget)
- Deterministic execution of DistanceEngine
- Golden-output JSON generation
- Strict regression comparison against committed baselines
- CI integration (fail-fast on mismatch)

This ensures that refactors preserve:

- numeric results
- connectivity bookkeeping
- centrality vectors
- directed/weighted semantics

---


## Design Principles

* No UI involvement
* No graphics dependency
* Deterministic ordering
* Deterministic float formatting
* Zero semantic modification of Graph

The CLI is a safety harness, not a new analytics engine.


---

## Build

The tool is built as:

    socnetv-cli

It is compiled alongside the main application via CMake.

---

## Basic Usage

Load a dataset and print summary metrics:

```bash
./socnetv-cli \
  -i src/data/SmallWorld_N10_E12.graphml \
  -f 1
```

Common file types (see `global.h`):

* 1 → GRAPHML
* 2 → PAJEK
* 3 → ADJACENCY
* 5 → UCINET
* etc.

---

## Golden Output Dump

Generate a deterministic JSON baseline:

```bash
./socnetv-cli \
  -i src/data/SmallWorld_N10_E12.graphml \
  -f 1 \
  --dump-json src/tools/baselines/SmallWorld_N10_E12__C1_W0_IW1_DI0.json
```

The filename convention encodes run flags:

```
C1 = computeCentralities=1
W0 = considerWeights=0
IW1 = inverseWeights=1
DI0 = dropIsolates=0
```

---

## Golden Output Compare

Strict comparison against a baseline:

```bash
./build/socnetv-cli \
  -i src/data/SmallWorld_N10_E12.graphml \
  -f 1 \
  --compare-json src/tools/baselines/SmallWorld_N10_E12__C1_W0_IW1_DI0.json
```

On mismatch, the tool prints exact field differences and exits non-zero.

This makes it suitable for:

* CI pipelines
* pre-merge validation
* refactor checkpoints

---

## What Is Verified

### Graph-level metrics

* nodes
* links_sna (loader semantics)
* ties_graph (canonical Graph model)
* directed / weighted
* average geodesic distance
* diameter
* disconnected_pairs
* connected

### Per-node vectors (when centralities enabled)

For each vertex (deterministic ascending id order):

* CC / SCC
* BC / SBC
* SC / SSC
* EC / SEC
* PC / SPC
* distance_sum
* eccentricity

Floating-point values are serialized as strings to avoid JSON
double-format inconsistencies.

---

## Micro-Benchmarking Mode

The CLI also provides a lightweight benchmarking mode to measure
DistanceEngine compute performance during refactors.

This mode does **not** modify algorithmic behavior and does not
generate or compare JSON output.

It is intended to:

* Detect performance regressions
* Track kernel timing across refactor phases
* Provide repeatable compute-only measurements

---

### Usage

```bash
./build/socnetv-cli \
  -i src/data/Stephenson_Zelen_Dunbar_Dunbar_Gelada_baboon_colony_H22a_IC.paj \
  -f 2 \
  -c 1 -w 1 -x 1 -k 0 \
  --bench 10
```

Where:

* `--bench N` runs:

  * 1 warmup execution (not measured)
  * N measured executions of the compute kernel

---

### Output

Example:

```
COMPUTE_RUNS=10
COMPUTE_MS_MIN=4
COMPUTE_MS_MEDIAN=4
COMPUTE_MS_MEAN=4.200
COMPUTE_MS_MAX=5
```

Metrics:

* `COMPUTE_RUNS` → number of measured runs
* `COMPUTE_MS_MIN` → fastest run
* `COMPUTE_MS_MEDIAN` → median time
* `COMPUTE_MS_MEAN` → arithmetic mean (ms)
* `COMPUTE_MS_MAX` → slowest run

In normal (non-bench) mode, a single run prints:

```
COMPUTE_MS=7
```

---

### Constraints

* `--bench` cannot be combined with:

  * `--dump-json`
  * `--compare-json`

* Bench mode measures **only** the distance / centrality compute kernel.

* Dataset loading time is reported separately via `LOAD_MS`.

---

### Design Notes

* Each measured run resets internal cache flags before execution to avoid early-return short-circuiting.
* No algorithmic paths are modified.
* Bench mode does not affect JSON schema or regression comparison logic.
* Intended for developer use, not end-user profiling.

---

### Recommended benchmark cases

These cases are used to spot performance regressions during refactors.

Medium (UCINET, 48 actors):

```bash
./build/socnetv-cli \
  -i src/data/Freeman_EIES_network_48actors_Acquaintanceship_at_time_1.dl \
  -f 5 \
  -c 1 -w 1 -x 1 -k 0 \
  --bench 10
```


| Class  | Dataset | N   | Approx cost |
| ------ | ------- | --- | ----------- |
| Small  | Dunbar  | 12  | ~4 ms       |
| Medium | EIES 48 | 48  | ~105 ms     |
| Large  | BA 500  | 500 | ~420 ms     |

That’s a **proper performance ladder**.


---

### Large Scale-Free Benchmark (BA model)

Generated with SocNetV 3.2:

* Model: Barabási–Albert
* n = 500
* m = 3
* Directed
* No loops
* Exported as Pajek

File:

```
src/data/Benchmark_BA_Directed_N500_m3.paj
```

Centralities ON:

```bash
./build/socnetv-cli \
  -i src/data/Benchmark_BA_Directed_N500_m3.paj \
  -f 2 \
  -c 1 -w 0 -x 1 -k 0 \
  --bench 20
```

Distances only:

```bash
./build/socnetv-cli \
  -i src/data/Benchmark_BA_Directed_N500_m3.paj \
  -f 2 \
  -c 0 -w 0 -x 1 -k 0 \
  --bench 20
```

These benchmarks are used to detect performance regressions in:

* SSSP (BFS/Dijkstra)
* Brandes dependency accumulation
* Centrality aggregation




---



## Baselines

Baselines live in:

```
src/tools/baselines/
```

These are committed to the repository and represent
“known good” outputs from the refactor target.

See [BASELINES__README.md](./baselines/BASELINES__README.md)
