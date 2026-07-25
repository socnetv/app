# Matrices Modernization Roadmap (WS5)

## Goal

- Make the matrix subsystem genuinely faster — contiguous storage, flat-array APSP results.
- Isolate matrix creation and computation into coherent, testable types.
- Cancellation-aware algebra kernels.

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
Reality above.

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
`shortestPaths(v1)` iterates all entries for key `v1` to find the one matching `m_curRelation` — a
hash lookup per predecessor per vertex per source. For V=5,000 sources with average degree k=10,
that's O(V²k) = 250M hash lookups just for sigma reads. WS3 M1 also introduced a per-vertex
`std::mutex` array purely to make this QHash storage thread-safe under Phase 2's parallel loop.

**Target state:** a centralised relation-keyed matrix pair on `Graph`:
```
Graph::m_apspDist:   QHash< relation_id, Matrix >   — geodesic distances
Graph::m_apspSigma:  QHash< relation_id, Matrix >   — sigma counts
```
Row = source vertex position, column = target vertex position. APSP reads become flat array
lookups (`sigma[si][wi]`, O(1)); write-back becomes a single flat write with no mutex needed (each
parallel source owns its own row — WS3 M1's mutex array is removed entirely).

**Memory — worked through, not assumed:** `m_distance` stores roughly N−1 entries per vertex for a
connected graph (APSP reaches every other vertex), so ~N² QHash entries total — same order as a
dense matrix, not smaller. Each entry carries a `QPair<int,qreal>` payload (~16 bytes) plus Qt's
hash-table bucket/chain overhead, which is real but not precisely quantified here — that requires
checking Qt 6.10's actual internal `QHash` layout, not assuming a number. A flat `Matrix` cell is
exactly 8 bytes, no per-entry overhead, so the matrix should win on memory too for **connected**
graphs. It should **lose** on memory for graphs with many isolates/disconnected components — the
matrix allocates all N² cells regardless, while the QHash simply never stores an entry for
unreachable pairs. Whether that crossover point matters for realistic SocNetV networks is an open
question, not a settled one — see A2.0.

**A2.0 — Empirical validation (prerequisite, before committing to the rest of A2):** build a small
standalone measurement comparing actual memory (process RSS, not theoretical byte-counting) and
lookup speed for `QHash<int, QPair<int,qreal>>` vs. a flat `Matrix`, at:
- N=100, N=1,000, N=7343 (matching `geom.net`)
- Both a fully-connected topology and a topology with several disconnected components/isolates
  (the case flagged above as a possible matrix loss)

Report actual numbers — memory delta and lookup-time delta per configuration — before deciding
whether to proceed with the full migration, adjust the design (e.g. only switch to `Matrix` when a
component is large/dense enough to be worth it), or drop this milestone. This replaces "the matrix
approach is obviously better" with an actual answer.

**Extension:** `reachability/graph_reachability_walks.cpp` (the walks/A^k matrix code, one of A1's
six scatter locations) already uses `Matrix::pow()` directly with no `QHash`/`QMap` involved, so it
doesn't need this migration.

**Completion criteria:** A2.0 run and reported first. If it confirms a net win for realistic
network sizes/topologies: `run_golden_compares.sh` and `run_benchmarks.sh` pass with no regression;
benchmark the back-propagation-heavy cases specifically (e.g. `DIST_GRAPHML_1000N_10000A_C1_W0`)
for a measured speedup, matching the evidence standard WS3 M1 set with its 2.7×–8.3× benchmark
table.

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
  outer loop.
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
