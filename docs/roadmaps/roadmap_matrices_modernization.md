# Matrices Modernization Roadmap (Skeleton)

## Goal
Isolate matrix creation and computations into coherent types and services.

## Current Reality
- Matrix-related logic is scattered and sometimes intertwined with Graph/UI.
- Matrix algebra methods (inverse, power iteration, etc.) run synchronously
  on the main thread with no cancellation support and no progress reporting.
- Callers cannot interrupt mid-computation; cancellation only works at the
  boundary between Graph-level methods, not inside linear algebra kernels.

## Known Issues (found during #52 Cancel-button fix)

### I1 — Matrix algebra methods are not cancellation-aware
`Matrix::inverse()`, `Matrix::inverseByGaussJordanElimination()`, and
`Matrix::powerIteration()` run to completion regardless of user cancel.
Once `createMatrixAdjacency()` completes and hands off to these methods,
there is no way to interrupt them.
Affected callers:
- `createMatrixAdjacencyInverse()` → `invAM.inverse(AM)` or
  `invAM.inverseByGaussJordanElimination(AM)`
- `centralityEigenvector()` → `AM.powerIteration(...)`
- `centralityInformation()` → `invM.inverse(WM)`
Fix direction: pass a cancellation-check callable into these methods,
or split them into iterative steps that check a flag between iterations.

### I2 — `createMatrixAdjacencyInverse()` does not check cancel flag
after `createMatrixAdjacency()` returns. Added a guard before the inversion
call as a partial fix (cancels before algebra starts), but cannot cancel
mid-inversion. See I1.

### I3 — `writeMatrix()` had missing `file.close()` on cancel paths
Fixed during #52: all early-return cancel paths now close the file and
return `false`. Callers in MainWindow now check the return value.

### I4 — No cancellation support in similarity/dissimilarity distance
matrix computations (`Matrix::distancesMatrix()`). Called from
`writeMatrix()` for MATRIX_DISTANCES_EUCLIDEAN/HAMMING/JACCARD/MANHATTAN/
CHEBYSHEV cases. These cases do not yet have cancel guards in `writeMatrix()`.

## Target Direction
- Clear matrix types (adjacency, laplacian, distance, similarity, etc.)
- Deterministic constructors
- Cancellation-aware algebra kernels (at minimum: inverse, power iteration)
- Progress reporting from inside long algebra operations
- Headless tests

## Milestones
- A1: Inventory matrix-related classes and their current callers
- A2: Extract construction code paths
- A3: Add golden outputs for small graphs
- A4: Add cancellation support to Matrix algebra methods (see I1)
- A5: Add cancel guards to remaining writeMatrix() cases (see I4)

---

## Incoming from WS3 — APSP Storage Migration (Phase 3)

*Handed off from [`roadmap_domain_model_split.md`](roadmap_domain_model_split.md) Phase 3.*

### Current state (post WS3 Phase 2)

After WS3 Phase 2, `m_distance` and `m_shortestPaths` on `GraphVertex` are still
per-vertex QHash stores:

```
m_distance:       QHash< target_vertex_num, pair(relation_id, geodesic_distance) >
m_shortestPaths:  QHash< target_vertex_num, pair(relation_id, sigma_count) >
```

Access during back-propagation: `shortestPaths(v1)` iterates all entries for key `v1`
to find the one matching `m_curRelation` — a hash lookup per predecessor per vertex
per source. For V=5 000 sources with average degree k=10, that is O(V²k) = 250M hash
lookups just for sigma reads.

Phase 2 also introduced a per-vertex `std::mutex` array for write-back safety.
Phase 3 eliminates both the lookup overhead and the mutex array.

### Target state

Replace the distributed per-vertex storage with a centralised relation-keyed matrix pair
on the `Graph` object:

```
Graph::m_apspDist:   QHash< relation_id, Matrix >   — geodesic distances
Graph::m_apspSigma:  QHash< relation_id, Matrix >   — sigma counts
```

Where `Matrix` is the existing SocNetV `Matrix` class (or a thin flat-array wrapper
for cache efficiency). Layout: row = source vertex position, column = target vertex position.

Benefits:
- APSP reads during back-propagation become flat array lookups: `sigma[si][wi]` — O(1),
  cache-friendly, no hash collision handling
- APSP write-back in Phase 2 becomes a single flat write per (source, target) pair —
  no mutex needed (each parallel source owns its own row)
- The per-vertex mutex array introduced in Phase 2 is removed
- `m_distance` and `m_shortestPaths` are removed from `GraphVertex`, reducing per-node
  memory footprint
- The `Matrix` class gains a well-defined role as the APSP result store, consistent
  with WS5 goals (cancellable, testable matrix subsystem)

### Public API preservation

The accessors `graph.distance(u, v)` and `graph.shortestPaths(u, v)` keep their
signatures — only their backing storage changes. All callers outside `DistanceEngine`
are unaffected.

### Milestone

- A6: Migrate `m_distance` / `m_shortestPaths` to centralised `Matrix` per relation;
  remove per-vertex QHash storage and Phase 2 mutex array; verify with
  `run_golden_compares.sh` and `run_benchmarks.sh`
