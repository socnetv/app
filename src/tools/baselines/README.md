# Baselines

This folder contains committed golden outputs for `socnetv-cli`.

Baselines are used to detect regressions while refactoring core algorithms.

## Naming convention

Recommended:

`<DATASET>__C<0|1>_W<0|1>_IW<0|1>_DI<0|1>.json`

- C: computeCentralities
- W: considerWeights
- IW: inverseWeights
- DI: dropIsolates

Example:

`SmallWorld_N10_E12__C1_W0_IW1_DI0.json`

## Notes

- `LINKS` are SNA-style directed ties.
- `EDGES` and `ARCS` are derived for convenience.
- Baselines should be generated using the same dataset files in `src/data`.