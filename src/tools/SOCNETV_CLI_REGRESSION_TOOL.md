# SocNetV CLI Regression Tool

`socnetv-cli` is a headless regression tool used to verify algorithmic correctness during the ongoing architectural refactor of SocNetV.

It provides deterministic execution of compute kernels without loading any UI components.

Originally introduced to protect the code refactor we did during WorkStreams WS1 and WS2 [WorkStreams WS1 and WS2](../../docs/ARCHITECTURAL_REFACTORING_ROADMAP.md). It has since evolved into a modular regression harness for multiple algorithm families.

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
* `cli/kernels/kernel_io_roundtrip_v5.cpp`
* `cli/kernels/kernel_clustering_v6.cpp`


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

Kernels are selected with `--kernel`. See more in 'Basic usage' further below.

---

## Distance / Centrality Kernel

* Kernel: `distance` (default)
* JSON schema: `schema_version = 1`

Protects:

* DistanceEngine
* Geodesic-based centralities
* Connectivity semantics

Defaults are:

```
-c 1
-w 0
-x 1
-k 0
```

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

## IO Roundtrip Kernel

* Kernel: `io_roundtrip`
* JSON schema: `schema_version = 5`

Purpose:
* Protects IO + parser correctness during refactors by enforcing a strict **load → (optional export) → reload** contract.
* Verifies canonical graph signatures rather than trusting parser counters.
* Supports **multi-relational** graphs via a per-relation bundle comparison.

Workflow:
1) Load input dataset (any supported format).
2) Attempt to export in the same format (only when export is supported for that file type).
3) Reload exported file.
4) Compare canonical signatures between original load and roundtrip reload:
   * Single relation: compares the single signature.
   * Multi-relational: compares a per-relation signature bundle (relation names + per-relation counts/signatures).

Export support:
* Some file formats do not export yet (e.g. GRAPHVIZ, GML, EDGELIST_*).  
  For those formats, the kernel reports a stable “skipped export” outcome and still emits v5 JSON.

Key output fields (v5):
* `KERNEL_DESC` — describes the kernel contract
* `RELATIONS` — number of relations in the loaded graph
* `SYMMETRIC` — 0/1 (important because some formats default to DIRECTED but are symmetric)
* `ROUNDTRIP_EQUIV` — 0/1 if roundtrip was performed
* `ROUNDTRIP_SKIPPED` — string reason when export is not supported (performed=false)

Comparison:
* `compareGoldenV5Io()` enforces stable behavior for:
  * performed vs skipped export
  * per-relation bundle structure and signatures
  * canonical counts derived from the Graph (ties_graph) and derived links_sna
  * relation-name normalization (no double quoting, etc.)

---

## Clustering Kernel

* Kernel: `clustering`
* JSON schema: `schema_version = 6`

Protects:

* local clustering coefficient (CLC) per node
* network average clustering coefficient
* triad census (16 MAN classes)
* maximal clique counts by size

Characteristics:

* deterministic vertex ordering
* deterministic float serialization
* strict per-field comparison
* no UI involvement

This kernel combines:

* `Graph::clusteringCoefficient(false)`
* `Graph::graphTriadCensus()`
* `Graph::graphCliques(QSet<int>(), QSet<int>(), QSet<int>())`

Notes:

* `-c` is not applicable
* `--bench` not supported
* `-w/-x` are still encoded in baselines for consistency, even if the
  current clustering/clique routines are not weight-driven in the same
  way as distance-based kernels

---

# Basic Usage

## Available Parameters

`socnetv-cli` is intentionally small: a **single façade** parses a shared set of options, then dispatches into a selected `--kernel` implementation.

### Input selection

#### `-i <path>` / `--input <path>`

Path to the dataset file to load.

* Required for all kernels unless a kernel explicitly supports synthetic generation (currently: **no**).
* Relative paths are allowed.

Examples:

```bash
-i src/data/SmallWorld_N10_E12.graphml
-i ./mygraph.paj
```

#### `-f <id>` / `--format <id>`

Input file type **ID** (must match SocNetV’s internal file-type enum).

Common file types (from `global.h` as referenced in this doc):

* `1` → GRAPHML
* `2` → PAJEK (.paj / .net)
* `3` → ADJACENCY
* `4` → GRAPHVIZ (DOT)
* `5` → UCINET (DL)
* `6` → GML
* `7` → EDGELIST_WEIGHTED
* `8` → EDGELIST_SIMPLE
* `9` → TWOMODE (**not supported by CLI kernels; do not baseline**)

Notes:

* The CLI is strict: if you pass a mismatched `-f` for the actual file contents, parsing may fail or semantics may differ.
* For IO regression work, treat `-f` as part of the baseline identity.

---

### Kernel selection

#### `--kernel <name>`

Selects which analysis kernel to run.

Supported kernels:

* `distance` (default) — schema v1
* `reachability` — schema v2
* `walks_matrix` — schema v3
* `prominence` — schema v4
* `io_roundtrip` — schema v5
* `clustering` — schema v6

Examples:

```bash
--kernel distance
--kernel reachability
--kernel walks_matrix
--kernel prominence
--kernel io_roundtrip
--kernel clustering
```

If omitted:

* `--kernel distance` is assumed.


**Basic Example (Distance kernel)**

```bash
./socnetv-cli \
  -i src/data/SmallWorld_N10_E12.graphml \
  -f 1
```
---

### Run flags (shared semantics)

These flags control *how the graph is interpreted* and/or *what extra results are computed*.

#### `-c <0|1>` / `--centralities <0|1>`

Controls whether the **distance kernel** computes geodesic-based centralities.

* `1` = compute centralities (default for distance kernel baselines: usually **1**)
* `0` = distances-only run (faster; smaller output)

Notes:

* Only meaningful for `--kernel distance`.
* For other kernels, see per-kernel constraints below.

#### `-w <0|1>` / `--weights <0|1>`

Controls whether edge weights are considered (when weights exist).

* `1` = consider weights
* `0` = ignore weights (treat as unweighted)

Notes:

* If the dataset format has no weights (or all weights are default), `-w 1` may behave the same as `-w 0`, but **still keep it explicit in baselines**.

#### `-x <0|1>` / `--inverse-weights <0|1>`

Controls how weights are interpreted when `-w 1` is enabled.

* `1` = treat weight as “strength” and use inverse weight as distance cost (common in SNA)
* `0` = treat weight directly as distance cost

Notes:

* If `-w 0`, this flag should not change results, but we still keep it explicit for stable baseline naming.

#### `-k <0|1>` / `--drop-isolates <0|1>`

Controls whether isolate vertices are removed before computation.

* `1` = drop isolates (removes isolated nodes)
* `0` = keep isolates

Notes:

* This affects N, connectivity bookkeeping, averages, and many per-node vectors. Always encode it in baseline names.

---

### Walks kernel specific

#### `--walks-length <K>`

Required only for `--kernel walks_matrix`.

Meaning:

* Computes the walks matrix `A^K`, where:

  * `XM(i,j)` = number of walks of **exact** length `K` from i to j.

Constraints:

* Must be a positive integer.
* Required when `--kernel walks_matrix`.
* Ignored / invalid for other kernels.

Example:

```bash
--kernel walks_matrix --walks-length 6
```

---

### Output modes

`socnetv-cli` runs in **one** of the following “modes”:

1. normal run (prints metrics to stdout)
2. dump deterministic JSON (`--dump-json`)
3. strict compare against a golden JSON baseline (`--compare-json`)
4. benchmarking (`--bench`, distance kernel only)

#### `--dump-json <path>`

Writes the kernel’s deterministic JSON output to `<path>`.

* Intended to generate new golden baselines.
* Output is schema-versioned and stable.

Constraints:

* Not allowed together with `--compare-json`
* Not allowed together with `--bench`

Example:

```bash
--dump-json src/tools/baselines/ErdosRenyi_N10_E10__C1_W0_IW1_DI0.json
```

#### `--compare-json <baseline.json>`

Runs the selected kernel and strictly compares output to an existing JSON baseline.

Behavior:

* Prints per-field diffs on mismatch
* Exits non-zero on mismatch (CI-safe)

Constraints:

* Not allowed together with `--dump-json`
* Not allowed together with `--bench`

Example:

```bash
--compare-json src/tools/baselines/prominence/Krackhardt_Kite_N10__PROM__V4__FT2__W0_IW1_DI0.json
```

---

### Benchmarking (distance kernel only)

#### `--bench <runs>`

Runs the compute step multiple times and prints timing stats:

* `COMPUTE_RUNS`
* `COMPUTE_MS_MIN`
* `COMPUTE_MS_MEDIAN`
* `COMPUTE_MS_MEAN`
* `COMPUTE_MS_MAX`

Constraints:

* Only valid for `--kernel distance`
* Cannot combine with `--dump-json`
* Cannot combine with `--compare-json`

Example:

```bash
--kernel distance -c 1 -w 1 -x 1 -k 0 --bench 20
```

---

## Per-kernel constraints summary

To avoid accidentally producing meaningless baselines:

### `--kernel distance` (schema v1)

Allowed:

* `-c`, `-w`, `-x`, `-k`
* `--dump-json`, `--compare-json`, `--bench`

Notes:

* `--bench` only here.

### `--kernel reachability` (schema v2)

Allowed:

* `-w`, `-x`, `-k` (affects the underlying distance semantics)
* `--dump-json`, `--compare-json`

Not applicable / required:

* `-c` must be `0` (centralities not used here)
* `--bench` not supported

### `--kernel walks_matrix` (schema v3)

Required:

* `--walks-length K`

Allowed:

* `-w` (if the implementation supports weighted adjacency behavior; otherwise treat as unweighted by design)
* `--dump-json`, `--compare-json`

Not supported:

* `--bench`

### `--kernel prominence` (schema v4)

Allowed:

* `-w`, `-x`, `-k`
* `--dump-json`, `--compare-json`

Notes:

* Prominence kernel covers *many* indices; `-w/-x` materially changes several results.

### `--kernel io_roundtrip` (schema v5)

Allowed:

* `-w`, `-x`, `-k` (depending on loader semantics)
* `--dump-json`, `--compare-json`

Notes:

* Some formats will “skip export” deterministically; that is expected and baseline-stable.

---

### `--kernel clustering` (schema v6)

Allowed:

* `-w`, `-x`, `-k`
* `--dump-json`, `--compare-json`

Not applicable / required:

* `-c` not used
* `--bench` not supported

Notes:

* v6 verifies clustering coefficient outputs, triad census, and maximal clique counts by size.


## Baseline naming convention (recommended)

When you dump JSON, bake the run flags into the filename (as already used in this repo):

* Distance v1: `__C{0|1}_W{0|1}_IW{0|1}_DI{0|1}`
* Prominence v4: `__W{0|1}_IW{0|1}_DI{0|1}`
* Reachability v2 / Walks v3 / IO v5: include kernel + schema label and any required parameters (e.g. `__WALKS_K6__V3`, `__FT2__...`, etc.)

This keeps baselines self-describing and prevents “wrong flags, right file” mistakes.

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
FT2 = file type =2 
IW1 = inverseWeights=1
DI0 = dropIsolates=0
```

---

## IO Roundtrip (schema v5)

```bash
./socnetv-cli \
  --kernel io_roundtrip \
  -i src/data/TinyGraphML_Weighted_Dir_N3.graphml \
  -f 1 \
  --dump-json src/tools/baselines/io_roundtrip/TinyGraphML_Weighted_Dir_N3__FT1.json
```

Notes:

* Formats without export support still produce a stable v5 JSON with:

  * performed=false
  * a `ROUNDTRIP_SKIPPED` reason string
  * canonical load signatures

Baseline directory:

* `src/tools/baselines/io_roundtrip/`

---

## Clustering (schema v6)

```bash
./socnetv-cli \
  --kernel clustering \
  -i src/data/Krackhardt_Kite_N10.paj \
  -f 2 -w 0 -x 1 -k 0 \
  --dump-json src/tools/baselines/clustering/Krackhardt_Kite_N10__CLUST__V6__FT2__W0_IW1_DI0.json
```

Flag encoding:

```
W0  = considerWeights=0
FT2 = file type =2
IW1 = inverseWeights=1
DI0 = dropIsolates=0
```

Baseline directory:

src/tools/baselines/clustering/



---

# Golden Output Compare

```bash
./socnetv-cli \
  -i src/data/data_file.graphml \
  -f 1 \
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

## IO Roundtrip Kernel (v5)

Graph-level:
* nodes
* relations
* directed / symmetric / weighted
* ties_graph (canonical, from Graph adjacency)
* links_sna (derived)

Roundtrip-level:
* performed vs skipped export behavior (must remain stable)
* `ROUNDTRIP_EQUIV` and mismatch hints when performed
* per-relation signature bundle comparison for multi-relational datasets
  (expected relations + signatures from original load vs what reload produced)

---

## Clustering Kernel (v6)

Graph-level:

* nodes
* links_sna
* ties_graph
* directed / weighted

Metrics:

* averageCLC
* nodesWithCLC

Per-node:

* CLC
* hasCLC

Triad census:

* 003
* 012
* 102
* 021D
* 021U
* 021C
* 111D
* 111U
* 030T
* 030C
* 201
* 120D
* 120U
* 120C
* 210
* 300

Cliques:

* maximal clique counts by size
* max_clique_size
* total_cliques


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
* Clustering (v6)

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

IO baselines:

```
src/tools/baselines/io_roundtrip/
```

Clustering baselines:

```
src/tools/baselines/clustering/
```


See:

[BASELINES__README.md](./baselines/BASELINES__README.md)

```

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

