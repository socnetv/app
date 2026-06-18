# Distance & Geodesic Engine Refactor Roadmap (WS1)

**Status: Complete.** This document is an architectural history and reference record.
Active DistanceEngine work is tracked in [`roadmap_architecture_performance.md`](roadmap_architecture_performance.md) (WS3).

---

## Goal

Extract `Graph::graphDistancesGeodesic()` — a monolithic BFS/Dijkstra + Brandes + progress
+ result-storage method — into a standalone, testable `DistanceEngine` while preserving
exact numeric results and UI behaviour.

---

## What WS1 Delivered

### Phase A — Extraction Foundation ✅

- `DistanceEngine` introduced as the owner of the geodesic-distance computation.
- `DistanceEngine` accesses `Graph` internals via `friend class DistanceEngine;`
  (explicit transitional choice; narrowing it is deferred to WS3 M2+).

### Phase B — Verified Behavioral Parity ✅

- Verified against Zachary's Karate Club: identical distances, centralities, and
  prestige indices vs SocNetV 3.2 release.

### Phase C — Internal Structure ✅

- `compute()` broken into `initRun()`, `runAllSources()`, `finalize()`.
- Explicit scratch structs (`DistanceScratch`, `CentralityScratchSSSP`) introduced to
  replace giant parameter lists. These cover per-run scalars, iterators, N/E snapshots,
  and connectivity tracking.

### Phase D — Decoupling & Safety Net ✅

#### D.1 — Progress / UI Decoupling

- Introduced `IDistanceProgressSink` interface.
- Implemented `GraphDistanceProgressSink` (Qt/UI-backed) and `NullDistanceProgressSink`
  (headless/CLI usage).
- All direct `emit graph.*` calls replaced with sink calls.
- **Result:** `DistanceEngine` no longer depends on Qt signals; headless execution is possible.

#### D.2 — Reduced Graph Internals Access (transitional — superseded by WS3)

Introduced intent-revealing accessor methods on `Graph` and `GraphVertex` to wrap
per-source SSSP scratch state rather than having `DistanceEngine` mutate it directly.
This was always a transitional step; the underlying data members were removed in the
WS3 Phase 2 dead-code cleanup (see below).

#### D.3 — Golden Regression Harness ✅

Established a deterministic, format-agnostic regression harness via the headless CLI tool
(`socnetv-cli`). This became the foundation of WS6.

**What was implemented:**

- Headless CLI execution path (`socnetv-cli --kernel distance --compare-json`)
- Deterministic JSON output schema (v1): graph-level metrics + per-node vectors (CC, BC,
  SC, EC, PC, distance_sum, eccentricity) serialised with strict vertex-id ordering and
  string-formatted floats
- Strict comparison mode (non-zero exit on mismatch — CI-safe)
- Initial baseline coverage: GraphML (Erdős–Rényi N=10, small-world N=10), UCINET DL
  (Stokman–Ziegler, weighted + unweighted), Pajek (Dunbar Gelada baboon colony,
  weighted + unweighted)

**Determinism guarantees:** vertex order sorted by id; floats serialised as strings; NaN
handled explicitly; field-by-field comparison catches subtle algorithmic drift (stack
ordering, Brandes accumulation changes, loader semantics).

The harness has since grown to 7 kernel families and 36+ baselines (WS6).

#### D.4 — Engine Boundary Documented ✅

Ownership defined: `DistanceEngine` owns algorithm flow and scratch lifetime;
`Graph` owns storage and connectivity bookkeeping; UI owns sink construction and
signal translation.

#### D.5 — Physical Extraction from `graph.cpp` ✅

- `src/engine/distance_engine.h` and `distance_engine.cpp` created.
- `graph.cpp` reduced to constructor + `clear()`.

#### D.6 — Micro-benchmarking ✅

- Benchmark harness introduced: `scripts/run_benchmarks.sh`.
- Timing output (ms, N, E) with configurable baseline sets.
- Performance regression guardrail vs v3.2 established.

---

## Transitional SSSP Accessors (D.2) — Superseded and Removed by WS3

D.2 kept per-source scratch state inside `Graph` and `GraphVertex` but wrapped it behind
intent-revealing accessors. WS3 Phase 1 extracted all of this into `PerSourceScratch`;
WS3 Phase 2 parallelised the source loop and removed the transitional surface entirely.

**Removed in WS3 Phase 2 dead-code cleanup:**

| Group | Members removed |
|---|---|
| BFS/Brandes stack | `ssspStack*` methods, `Graph::Stack` |
| Power Centrality nth-order | `ssspNthOrder*` methods, `Graph::sizeOfNthOrderNeighborhood` |
| Component accumulator | `ssspComponent*` methods, `Graph::sizeOfComponent` |
| Predecessor lists | `GraphVertex::clearPs()`, `appendToPs()`, `Ps()`, `myPs` |
| Dependency accumulators | `GraphVertex::setDelta()`, `delta()`, `m_delta` |

**Still live** (connectivity bookkeeping, not per-source scratch):
```cpp
void notConnectedPairsClear();
void notConnectedPairsInsert(int from, int to);
int  notConnectedPairsSize() const;
```

---

## Current Engine Shape (post-WS3)

```
DistanceEngine::compute(computeCentralities, considerWeights, inverseWeights, dropIsolates)
  initRun()
  runAllSources()          ← parallel via QtConcurrent::blockingMap (WS3 Phase 2)
  finalize()
```

**Scratch state layers:**

| Struct | Scope | Location |
|---|---|---|
| `DistanceScratch` | Per run (N/E snapshots, iterators, connectivity) | `distance_engine.h` |
| `PerSourceScratch` | Per source vertex — reused within a thread | `src/engine/per_source_scratch.h` |
| `ThreadLocalState` | Per worker thread — owns a `PerSourceScratch` + partial BC/SC arrays | `src/engine/thread_local_state.h` |

---

## What Remains Open

- Narrowing / removing the `friend` access between `DistanceEngine` and `Graph` —
  deferred to WS3 M2+ (requires a stable domain model).
- Type tightening (`int` → `qsizetype`) — low priority, deferred.
