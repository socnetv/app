# Matrices Modernization Roadmap (WS5)

## Goal

- Make the matrix subsystem genuinely faster — contiguous storage, flat-array APSP results.
- Isolate matrix creation and computation into coherent, testable types.
- Cancellation-aware algebra kernels.

## Status

🚧 In progress. A1 (inventory) and A2.0 (empirical validation, GO decision) done. A2 (the APSP
storage migration) underway — step 1 of 3 landed. A3–A7 not started.

## Current Reality

- Matrix-related logic is scattered: code that constructs/populates `Graph`'s 11 named `Matrix`
  fields (`SIGMA, DM, sumM, invAM, AM, invM, WM, XM, XSM, XRM, CLQM`) lives across six different
  `src/graph/` slice directories, not one (`centrality/`, `distances/`, `cohesion/`, `matrices/`,
  `reachability/`, `reporting/`). Only one of the six is the `matrices/` directory that nominally
  owns this concern.
- **`Matrix`'s own internal storage does N+1 separate heap allocations per matrix, not one.**
  Confirmed by reading the constructor directly (`src/matrix.cpp:33-39`):
  ```cpp
  Matrix::Matrix (int rowDim, int colDim) : m_rows(rowDim), m_cols(colDim) {
      row = new MatrixRow[m_rows];              // allocation #1: the row objects
      for (int i=0; i<m_rows; i++)
          row[i].resize(m_cols);                // allocation #2..N+1: one per row
  }
  ```
  Each `MatrixRow` (`src/matrix.h:51-110`) owns its own separately-`new`'d `qreal[]` buffer. For a
  7343-node network (`geom.net`), constructing a single N×N matrix means **7344 separate heap
  allocations** instead of 1. Rows aren't guaranteed contiguous in memory, so row-to-row traversal
  (the common `for(i) for(j) M[i][j]` access pattern used throughout the centrality/distance code)
  doesn't benefit from cache prefetching the way a single flat buffer would.
- Matrix algebra methods (inverse, power iteration, etc.) run synchronously on the main thread with
  no cancellation support (see Known Issues below).

## Known Issues

### Performance

**P1 — `Matrix` storage is N+1 heap allocations instead of 1 contiguous buffer.** See Current
Reality above. Re-verified against current code 2026-07-30 — `Matrix::Matrix()` (`src/matrix.cpp:33-39`)
and `MatrixRow`'s own `new qreal[]` (`src/matrix.h:51-60`) are both unchanged, so A3's premise still
holds as written.

**P2 — APSP per-vertex QHash storage** (`GraphVertex::m_distance` / `m_shortestPaths`) — the
incoming migration from WS3 M1, detailed as A2 below.

### Cancellation (found during #52 Cancel-button fix)

**I1 — Matrix algebra methods are not cancellation-aware.** `Matrix::inverse()`
(`src/matrix.cpp:1256`), `Matrix::inverseByGaussJordanElimination()` (`src/matrix.cpp:1003`), and
`Matrix::powerIteration()` (`src/matrix.cpp:808`) run to completion regardless of user cancel.
Affected callers: `createMatrixAdjacencyInverse()` → `invAM.inverse(AM)` /
`invAM.inverseByGaussJordanElimination(AM)`; `centralityEigenvector()` → `AM.powerIteration(...)`;
`centralityInformation()` → `invM.inverse(WM)`.

**I2 — `createMatrixAdjacencyInverse()` doesn't check the cancel flag mid-computation**, only
before `inverse()`/`inverseByGaussJordanElimination()` starts (`graph/matrices/graph_matrix_adjacency.cpp:156`,
the existing partial fix). See I1.

**I3 — `writeMatrix()` had missing `file.close()` on cancel paths.** Fixed during #52.

**I4 — No cancellation support in `Matrix::distancesMatrix()`**, called from `writeMatrix()`
(`graph/reporting/graph_reports.cpp:5698`) for the `MATRIX_DISTANCES_EUCLIDEAN`/`HAMMING`/`JACCARD`/
`MANHATTAN`/`CHEBYSHEV` cases — no cancel guards yet.

## Milestones

### A1 — Inventory (done as part of this design pass)

The six-directory scatter and the 11-field catalog in Current Reality above is A1's output.
Prerequisite for A4 (isolating construction into `matrices/`).

### A2 — APSP Storage Migration (incoming from WS3 M1)

*Handed off from [`roadmap_ws3_architecture_performance.md`](roadmap_ws3_architecture_performance.md),
the "M1 continuation" section.*

**Current state (post WS3 M1):** `m_distance` and `m_shortestPaths` on `GraphVertex` are
per-vertex `QHash<target_vertex_num, QPair<relation_id, value>>`. Access during back-propagation:
`shortestPaths(v1)` iterates entries for key `v1` to find the one matching `m_curRelation` — a
hash lookup per predecessor per vertex per source. For V=5,000 sources with average degree k=10,
that's O(V²k) = 250M hash lookups just for sigma reads. Confirmed by live profiling (`sample`,
2000N/40000E, post-WS14 so the `qDebug()` noise that made earlier profiles unreadable is gone):
`DistanceEngine::finalize()`'s O(N²) pair loop spends ~73% of its samples in
`GraphVertex::distance()` — the QHash lookup this migration removes.

**Target state:** a centralised relation-keyed matrix pair on `Graph`:
```
Graph::m_apspDist:   QHash< relation_id, Matrix >   — geodesic distances
Graph::m_apspSigma:  QHash< relation_id, Matrix >   — sigma counts
```
Row = source vertex position, column = target vertex position. APSP reads become flat array
lookups (O(1)); write-back is a single flat write per source, safe without any lock since each
parallel source owns its own row exclusively — no mutex involved, in either the old or new storage
(per-source row-uniqueness already made the QHash write-back race-free, per
`roadmap_ws3_architecture_performance.md`). A2's win is lookup speed and, per A2.0 below, memory —
not synchronization.

**Memory — the open question, resolved by A2.0 below:** a flat `Matrix` always allocates N² cells;
`QHash` only stores entries for reachable pairs. So `Matrix` should win for connected graphs and
could lose for graphs with many isolates/disconnected components — whether that crossover matters
for realistic SocNetV networks is what A2.0 measures.

### A2.0 — Empirical validation ✅ Done, GO (2026-07-31)

Built a standalone tool, `src/tools/matrix_storage_bench.cpp` (`BUILD_MATRIX_BENCH`, off by
default) + `scripts/run_matrix_storage_bench.sh`, comparing `QHash<int, QPair<int,qreal>>`
(mirrors `GraphVertex::m_distance` exactly) against a flat `Matrix`, at N=1,000/7,343, across three
topologies: **connected** (every vertex reachable), **disconnected** (~8 equal-sized components +
5% isolates), and **giant** (one dominant component + a long tail of small ones — the realistic
shape for real networks; added after finding "8 equal islands" was an unrepresentative test case).
Checksums matched exactly between `qhash` and `matrix` for every configuration, confirming both
held identical values.

*Methodology note:* memory was originally measured in-process (`getrusage`/`mach_task_basic_info`),
which under-reported by 6×-14× at scale — this tool's tight, single-threaded, syscall-free loops
don't generate enough kernel bookkeeping events for those counters to keep up. Fixed by measuring
externally instead, via `/usr/bin/time -l`/`time -v` wrapping each run.

| N | Topology | Structure | Construct (ms) | Lookup (ms) | Memory |
|---|---|---|---|---|---|
| 1,000 | connected | qhash | 70 | 65 | 34.0 MB |
| 1,000 | connected | matrix | 6 | 2 | 7.8 MB |
| 1,000 | disconnected | qhash | 7 | 51 | 4.0 MB |
| 1,000 | disconnected | matrix | 2 | 2 | 7.8 MB |
| 1,000 | giant | qhash | 54 | 58 | 29.6 MB |
| 1,000 | giant | matrix | 5 | 2 | 7.8 MB |
| 7,343 | connected | qhash | 3,606 | 3,523 | 1,919 MB |
| 7,343 | connected | matrix | 325 | 117 | 461 MB |
| 7,343 | disconnected | qhash | 403 | 2,656 | 225 MB |
| 7,343 | disconnected | matrix | 120 | 117 | 461 MB |
| 7,343 | giant | qhash | 2,896 | 3,244 | 1,601 MB |
| 7,343 | giant | matrix | 289 | 123 | 461 MB |

**Lookup speed:** `matrix` wins every configuration, 22×-32×, independent of topology.

**Memory:** `Matrix`'s footprint is topology-independent (always N² cells, ~461 MB at N=7,343).
`QHash`'s footprint tracks reachability: **4.2× more** than matrix when connected, **2.0× less**
only for the artificial "8 equal islands" case, **3.5× more** for the realistic `giant` shape. Real
networks — including `geom.net`-scale ones this migration targets — look like `giant`, not "equal
islands," so `Matrix` wins on both memory and speed for the topologies that actually matter. (These
numbers are also against `Matrix`'s current N+1-allocation implementation — A3's contiguous-buffer
work would widen the memory advantage further, not narrow it.)

**Decision: GO.**

**Extension:** `reachability/graph_reachability_walks.cpp` already uses `Matrix::pow()` directly
with no `QHash`/`QMap` involved, so it doesn't need this migration.

### A2 implementation — 🚧 in progress

- **Step 1 ✅ done:** `Graph::m_apspDist`/`m_apspSigma` added; `DistanceEngine` writes both the new
  matrices and the old per-vertex `QHash` in parallel (nothing reads the new storage yet). Golden
  compares and distance-type benchmarks unchanged, confirming no behavior/perf change at this step.
- **Step 2 (next):** migrate the 18 read call sites off `GraphVertex::distance()`/`shortestPaths()`
  onto the new matrices, one file/group per commit, golden-verified each time.
- **Step 3:** remove the old `GraphVertex` storage and accessors once nothing reads them.
- **Completion criteria:** `run_golden_compares.sh`/`run_benchmarks.sh` pass with no regression;
  benchmark the back-propagation-heavy cases (e.g. `DIST_GRAPHML_1000N_10000A_C1_W0`) for a measured
  speedup, matching the evidence standard WS3 M1 set with its 2.7×–8.3× table.

### A3 — `Matrix` contiguous storage (P1)

Replace `MatrixRow*` (N separately-allocated row objects) with one contiguous `qreal*` buffer of
size `rows*cols`, row-major, inside `Matrix` itself. `MatrixRow`'s public interface (`column()`,
`operator[]`, `setColumn()`) can likely be preserved as a thin non-owning view into a slice of the
shared buffer, keeping every existing call site source-compatible — verify this as the actual
implementation approach before starting.

**Completion criteria:** `run_golden_compares.sh` and `run_benchmarks.sh` pass with no numeric
change; measure allocation count and construction time for a large reference matrix (e.g. building
`AM` for `geom.net`, N=7343) before and after — one allocation instead of 7344, with the actual
measured construction-time delta reported, same evidence standard as A2.0.

### A4 — Isolate construction into `matrices/`

Migrate the five misplaced construction sites (everything outside
`matrices/graph_matrix_adjacency.cpp` from A1's inventory) into the `matrices/` slice directory,
one call site at a time, golden-regression-verified after each move.

### A5 — Cancellation-aware algebra kernels (I1, I2)

Insertion points, found by reading each method directly:
- **`Matrix::inverse()`** (`src/matrix.cpp:1256`) — after the one-time `ludcmp()` decomposition,
  the actual cost is a `for (j=0; j<n; j++)` loop calling `lubksb()` once per column (line 1279).
  A cancellation check at the top of this loop interrupts between columns.
- **`Matrix::inverseByGaussJordanElimination()`** (`src/matrix.cpp:1003`) — same treatment, its own
  outer loop. **But check reachability first (2026-07-30): this method has no reachable caller.**
  `createMatrixAdjacencyInverse()` only calls it when `method == "gauss"`
  (`graph/matrices/graph_matrix_adjacency.cpp:162-165`), its sole caller passes `"lu"`
  (`graph/reporting/graph_reports.cpp:5780`), and the parameter default is also `"lu"`
  (`graph.h:823`). Adding cancellation support to dead code is wasted work — decide whether to
  delete the method outright before doing A5 on it. It also carries an O(N³) logging problem (4
  unconditional `qDebug()` calls in its innermost `for k` loop, `src/matrix.cpp:1057-1074`, three of
  which compute arithmetic purely in order to print it), which is why
  [WS14](roadmap_ws14_logging_cost.md)'s L4 needs the same decision. Make it once, in whichever
  workstream gets there first, and record it in both.

  **Decision made (2026-07-31, in WS14's L4): keep it, don't delete.** Its logging was converted to
  `qCDebug(lcMatrix)` like the rest of `matrix.cpp` — the O(N³) landmine above is defused (near-free
  when the category is disabled) but the method itself still has no reachable caller. A5's
  cancellation-support work on this method is therefore still adding a feature to dead code — that
  part of the original concern stands regardless of the logging decision, and is A5's call to make,
  not WS14's.
- **`Matrix::powerIteration()`** (`src/matrix.cpp:808`) — already a bounded
  `do { ... } while (iter < maxIter && ...)` loop; check goes at the top of the existing loop body.

**Approach:** add an optional `std::function<bool()> cancelCheck` parameter to each method
(defaults to `nullptr` — a no-op for every existing call site), matching the existing
`Graph::progressCanceled()` convention already checked at ~30 call sites across `centrality/`,
`clustering/`, `distances/`, `generators/`, `cohesion/`. `createMatrixAdjacencyInverse()`
(`graph/matrices/graph_matrix_adjacency.cpp:143`) would pass `[this]{ return progressCanceled(); }`
instead of only checking before the call (I2's current partial fix).

**Completion criteria:** golden/benchmark scripts pass unchanged; manual test — cancel a slow
inversion/power-iteration mid-computation on a large network, confirm the app returns to
responsive state within roughly one loop iteration instead of running to completion.

### A6 — Cancel guards in `writeMatrix()` (I4)

Add `progressCanceled()` checks to the `MATRIX_DISTANCES_EUCLIDEAN`/`HAMMING`/`JACCARD`/`MANHATTAN`/
`CHEBYSHEV` cases in `writeMatrix()` (`graph/reporting/graph_reports.cpp:5698`), matching the
pattern I3 already established for the file-handle-cleanup paths in the same function.

### A7 — Golden coverage for matrix operations

Extend the CLI kernel harness (WS6) with small golden-output baselines for at least one operation
per matrix category (adjacency, inverse, distance, similarity), ahead of A3/A5.

## Work Rules

- Performance claims must be measured (allocation counts, RSS, benchmark deltas), not asserted —
  A2.0 exists specifically to enforce this for the APSP migration.
- `run_golden_compares.sh` and `run_benchmarks.sh` must pass after every milestone.
- A3 (storage) and A5 (cancellation) are independent and can proceed in either order; A4
  (isolation) should wait until both have landed so it moves settled code.
