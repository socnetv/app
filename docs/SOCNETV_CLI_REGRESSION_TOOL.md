# SocNetV CLI Regression Tool

`socnetv-cli` is a headless regression tool for verifying algorithmic correctness during development and refactoring of SocNetV.

It provides deterministic execution of compute kernels without loading any UI components, and is the backbone of the WS6 regression harness.

See also: [`ARCHITECTURAL_REFACTORING_ROADMAP.md`](ARCHITECTURAL_REFACTORING_ROADMAP.md)

---

# Purpose

The CLI enables:

* Headless dataset loading (no MainWindow / GraphicsWidget)
* Deterministic kernel execution
* Golden-output JSON generation
* Strict regression comparison against committed baselines
* Performance benchmarking (`distance` and `prominence` kernels)
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
* `cli/kernels/kernel_connectivity_v7.cpp`
* `cli/kernels/kernel_matrix_v8.cpp`
* `cli/kernels/kernel_vertex_connectivity_v9.cpp`

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

It is compiled alongside the main application via CMake (`BUILD_CLI=ON`).

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
* Two of the nine importable formats don't export yet — GML and TWOMODE (`Graph::saveToFile()`
  falls through to its `default:` case for both; see `m_graphFileFormatExportSupported` in
  `graph.cpp`). Every other format (GraphML, Pajek, Adjacency, GraphViz DOT, UCINET DL, both Edge
  List variants) does export and is exercised by this kernel's full roundtrip.
  For the two unsupported formats, the kernel reports a stable "skipped export" outcome and still
  emits v5 JSON.

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

## Connectivity Kernel

* Kernel: `connectivity`
* JSON schema: `schema_version = 7`

Protects:

* weakly or strongly connected component count
* per-node component ID assignment (weak mode only)
* connected/disconnected determination

Connectivity semantics:

| Graph type | Method | Connected when |
|------------|--------|----------------|
| Undirected | standard BFS | 1 component |
| Directed, `--connectivity-type weak` (default) | BFS treating all arcs as undirected | 1 weak component |
| Directed, `--connectivity-type strong` | Tarjan's SCC algorithm, respecting arc direction | 1 strong component |

Weak connectivity answers "how many disconnected islands exist?", ignoring arc direction. Strong
connectivity answers "can every node reach every other node via directed paths?" — a strictly
finer partition (a directed path a→b→c is one weak component but three strong components, since
`c` cannot reach `a`). `--connectivity-type` is ignored on undirected graphs, where the two notions
coincide.

Output fields:

* `connectivity.component_count` — number of components
* `connectivity.connected` — true if component_count == 1
* `connectivity.type` — `"connected"` (undirected), `"weak"`, or `"strong"` (directed)
* `per_node[].component_id` — 1-based component assignment per vertex, **weak mode only** (strong
  mode reports a count, not per-vertex membership — `Graph::graphStronglyConnectedComponents()`
  doesn't track it, see the function's own doc comment in `graph_distance_facade.cpp`)

Characteristics:

* deterministic vertex ordering
* topology-only (no weights, no centralities)
* no UI involvement

Notes:

* `-c`, `-w`, `-x`, `-k` are not applicable
* `--bench` not supported
* `--connectivity-type weak|strong` (default `weak`); only meaningful on directed graphs
* Component IDs are 1-based and assigned in BFS discovery order

---

## Matrix Kernel

* Kernel: `matrix`
* JSON schema: `schema_version = 8`

Protects raw contents of every `Matrix`-producing `Graph` operation — direct coverage of
`Matrix::item()`/`setItem()` indexing, independent of whatever downstream result (centrality score,
distance value, clique count) happens to read that matrix. See WS6.7 in
`roadmap_ws6_testing_ci_regression.md` for the motivating gap and how the dump-mode split below was
decided.

Seven categories dumped:

* adjacency (`AM`)
* adjacency inverse (`invAM`) — plus `invertible` (bool)
* distances (`DM`)
* similarity (`SCM`) — measure selectable via `--similarity-measure
  simple_matching|jaccard|pearson` (default `simple_matching`, unchanged from before); the chosen
  measure is echoed in `matrices.similarity.metric`. Added for #279 (NaN from divide-by-zero on
  Jaccard/Simple-Matching's `ties==0` and Pearson's `N-2<=0`/`M-4<=0` degenerate sample) so each
  measure's guarded path has its own golden coverage — see the `TinyArc_Dir_N2_E1` baselines below.
* reachability (`XRM`)
* walks, fixed length (`XM`)
* total walks (`XSM`) — **skipped above N=50** (`kTotalWalksSkipThreshold`, `kernel_matrix_v8.cpp`);
  summing matrix powers up to N-1 measured ~9.2 minutes at N=500, so this category simply isn't
  emitted on larger fixtures rather than making every run pay that cost
* clique co-membership (`CLQM`) — no size gate, stays cheap (single-digit ms) even at N=500

Output fields:

* `matrices.adjacency`, `matrices.distances`, `matrices.reachability`, etc. — one object per
  category, `null`/omitted for `total_walks` above the size threshold
* `matrices.adjacency_inverse.invertible` — bool; `false` for a singular matrix (see #269)
* Each matrix object: `dump_mode` (`"full"` or `"summary"`), `rows`, `cols`, and either `data` (the
  full grid, small fixtures only) or `row_sums`/`col_sums`/`trace`/`sample_cells` (large fixtures)

Characteristics:

* deterministic vertex ordering
* dump mode is size-dependent, not a flag — decided internally per fixture

Notes:

* `-w`, `-x`, `-k` control whether weights/isolate-dropping factor into the underlying computations
  before matrices are dumped. `-c` must be `0` — matrix categories don't involve centralities, and
  the kernel rejects a truthy value outright (default is `1`, so pass `-c 0` explicitly).
* `--bench` not supported

---

## Vertex Connectivity Kernel

* Kernel: `vertex_connectivity`
* JSON schema: `schema_version = 9`

Protects local (Menger's theorem / vertex-split max-flow) and global (pairwise-minimum) vertex
connectivity — see #7 and `roadmap_ws11_algorithm_additions.md` for the algorithm design.

Two modes, via `--conn-mode`:

* `local` — kappa(s,t) between `--conn-source S` and `--conn-target T` (both required)
* `global` (default) — kappa(G), the whole network's vertex connectivity

Connectivity semantics — same weak/strong split as the Connectivity Kernel above, via
`--connectivity-type weak|strong` (default `weak`, ignored on undirected graphs):

| Mode | Directed, weak | Directed, strong |
|------|-----------------|-------------------|
| local | undirected adjacency (edges treated as bidirectional) | respects arc direction — kappa(s,t) can differ from kappa(t,s) |
| global | minimum over unordered non-adjacent pairs | minimum over ordered non-adjacent pairs |

Output fields:

* `mode` — `"local"` or `"global"`
* `connectivity_type` — `"weak"`, `"strong"`, or `"undirected"`
* `local.source`, `local.target`, `local.status`, `local.value` (mode `local` only) — `status` is
  one of `"ok"` (`value` holds kappa(s,t), 0 means unreachable — a normal, valid answer), `"adjacent"`
  (s,t are directly connected — no finite cut exists, Menger's theorem requires non-adjacency,
  `value` is omitted), or `"invalid"` (nonexistent or equal source/target, `value` omitted)
* `global.value` (mode `global` only) — kappa(G); 0 means the network is already disconnected

Characteristics:

* deterministic vertex ordering
* topology-only (no weights, no centralities)
* no UI involvement
* global mode is O(n²) local-connectivity computations in the worst case — fine for the small/toy
  baseline fixtures used here, not something to run against large benchmark datasets

Notes:

* `-c`, `-w`, `-x`, `-k` are not applicable
* `--bench` not supported
* `--conn-source`/`--conn-target` are required for `--conn-mode local`, ignored for `global`
* The `status`/`value` split (rather than a single int with a sentinel like `-1`) is deliberate —
  see #271, a real bug this session caused by exactly that pattern (a sentinel silently misused as
  a bool/count)

---

# Basic Usage

## Available Parameters

`socnetv-cli` is intentionally small: a **single façade** parses a shared set of options, then dispatches into a selected `--kernel` implementation.

### Global flags

#### `-b` / `--verbose`

Enables `qDebug()`/`qCDebug()` output. Without it, only warnings/criticals reach stderr — the
default for clean regression-script output.

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

Input file type **ID** (must match SocNetV's internal file-type enum).

Common file types (from `global.h`):

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

#### `-d <str>` / `--delim <str>`

Field delimiter, passed straight through to `Parser::load()`. Relevant for Adjacency and Edge List
formats; default is a single space.

#### `-m <0|1>` / `--two-mode <0|1>`

Marks the input as a two-mode (affiliation) network. Default `0`.

#### `-l <0|1>` / `--labels <0|1>`

Tells the Adjacency parser the input file has row/column labels. Default `0`.

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
* `connectivity` — schema v7
* `matrix` — schema v8
* `vertex_connectivity` — schema v9

Examples:

```bash
--kernel distance
--kernel reachability
--kernel walks_matrix
--kernel prominence
--kernel io_roundtrip
--kernel clustering
--kernel connectivity
--kernel matrix
--kernel vertex_connectivity
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

* `1` = treat weight as "strength" and use inverse weight as distance cost (common in SNA)
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

`socnetv-cli` runs in **one** of the following "modes":

1. normal run (prints metrics to stdout)
2. dump deterministic JSON (`--dump-json` / `-j`)
3. strict compare against a golden JSON baseline (`--compare-json` / `-p`)
4. benchmarking (`--bench`, `distance` and `prominence` kernels only)

#### `--dump-json <path>` / `-j <path>`

Writes the kernel's deterministic JSON output to `<path>`.

* Intended to generate new golden baselines.
* Output is schema-versioned and stable.

Constraints:

* Not allowed together with `--compare-json`
* Not allowed together with `--bench`

Example:

```bash
--dump-json src/tools/baselines/ErdosRenyi_N10_E10__C1_W0_IW1_DI0.json
```

#### `--compare-json <baseline.json>` / `-p <baseline.json>`

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

#### `--strict`

`io_roundtrip`-kernel-specific modifier for `--compare-json`: promotes a timing regression
(actual roundtrip time vs. the baseline's recorded time) from an advisory warning to a hard
failure. Has no effect on any other kernel.

---

### Benchmarking (`distance` and `prominence` kernels only)

#### `--bench <runs>`

Runs the compute step multiple times and prints timing stats:

* `COMPUTE_RUNS`
* `COMPUTE_MS_MIN`
* `COMPUTE_MS_MEDIAN`
* `COMPUTE_MS_MEAN`
* `COMPUTE_MS_MAX`

Constraints:

* Only valid for `--kernel distance` or `--kernel prominence`
* Cannot combine with `--dump-json`
* Cannot combine with `--compare-json`

Example:

```bash
--kernel distance -c 1 -w 1 -x 1 -k 0 --bench 20
--kernel prominence --bench 20
```

---

## Per-kernel constraints summary

To avoid accidentally producing meaningless baselines:

### `--kernel distance` (schema v1)

Allowed:

* `-c`, `-w`, `-x`, `-k`
* `--dump-json`, `--compare-json`, `--bench`

Notes:

* `--bench` is also supported here and on `--kernel prominence` — no other kernel.

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

* `-w`, `-x`, `-k` (all three are read and baked into the schema's `run` object)
* `--dump-json`, `--compare-json`

Not supported:

* `--bench`

### `--kernel prominence` (schema v4)

Allowed:

* `-w`, `-x`, `-k`
* `--dump-json`, `--compare-json`, `--bench`

Notes:

* Prominence kernel covers *many* indices; `-w/-x` materially changes several results.
* One of only two kernels (with `distance`) that support `--bench`.

### `--kernel io_roundtrip` (schema v5)

Allowed:

* `-w`, `-x`, `-k` (depending on loader semantics)
* `--dump-json`, `--compare-json`

Notes:

* Some formats will "skip export" deterministically; that is expected and baseline-stable.

### `--kernel clustering` (schema v6)

Allowed:

* `-w`, `-x`, `-k`
* `--dump-json`, `--compare-json`

Not applicable / required:

* `-c` not used
* `--bench` not supported

Notes:

* v6 verifies clustering coefficient outputs, triad census, and maximal clique counts by size.

### `--kernel connectivity` (schema v7)

Allowed:

* `--connectivity-type weak|strong`
* `--dump-json`, `--compare-json`

Not applicable:

* `-c`, `-w`, `-x`, `-k` — connectivity is topology-only; these flags have no effect and should be omitted
* `--bench` not supported

Notes:

* v7 identifies weakly (BFS, arcs treated as undirected) or strongly (Tarjan's SCC, respects arc
  direction) connected components, per `--connectivity-type` (default `weak`).
* `--connectivity-type` is ignored on undirected graphs, where the two notions coincide.
* Component IDs are 1-based and assigned in BFS discovery order — **weak mode only**; strong mode
  reports a count, not per-vertex membership.

### `--kernel matrix` (schema v8)

Allowed:

* `-w`, `-x`, `-k`
* `--similarity-measure simple_matching|jaccard|pearson` (default `simple_matching`)
* `--similarity-input adjacency|distances` (default `adjacency`)
* `--dump-json`, `--compare-json`

Not applicable / required:

* `-c` must be `0` — `kernel_matrix_v8.cpp` rejects a truthy `-c` outright (`"--centralities is
  not applicable to --kernel matrix"`). Since `-c`'s CLI-wide default is `1`, **always pass
  `-c 0` explicitly** or the run fails immediately.
* `--bench` not supported

Notes:

* v8 dumps raw contents of seven `Matrix`-producing operations (adjacency, inverse, distances,
  similarity, reachability, walks, total walks) plus clique co-membership.
* Dump mode (full grid vs. row/col-sum summary) is chosen internally based on fixture size, not a
  flag.
* `total_walks` is omitted above N=50 (`kTotalWalksSkipThreshold`) — see the kernel section above.
* `--similarity-measure` selects which measure the `similarity` category runs
  (`createMatrixSimilarityMatching()` for `simple_matching`/`jaccard`,
  `createMatrixSimilarityPearson()` for `pearson`); an invalid value is rejected before the graph
  even loads.
* `--similarity-input` selects which `Matrix`-producing operation feeds the `similarity`
  category: the adjacency matrix (`AM`, default — never contains `RAND_MAX`) or the geodesic
  distances matrix (`DM`, computed unconditionally just before this category runs — contains
  `RAND_MAX` for unreachable pairs on a disconnected network). Needed to exercise a fix where
  `similarityMatrix()`'s Jaccard branch didn't exclude `RAND_MAX` from its match/ties count the
  way `distancesMatrix()` does — invisible when similarity runs on `AM`, only reachable via
  `distances`. The chosen input is echoed in `matrices.similarity.input`.

### `--kernel vertex_connectivity` (schema v9)

Allowed:

* `--conn-mode local|global`
* `--conn-source S --conn-target T` (required for `--conn-mode local`)
* `--connectivity-type weak|strong`
* `--dump-json`, `--compare-json`

Not applicable:

* `-c`, `-w`, `-x`, `-k` — topology-only; these flags have no effect and should be omitted
* `--bench` not supported

Notes:

* v9 computes local kappa(s,t) (`local` mode) or global kappa(G) (`global` mode, the default) via
  Menger's theorem / vertex-split max-flow.
* `--connectivity-type` is ignored on undirected graphs, where the two notions coincide.
* Local mode's `status` field distinguishes `"ok"` (a real value, including 0 for unreachable),
  `"adjacent"` (no finite cut exists — not a numeric answer), and `"invalid"` (bad source/target).

---

## Baseline naming convention (recommended)

When you dump JSON, bake the run flags into the filename (as already used in this repo):

* Distance v1: `__C{0|1}_W{0|1}_IW{0|1}_DI{0|1}`
* Prominence v4: `__W{0|1}_IW{0|1}_DI{0|1}`
* Reachability v2 / Walks v3 / IO v5: include kernel + schema label and any required parameters (e.g. `__WALKS_K6__V3`, `__FT2__...`, etc.)
* Clustering v6: `__CLUST__V6__FT{n}__W{0|1}_IW{0|1}_DI{0|1}`
* Connectivity v7: `__CONN__V7__FT{n}` (no flag suffixes — topology-only; add `__STRONG` for `--connectivity-type strong`)
* Matrix v8: `__MATRIX__V8__FT{n}__W{0|1}_IW{0|1}_DI{0|1}`, plus a trailing `__{measure}` suffix
  (e.g. `__jaccard`, `__pearson`) whenever `--similarity-measure` is not the default
  `simple_matching` — see the `TinyArc_Dir_N2_E1` baselines added for #279
* Vertex Connectivity v9: `__VCONN__V9__FT{n}` (no flag suffixes — topology-only; suffix with mode/pair, e.g. `__global` or `__local_1_3`)

This keeps baselines self-describing and prevents "wrong flags, right file" mistakes.

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

```
src/tools/baselines/clustering/
```

---

## Connectivity (schema v7)

```bash
./socnetv-cli \
  --kernel connectivity \
  -i src/data/TinyDisconnected_Undir_N6_E4.paj \
  -f 2 \
  --dump-json src/tools/baselines/connectivity/TinyDisconnected_Undir_N6_E4__CONN__V7__FT2.json
```

Strong mode, on a directed graph:

```bash
./socnetv-cli \
  --kernel connectivity \
  -i src/data/TinyArc_Dir_N2_E1.paj \
  -f 2 --connectivity-type strong \
  --dump-json src/tools/baselines/connectivity/TinyArc_Dir_N2_E1__CONN__V7__FT2__STRONG.json
```

Flag encoding:

```
CONN    = connectivity kernel
V7      = schema version 7
FT2     = file type = 2 (Pajek)
STRONG  = --connectivity-type strong (suffix omitted for weak, the default)
```

No weight or centrality flags — connectivity is topology-only.

Baseline directory:

```
src/tools/baselines/connectivity/
```

---

## Matrix (schema v8)

```bash
./socnetv-cli \
  --kernel matrix \
  -i src/data/TinyPath_N3_E2.paj \
  -f 2 -c 0 \
  --dump-json src/tools/baselines/matrix/TinyPath_N3_E2__MATRIX__V8__FT2__W0_IW1_DI0.json
```

Flag encoding:

```
MATRIX = matrix kernel
V8     = schema version 8
FT2    = file type = 2 (Pajek)
W0     = considerWeights=0
IW1    = inverseWeights=1
DI0    = dropIsolates=0
```

`--similarity-measure` example (Fix #279 — the degenerate N=2 case that used to produce NaN):

```bash
./socnetv-cli \
  --kernel matrix \
  -i src/data/TinyArc_Dir_N2_E1.paj \
  -f 2 -c 0 \
  --similarity-measure jaccard \
  --dump-json src/tools/baselines/matrix/TinyArc_Dir_N2_E1__MATRIX__V8__FT2__W0_IW1_DI0__jaccard.json
```

Baseline directory:

```
src/tools/baselines/matrix/
```

---

## Vertex Connectivity (schema v9)

```bash
./socnetv-cli \
  --kernel vertex_connectivity \
  -i src/data/TinyPath_N3_E2.paj \
  -f 2 --conn-mode local --conn-source 1 --conn-target 3 \
  --dump-json src/tools/baselines/vertex_connectivity/TinyPath_N3_E2__VCONN__V9__FT2__local_1_3.json
```

Global mode:

```bash
./socnetv-cli \
  --kernel vertex_connectivity \
  -i src/data/TinyPath_N3_E2.paj \
  -f 2 --conn-mode global \
  --dump-json src/tools/baselines/vertex_connectivity/TinyPath_N3_E2__VCONN__V9__FT2__global.json
```

Flag encoding:

```
VCONN  = vertex_connectivity kernel
V9     = schema version 9
FT2    = file type = 2 (Pajek)
```

No weight or centrality flags — topology-only. Global-mode baselines are deliberately Tiny*/toy
datasets only (see the kernel section above for why).

Baseline directory:

```
src/tools/baselines/vertex_connectivity/
```

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

## Connectivity Kernel (v7)

Graph-level:

* component_count — number of weakly or strongly connected components, per `--connectivity-type`
* connected — true if component_count == 1
* type — `"connected"` (undirected), `"weak"`, or `"strong"` (directed)

Per-node:

* component_id — 1-based integer, BFS discovery order. **Weak mode only** — strong mode doesn't
  track per-vertex SCC membership, only the count.

Semantics:

* For undirected graphs: standard BFS component labeling; `--connectivity-type` has no effect.
* Weak mode (default): BFS traverses both out-edges and in-edges simultaneously (treats arcs as
  undirected). Answers "how many disconnected islands exist?" independent of arc direction.
* Strong mode (`--connectivity-type strong`): Tarjan's SCC algorithm, respecting arc direction.
  Answers "can every node reach every other node via directed paths?" — a strictly finer partition
  than weak connectivity, computed by `Graph::graphStronglyConnectedComponents()`.

---

## Matrix Kernel (v8)

Graph-level:

* `run.considerWeights`, `run.inverseWeights`, `run.dropIsolates` — the flags the underlying
  computations ran with

Per-category (`matrices.*`), each with `dump_mode` (`"full"` or `"summary"`), `rows`, `cols`:

* `adjacency` — raw `AM`
* `adjacency_inverse` — raw `invAM`, plus `invertible` (bool; false for a singular matrix, #269)
* `distances` — raw `DM`
* `similarity` — raw `SCM` (or `PCC` for `--similarity-measure pearson`); measure selectable via
  `--similarity-measure simple_matching|jaccard|pearson` (default `simple_matching`), echoed in
  `matrices.similarity.metric`
* `reachability` — raw `XRM`
* `walks` — raw `XM` (fixed length)
* `total_walks` — raw `XSM`; omitted above N=50 (`kTotalWalksSkipThreshold`)
* `clique_comembership` — raw `CLQM`

`dump_mode` semantics:

* `"full"` — small fixtures dump the complete N×N grid (`data`)
* `"summary"` — large/sparse fixtures dump `row_sums`, `col_sums`, `trace`, and `sample_cells`
  (corners + center) instead of the full grid

---

## Vertex Connectivity Kernel (v9)

Graph-level:

* `mode` — `"local"` or `"global"`
* `connectivity_type` — `"weak"`, `"strong"`, or `"undirected"`

Local mode (`local.*`):

* `source`, `target` — the requested pair
* `status` — `"ok"` (`value` holds kappa(s,t)), `"adjacent"` (no finite cut exists, no `value`), or
  `"invalid"` (bad source/target, no `value`)
* `value` — present only when `status == "ok"`; 0 is a valid answer (target unreachable from source)

Global mode (`global.*`):

* `value` — kappa(G); 0 means the network is already disconnected

Semantics:

* Local: Menger's theorem via vertex-split max-flow (Edmonds-Karp). Weak mode treats edges as
  bidirectional; strong mode respects arc direction, so kappa(s,t) can differ from kappa(t,s).
* Global: minimum local connectivity over all non-adjacent pairs (unordered for weak/undirected,
  ordered for strong), pruned by a minimum-degree bound (Whitney's inequality).
* Complete graphs need no special case: with no non-adjacent pair to test, the degree bound itself
  is the answer (kappa(K_n) = n-1).

---

# Micro-Benchmarking Mode (Distance and Prominence Kernels Only)

The CLI provides benchmarking for `DistanceEngine`-based work, via `--kernel distance` or
`--kernel prominence`.

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
* Only supported with `--kernel distance` or `--kernel prominence`

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
* IO Roundtrip (v5)
* Clustering (v6)
* Connectivity (v7)
* Matrix (v8)
* Vertex Connectivity (v9)

Fails on any mismatch.

---

## Performance Benchmarks

```
scripts/run_benchmarks.sh
```

Validates median compute times for the `distance` and `prominence` kernels.

Machine-aware baseline sets supported.

---

# Baselines

Distance baselines:

```
src/tools/baselines/distance/
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

Connectivity baselines:

```
src/tools/baselines/connectivity/
```

Matrix baselines:

```
src/tools/baselines/matrix/
```

Vertex Connectivity baselines:

```
src/tools/baselines/vertex_connectivity/
```

See: [`src/tools/baselines/BASELINES__README.md`](../src/tools/baselines/BASELINES__README.md)

---

# Regression Discipline

Rules:

* Never modify existing schema structures
* New kernel → new schema version
* Deterministic ordering always
* Explicit failure on mismatch
* Baselines are updated only for deliberate semantic fixes

The CLI is the architectural safety harness of SocNetV.
