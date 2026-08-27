# Distance & Geodesic Engine Refactor Roadmap (WS1)

## Goal

Extract `Graph::graphDistancesGeodesic()` — a monolithic BFS/Dijkstra + Brandes + progress
+ result-storage method — into a standalone, testable `DistanceEngine` while preserving
exact numeric results and UI behaviour.

## Status

✅ Complete. Current `DistanceEngine` architecture (call flow, scratch-state layering, APSP
storage) lives in [`README_DEVELOPER_NOTES.md`](../README_DEVELOPER_NOTES.md)'s "Distance Engine"
section, not here. Active DistanceEngine work is tracked in
[`roadmap_ws3_architecture_performance.md`](roadmap_ws3_architecture_performance.md) (WS3).

## What WS1 Delivered

- **Extraction foundation** — `DistanceEngine` introduced as the owner of geodesic-distance
  computation, accessing `Graph` internals via `friend class DistanceEngine;` (an explicit
  transitional choice, narrowed further by WS3 M2+).
- **Verified behavioral parity** — checked against Zachary's Karate Club: identical distances,
  centralities, and prestige indices vs. SocNetV 3.2.
- **Internal structure** — `compute()` split into `initRun()`/`runAllSources()`/`finalize()`, with
  explicit scratch structs replacing giant parameter lists.
- **Progress/UI decoupling** — `IDistanceProgressSink` interface (`GraphDistanceProgressSink` for
  Qt/UI, `NullDistanceProgressSink` for headless), so `DistanceEngine` has no Qt-signal dependency.
- **Golden regression harness** (`socnetv-cli --kernel distance --compare-json`) — the deterministic
  JSON-schema foundation WS6 later grew to 7 kernel families.
- **Physical extraction** — `src/engine/distance_engine.{h,cpp}` created; `graph.cpp` reduced to
  constructor + `clear()`.
- **Micro-benchmarking** — `scripts/run_benchmarks.sh` introduced, with a performance guardrail vs.
  v3.2.
- A transitional step (wrapping per-source SSSP scratch behind intent-revealing `Graph`/
  `GraphVertex` accessors) was later fully superseded by WS3 Phase 1/2's `PerSourceScratch`
  extraction and parallel source loop — the transitional accessors no longer exist.

## What Remains Open

- Narrowing/removing the `friend class DistanceEngine` grant on `Graph` — checked directly:
  `DistanceEngine` touches ~85 `graph.X` members through it, all public accessors or cache/aggregate
  fields, squarely WS3 M4 (explicit cache objects) territory, not a domain-model boundary. Parked,
  not scheduled.
- Type tightening (`int` → `qsizetype`) — low priority, deferred.
