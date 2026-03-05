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
