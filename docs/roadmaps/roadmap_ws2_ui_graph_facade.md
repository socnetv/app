# Graph as Façade / Coordinator — WS2

## Goal

Turn `Graph` from a monolithic algorithm host into a thin façade/coordinator: keeps state +
invariants, exposes a stable façade API for UI and CLI, delegates algorithms to slices/engines,
centralizes UI orchestration (signals, thread affinity) — does not host algorithm logic directly.

## Status

✅ Complete. Current `Graph`-as-façade architecture (directory layout, algorithm/UI boundary rules)
lives in [`README_DEVELOPER_NOTES.md`](../README_DEVELOPER_NOTES.md)'s "Graph as Façade" and
"Structural Boundary Inside `src/graph/`" sections, not here.

## What WS2 Delivered

- **F0 — Façade contract defined**: what UI/CLI are allowed to call, marked in `graph.h`; internal
  helpers no longer considered public API.
- **F1 — Engine boundary tightened**: `DistanceEngine` extracted with no direct access to `Graph`
  internals (later fully superseded by WS3's `PerSourceScratch` extraction).
- **F2 — Mechanical extraction of `graph.cpp`**: no logic/signature changes, pure translation-unit
  slicing, verified after every slice. The structural core of WS2 — `graph.cpp` now contains only
  the constructor and `clear()`.
- **F3 — UI boundary tightened**: audited `MainWindow`/`GraphicsWidget`/dialogs/graphics items;
  found thread-affinity handling (`thread()`/`moveToThread`) as the one non-façade pattern, fixed
  with façade wrappers.
- **F4 — Algorithm/UI separation enforced**: formal boundary between algorithm slices (compute-only,
  no QtWidgets/QtCharts, no UI signals) and the UI façade layer (`src/graph/ui/`), applied first to
  the prominence-distribution subsystem.

## Notes for the Future

- **F4 had a real gap, found and closed during WS3 (#254, 2026-07):** F4 separated *who* constructs
  QtCharts objects but not *which thread* runs that code — it was safe by accident (every
  `MainWindow` → `Graph` call was, incorrectly, a direct synchronous call, so UI façade code always
  happened to run on the GUI thread already). Once #254 made `Graph` methods actually run on their
  intended `graphThread`, this started constructing QtCharts objects off the GUI thread. Fixed with
  `Graph::runOnGuiThread()` — now documented as the standard pattern in `README_DEVELOPER_NOTES.md`.
  Any new UI façade code must use it.

## What Remains Open

Nothing scoped under WS2 itself. Further engine extraction, IO boundary work, and the
`DistanceEngine`/`Graph` `friend` relationship are tracked under WS3/WS4 respectively.
