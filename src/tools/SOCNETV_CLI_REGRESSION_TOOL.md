# SocNetV CLI Regression Tool

`socnetv-cli` is a headless regression tool used to verify algorithmic correctness during the ongoing architectural refactor of SocNetV.

It provides deterministic execution of compute kernels without loading any UI components.

Originally introduced to protect the refactor of:

```
Graph::graphDistancesGeodesic()
→ DistanceEngine
```

It has since evolved into a modular regression harness for multiple algorithm families.

---

# Purpose

The CLI enables:

* Headless dataset loading (no MainWindow / GraphicsWidget)
* Deterministic kernel execution
* Golden-output JSON generation
* Strict regression comparison against committed baselines
* Performance benchmarking (distance kernel only)
* CI integration (fail-fast on mismatch)

This ensures refactors preserve:

* numeric results
* connectivity bookkeeping
* centrality vectors
* prestige vectors
* directed/weighted semantics
* matrix-based computations (walks, reachability)

---

# Architecture

The CLI is modular.

* `socnetv_cli.cpp` → façade (argument parsing, dispatch)
* `cli/cli_common.cpp` → shared utilities
* `cli/kernels/kernel_distance_v1.cpp`
* `cli/kernels/kernel_reachability_v2.cpp`
* `cli/kernels/kernel_walks_v3.cpp`
* `cli/kernels/kernel_prominence_v4.cpp`

Each kernel owns:

* Its execution logic
* Its JSON schema builder
* Its comparison routine

The CLI is a safety harness, not a new analytics engine.

---

# Design Principles

* No UI involvement
* No graphics dependency
* Deterministic vertex ordering
* Deterministic float formatting (floats serialized as strings)
* Schema isolation per algorithm family
* Zero silent semantic modification of Graph

Each kernel has its own schema version.

Existing schemas are **never modified**.

---

# Build

The tool is built as:

```
socnetv-cli
```

It is compiled alongside the main application via CMake.

---

# Kernels and JSON Schemas

Kernels are selected with `--kernel`.

---

## Distance / Centrality Kernel

* Kernel: `distance` (default)
* JSON schema: `schema_version = 1`

Protects:

* DistanceEngine
* Geodesic-based centralities
* Connectivity semantics

---

## Reachability Kernel

* Kernel: `reachability`
* JSON schema: `schema_version = 2`

Reachability semantics:

* R(i,j) = 1 if geodesic distance is finite
* R(i,j) = 0 otherwise
* Diagonal convention: **R(i,i) = 1**

Derived from the distance kernel.

Constraints:

* `--centralities` not applicable
* Must use `-c 0`
* `--bench` not supported

---

## Walks Matrix Kernel

* Kernel: `walks_matrix`
* JSON schema: `schema_version = 3`
* Required option: `--walks-length K`

Computes:

```
XM = A^K
```

Each element:

```
XM(i,j) = number of walks of exact length K from i to j
```

Output includes:

* `walks.nodes`
* `walks.matrix`
* `walks.length`
* `walks.total_walks`

---

## Prominence Kernel

* Kernel: `prominence`
* JSON schema: `schema_version = 4`

Protects all node-level prominence indices.

### Centrality

* DC / SDC
* CC / SCC (classic closeness)
* IRCC / SIRCC (influence-range closeness)
* BC / SBC
* SC / SSC
* PC / SPC
* IC / SIC
* EVC / SEVC
* eccentricity (+ eccentricity_inf)

### Prestige

* DP / SDP (degree prestige)
* PP / SPP (proximity prestige)
* PRP / SPRP (PageRank)

Characteristics:

* Deterministic vertex ordering
* Deterministic float serialization
* Strict per-field comparison
* No UI involvement

This kernel combines:

* DistanceEngine-based indices
* Standalone centrality functions
* Prestige functions

---

# Basic Usage

Distance kernel:

```bash
./socnetv-cli \
  -i src/data/SmallWorld_N10_E12.graphml \
  -f 1
```

Common file types (see `global.h`):

* 1 → GRAPHML
* 2 → PAJEK (.paj / .net)
* 3 → ADJACENCY
* 5 → UCINET

---

# Golden Output Dump

## Distance (schema v1)

```bash
./socnetv-cli \
  -i src/data/SmallWorld_N10_E12.graphml \
  -f 1 \
  --dump-json src/tools/baselines/SmallWorld_N10_E12__C1_W0_IW1_DI0.json
```

Flag encoding:

```
C1  = computeCentralities=1
W0  = considerWeights=0
IW1 = inverseWeights=1
DI0 = dropIsolates=0
```

---

## Reachability (schema v2)

```bash
./socnetv-cli \
  --kernel reachability \
  -i src/data/Stokman_Ziegler_Corporate_Interlocks_Netherlands.dl \
  -f 5 -c 0 -w 1 -x 1 -k 0 \
  --dump-json src/tools/baselines/reachability/StokmanZiegler_Netherlands__REACH__V2.json
```

---

## Walks Matrix (schema v3)

```bash
./socnetv-cli \
  --kernel walks_matrix \
  --walks-length 6 \
  -i src/data/Stephenson_Zelen_Dunbar_Dunbar_Gelada_baboon_colony_H22a_IC.paj \
  -f 2 -c 0 -w 1 -x 1 -k 0 \
  --dump-json src/tools/baselines/walks/DunbarGelada_H22a__WALKS_K6__V3.json
```

---

## Prominence (schema v4)

```bash
./socnetv-cli \
  --kernel prominence \
  -i src/data/Krackhardt_Kite_N10.paj \
  -f 2 -w 0 -x 1 -k 0 \
  --dump-json src/tools/baselines/prominence/Krackhardt_Kite_N10__PROM__V4__FT2__W0_IW1_DI0.json
```

Flag encoding:

```
W0  = considerWeights=0
IW1 = inverseWeights=1
DI0 = dropIsolates=0
```

---

# Golden Output Compare

```bash
./socnetv-cli \
  --kernel <kernel> \
  --compare-json <baseline.json>
```

On mismatch:

* Exact field differences printed
* Non-zero exit code returned

Schemas are strictly compared per version.

---

# What Is Verified

## Distance Kernel (v1)

Graph-level:

* nodes
* links_sna
* ties_graph
* directed / weighted
* avg_distance
* diameter
* disconnected_pairs
* connected

Per-node:

* CC / SCC
* BC / SBC
* SC / SSC
* EC / SEC
* PC / SPC
* distance_sum
* eccentricity

---

## Reachability Kernel (v2)

* nodes
* matrix (0/1)
* reachable_pairs
* reachable_density

---

## Walks Kernel (v3)

* nodes
* matrix (integer counts)
* walks.length
* walks.total_walks

---

## Prominence Kernel (v4)

Graph-level:

* nodes
* links_sna
* ties_graph
* directed / weighted

Per-node:

Centrality:

* DC / SDC
* CC / SCC
* IRCC / SIRCC
* BC / SBC
* SC / SSC
* PC / SPC
* IC / SIC
* EVC / SEVC
* eccentricity (+ eccentricity_inf)

Prestige:

* DP / SDP
* PP / SPP
* PRP / SPRP

Floating-point values are serialized as strings.

---

# Micro-Benchmarking Mode (Distance Kernel Only)

The CLI provides benchmarking for DistanceEngine.

```bash
./socnetv-cli \
  -i dataset \
  -f type \
  -c 1 -w 1 -x 1 -k 0 \
  --bench 20
```

Outputs:

```
COMPUTE_RUNS
COMPUTE_MS_MIN
COMPUTE_MS_MEDIAN
COMPUTE_MS_MEAN
COMPUTE_MS_MAX
```

Constraints:

* Cannot combine with `--dump-json`
* Cannot combine with `--compare-json`
* Only supported with `--kernel distance`

---

# Automated Regression Scripts

## Golden Comparisons

```
scripts/run_golden_compares.sh
```

Validates:

* Distance (v1)
* Reachability (v2)
* Walks (v3)
* Prominence (v4)

Fails on any mismatch.

---

## Performance Benchmarks

```
scripts/run_benchmarks.sh
```

Validates median compute times for the distance kernel only.

Machine-aware baseline sets supported.

---

# Baselines

Distance baselines:

```
src/tools/baselines/
```

Reachability baselines:

```
src/tools/baselines/reachability/
```

Walks baselines:

```
src/tools/baselines/walks/
```

Prominence baselines:

```
src/tools/baselines/prominence/
```

See:

```
src/tools/baselines/BASELINES__README.md
```

---

# Regression Discipline

Rules:

* Never modify existing schema structures
* New kernel → new schema version
* Deterministic ordering always
* Explicit failure on mismatch
* Baselines are updated only for deliberate semantic fixes

The CLI is the architectural safety harness of SocNetV.

