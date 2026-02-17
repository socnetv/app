# SocNetV CLI Regression Tool

`socnetv-cli` is a headless regression tool used to verify algorithmic correctness
during the ongoing architectural refactor of SocNetV.

It provides deterministic execution of compute kernels without loading any UI components.
It was originally designed to protect the refactor of:

    Graph::graphDistancesGeodesic()
    → DistanceEngine

---

## Purpose

The CLI enables:

- Headless dataset loading (no MainWindow / GraphicsWidget)
- Deterministic kernel execution
- Golden-output JSON generation
- Strict regression comparison against committed baselines
- CI integration (fail-fast on mismatch)

This ensures refactors preserve:

- numeric results
- connectivity bookkeeping
- centrality vectors
- directed/weighted semantics

---

## Design Principles

- No UI involvement
- No graphics dependency
- Deterministic ordering
- Deterministic float formatting
- Zero semantic modification of Graph

The CLI is a safety harness, not a new analytics engine.

---

## Build

The tool is built as:

    socnetv-cli

It is compiled alongside the main application via CMake.

---

## Kernels and JSON Schemas

The CLI supports multiple kernels selected by `--kernel`.

### 1) Distance / Centrality Kernel (default)

- Kernel: `distance` (default)
- JSON schema: `schema_version = 1`

This kernel runs geodesic distances and (optionally) centralities, then emits:
- text KV output (always)
- JSON output (optional dump/compare)

### 2) Reachability Kernel

- Kernel: `reachability`
- JSON schema: `schema_version = 2`

This kernel computes the full NxN reachability matrix derived from geodesic distances:

- R(i,j) = 1 if a walk exists from i to j
- R(i,j) = 0 otherwise
- Diagonal convention: **R(i,i) = 1**

A pair is reachable iff its geodesic distance is finite.

---

## Basic Usage

Load a dataset and print summary metrics (distance kernel):

```bash
./socnetv-cli \
  -i src/data/SmallWorld_N10_E12.graphml \
  -f 1
````

Common file types (see `global.h`):

* 1 → GRAPHML
* 2 → PAJEK
* 3 → ADJACENCY
* 5 → UCINET
* etc.

---

## Golden Output Dump

### Distance kernel (schema v1)

Generate a deterministic JSON baseline:

```bash
./socnetv-cli \
  -i src/data/SmallWorld_N10_E12.graphml \
  -f 1 \
  --dump-json src/tools/baselines/SmallWorld_N10_E12__C1_W0_IW1_DI0.json
```

Filename convention encodes run flags:

```
C1  = computeCentralities=1
W0  = considerWeights=0
IW1 = inverseWeights=1
DI0 = dropIsolates=0
```

### Reachability kernel (schema v2)

Generate a deterministic reachability baseline:

```bash
./socnetv-cli \
  --kernel reachability \
  -i src/data/Stokman_Ziegler_Corporate_Interlocks_Netherlands.dl \
  -f 5 -w 1 -x 1 -k 0 \
  --dump-json src/tools/baselines/reachability/StokmanZiegler_Netherlands__REACH__V2.json
```

---

## Golden Output Compare

Strict comparison against a baseline:

### Distance kernel (schema v1)

```bash
./socnetv-cli \
  -i src/data/SmallWorld_N10_E12.graphml \
  -f 1 \
  --compare-json src/tools/baselines/SmallWorld_N10_E12__C1_W0_IW1_DI0.json
```

### Reachability kernel (schema v2)

```bash
./socnetv-cli \
  --kernel reachability \
  -i src/data/Stokman_Ziegler_Corporate_Interlocks_Netherlands.dl \
  -f 5 -w 1 -x 1 -k 0 \
  --compare-json src/tools/baselines/reachability/StokmanZiegler_Netherlands__REACH__V2.json
```

On mismatch, the tool prints exact field differences and exits non-zero.

---

## What Is Verified

### Distance kernel (schema v1)

Graph-level metrics:

* nodes
* links_sna (loader semantics)
* ties_graph (canonical Graph model)
* directed / weighted
* average geodesic distance
* diameter
* disconnected_pairs
* connected

Per-node vectors (when centralities enabled), for each vertex (deterministic ascending id order):

* CC / SCC
* BC / SBC
* SC / SSC
* EC / SEC
* PC / SPC
* distance_sum
* eccentricity

Floating-point values are serialized as strings to avoid JSON double-format inconsistencies.

### Reachability kernel (schema v2)

* `reachability.nodes` → deterministic ascending vertex ids (order)
* `reachability.matrix` → NxN array of 0/1 integers
* `reachability.reachable_pairs` → number of reachable pairs (including diagonal)

Schema v2 is separate from schema v1 to avoid modifying existing baselines.

---

## Micro-Benchmarking Mode (Distance Kernel Only)

The CLI provides a lightweight benchmarking mode to measure DistanceEngine compute performance.

This mode:

* does **not** modify algorithmic behavior
* does **not** generate or compare JSON output

### Usage

```bash
./socnetv-cli \
  -i src/data/Stephenson_Zelen_Dunbar_Dunbar_Gelada_baboon_colony_H22a_IC.paj \
  -f 2 \
  -c 1 -w 1 -x 1 -k 0 \
  --bench 10
```

### Output

Example:

```
COMPUTE_RUNS=10
COMPUTE_MS_MIN=4
COMPUTE_MS_MEDIAN=4
COMPUTE_MS_MEAN=4.200
COMPUTE_MS_MAX=5
```

### Constraints

* `--bench` cannot be combined with:

  * `--dump-json`
  * `--compare-json`

* Bench mode is supported **only** with:

  * `--kernel distance` (default)

* Dataset loading time is reported separately via `LOAD_MS`.

---

## Automated Regression Scripts

### Golden comparisons

```
scripts/run_golden_compares.sh
```

Runs the CLI against committed baselines and fails on mismatch.

### Performance benchmarks (DistanceEngine)

```
scripts/run_benchmarks.sh
```

Runs a benchmark ladder (Small / Medium / Large), extracts `COMPUTE_MS_MEDIAN`,
and compares against expected medians (per-host baselines supported).

---

## Baselines

Distance kernel baselines:

```
src/tools/baselines/
```

Reachability baselines:

```
src/tools/baselines/reachability/
```

See:

* `src/tools/baselines/BASELINES__README.md`

