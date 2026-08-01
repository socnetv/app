# Matrices Modernization Roadmap (WS5)

## Goal

- Make the matrix subsystem genuinely faster — contiguous storage, flat-array APSP results.
- Isolate matrix creation and computation into coherent, testable types.
- Cancellation-aware algebra kernels.

## Status

🚧 In progress. A1 (inventory) and A2.0 (empirical validation, GO decision) done. A2 (the actual
APSP storage migration) is scoped, not yet implemented. A3–A7 not started.

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

**Status: done, 2026-07-31 — result is GO.** See "A2.0 results" below for the measured data and
the resulting decision.

**New supporting evidence for running it (2026-07-30).** Profiling (`sample`, 2000N/40000E, all
centralities) a build with the WS14 logging cost removed — i.e. with the dominant noise source gone,
so the remaining profile is meaningful — shows the QHash lookups this milestone targets are now the
visible cost inside `finalize()`:

- 6106 / 6369 samples in `DistanceEngine::runAllSources()` — the actual parallel SSSP work.
- Within `DistanceEngine::finalize()` (191 samples): **126 samples, 66 %, in
  `GraphVertex::distance(int const&)`** — the per-vertex `m_distance` QHash lookup.

`finalize()` is an O(N²) nested vertex-pair loop (`distance_engine.cpp:688-736`), so that is ~N²
hash lookups where the target design gives flat row-major array reads. This does not pre-judge
A2.0's memory question (which remains genuinely open, and is the reason the gate exists) — it only
confirms the lookup cost A2 is meant to remove is real and measurable rather than assumed.

**Sequencing note:** run A2.0 *after* WS14's L2, not before. On the current `develop` build this
profile is unreadable — `qDebug()` formatting swamps everything at 43×–72×, and any lookup-speed
number measured through that noise would be meaningless. See
[`roadmap_ws14_logging_cost.md`](roadmap_ws14_logging_cost.md).

**Re-profiled against WS14's finished state (2000N/40000E, `socnetv-cli --kernel distance -c 1`,
same finalize()/GraphVertex::distance() path as above):** the finding holds, but the profile itself
got much thinner, since WS14's full sweep (not just L2) is done now — `sample` only caught 44 total
samples in `DistanceEngine::compute()` for this network, down from 6369 in the pre-sweep capture
above. Of `finalize()`'s 44 samples: **32 (≈73 %) in `GraphVertex::distance()`**, 6 in
`GraphVertex::number()`, 1 in `GraphVertex::isEnabled()`. Same conclusion (the QHash lookup
dominates what's left of `finalize()`'s cost) but a much wider confidence interval than the
191-sample capture this section was originally written against — 44 samples isn't a lot to build a
percentage on. A follow-up profiling run against a larger stress-test network (attempted here at
N=8000/~640K edges, but the synthetic network file got truncated mid-save when the generating
process was killed too early — needs a longer wait for the async save to finish, or a completion
signal instead of a fixed delay) would give a more statistically solid number if this profile is
used as go/no-go evidence for A2 rather than just directional confirmation. The dramatic drop in
raw sample count is itself worth noting: profiling this codebase for anything now generally needs a
bigger input than it used to, since typical operations finish fast enough that a 1ms-interval
sampler barely gets a look in.

**A2.0 results (2026-07-31).** Built a standalone tool, `src/tools/matrix_storage_bench.cpp`
(CMake option `BUILD_MATRIX_BENCH`, off by default), driven by
`scripts/run_matrix_storage_bench.sh`. It builds one structure — `QHash<int, QPair<int,qreal>>`
per vertex (mirrors `GraphVertex::m_distance`/`setDistance()`/`distance()` exactly) or a single
flat `Matrix` — for a given N and topology in one process, times construction and a full O(N²)
lookup sweep (matching `finalize()`'s actual access pattern), and reports a checksum so the
compiler can't dead-code-eliminate the reads. Checksums matched exactly between `qhash` and
`matrix` for every (N, topology) pair, confirming both structures held identical values.

Three topologies, not two — a third was added mid-investigation (see below):
- **connected** — every vertex reachable from every other.
- **disconnected** — ~8 roughly-equal-sized fully-connected components plus ~5% isolates (the
  topology originally specified above).
- **giant** — one dominant component (90% of N) plus a long tail of halving-sized components
  down to isolates. Added because "8 equal islands" turned out to be an unrealistic shape for
  actual networks, which typically have one large component and a long tail of small ones, not
  several similarly-sized ones — see the finding below.

*Methodology correction, worth recording:* the tool originally measured RSS in-process
(`mach_task_basic_info` on macOS, later `getrusage`'s `ru_maxrss`), and initially reported implausibly
small memory deltas at N=20,000 (hundreds of MB where multi-GB was expected). Cross-checked against
macOS's own `/usr/bin/time -l` on the same run, which reported ~8 GB — a 6×-14× discrepancy. Root
cause: `ru_maxrss` is updated by the kernel on bookkeeping events (page faults, scheduling activity),
and this tool's tight, single-threaded, syscall-free construction loop doesn't generate enough of
those events for the in-process value to catch up to the true peak before the process reads it —
only the parent-side `wait4()` view (what `time -l`/`time -v` use, read after the child exits) is
accurate here. Fixed by moving memory measurement out of the tool entirely: the driver script wraps
each run in `/usr/bin/time -l` (macOS) or `time -v` (Linux, GNU coreutils) and parses the peak RSS
from there instead.

Measured (peak RSS minus the ~12.3 MB fixed Qt Core process floor, which swamps everything at
N=100 — those rows are omitted as noise):

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

**Lookup speed:** `matrix` wins in every configuration, 22×-32×, independent of topology — this
was never the open question, and the data confirms it cleanly.

**Memory — this was the open question, and it resolves in favor of migrating.** `Matrix`'s
footprint is topology-independent (always N² cells — ~461 MB at N=7,343 regardless of shape).
`QHash`'s footprint depends entirely on how reachable the graph is:
- **connected**: qhash costs **4.2× more** than matrix (1,919 MB vs 461 MB).
- **disconnected** (8 equal islands): qhash costs **2.0× less** than matrix (225 MB vs 461 MB) —
  the only configuration favoring qhash.
- **giant** (dominant component + long tail — the realistic shape): qhash costs **3.5× more**
  than matrix (1,601 MB vs 461 MB).

"8 equal disconnected islands" turned out to be the unrealistic case, not the representative one.
A single large reachable component dominates the total pair count even alongside a long tail of
small components/isolates, so `giant` behaves much closer to `connected` than to `disconnected`.
Real networks — including `geom.net`-scale ones this migration targets — look like "one giant
component plus a long tail," not "several similarly-sized islands." On that basis, `Matrix` wins
on both memory and lookup speed for the topologies that actually matter.

Also worth noting: these `Matrix` numbers are against its **current** N+1-separate-allocation
implementation (measured ~461 MB vs. a theoretical ~411 MB for N=7,343 if it were one contiguous
buffer) — i.e. the numbers above are conservative. A3's contiguous-buffer work would only widen
`Matrix`'s memory advantage further, not narrow it.

**Decision: GO.** Proceed with the full A2 migration (`Graph::m_apspDist`/`m_apspSigma` as
`QHash<relation_id, Matrix>`, replacing per-vertex `QHash` storage on `GraphVertex`).

**Extension:** `reachability/graph_reachability_walks.cpp` (the walks/A^k matrix code, one of A1's
six scatter locations) already uses `Matrix::pow()` directly with no `QHash`/`QMap` involved, so it
doesn't need this migration.

**Completion criteria for the rest of A2** (A2.0 itself is done — see above): implement the
`QHash<relation_id, Matrix>` migration; `run_golden_compares.sh` and `run_benchmarks.sh` pass with
no regression; benchmark the back-propagation-heavy cases specifically (e.g.
`DIST_GRAPHML_1000N_10000A_C1_W0`) for a measured speedup, matching the evidence standard WS3 M1
set with its 2.7×–8.3× benchmark table.

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
