# Graph as Façade / Coordinator — WS2 (COMPLETED)

## Purpose of This Document

This document records the detailed execution of **WS2**:
turning `Graph` from a monolithic algorithm host into a thin façade / coordinator.

---

## Goal (Achieved)

Make `Graph` a thin coordinator that:

* keeps **state + invariants**
* exposes a **stable façade API** for UI and CLI
* delegates algorithms to slices / engines
* centralizes UI orchestration (signals, thread affinity)
* does not host algorithm logic directly

## Status

✅ Complete. This document now serves as an architectural reference and historical record.

---

## Non-Goals (Respected)

* No behavior changes
* No numeric drift
* No performance regressions
* No domain model rewrite (WS3)
* No IO/parser redesign (WS4)

All changes were mechanical, incremental, and verified.

---

## Original Shape (Before WS2)

Historically, `Graph` mixed:

* UI coordination (Qt signals/slots to `MainWindow`, `GraphicsWidget`, `Parser`)
* storage + graph invariants
* algorithms (distances, centralities, walks, layouts, generators)
* exports/reporting
* crawler/web functionality
* caches and analysis state

`graph.cpp` was a monolithic compilation unit.

---

## Final Shape (After WS2)

### Structural Result

`graph.cpp` now contains only:

* `Graph::Graph(...)`
* `Graph::clear(...)`

All other functionality lives in dedicated translation units under:

```
src/graph/
```

Grouped by responsibility:

* distances/
* centrality/
* clustering/
* cohesion/
* similarity/
* layouts/
* generators/
* prominence/
* matrices/
* reporting/
* storage/
* relations/
* filters/
* core/
* io/
* ui/

Each slice:

* Compiles independently
* Is listed explicitly in `GRAPH_SOURCES`
* Passes golden comparisons
* Remains within benchmark guardrails

---

## Milestones Completed

### ✅ F0 — Façade Contract Defined

* Defined what UI/CLI are allowed to call.
* Marked façade region in `graph.h`.
* Internal helpers are no longer considered public API.

---

### ✅ F1 — Engine Boundary Tightened

* `DistanceEngine` extracted.
* No direct access to `Graph` internals.
* Narrow helper surface for SSSP context introduced (`ssspStack*`, `ssspNthOrder*`,
  `ssspComponent*` accessors on `Graph`; `myPs` / `m_delta` on `GraphVertex`).
* Golden parity verified.

> **Superseded by WS3:** The transitional SSSP helper surface introduced in F1 was a
> stepping stone, not a permanent API. WS3 Phase 1 extracted all per-source scratch state
> into `PerSourceScratch`; WS3 Phase 2 parallelised the source loop and removed the
> transitional accessors and their underlying data members entirely.

---

### ✅ F2 — Mechanical Extraction of `graph.cpp`

* No logic changes.
* No signature changes.
* Pure translation unit slicing.
* Verified after each slice (golden + perf).

This was the structural core of WS2.

---

### ✅ F3 — UI Boundary Tightened

Audit of:

* `MainWindow`
* `GraphicsWidget`
* Dialog forms
* Graphics items

Findings:

* UI interactions are centralized.
* No UI component accesses `Graph` internals directly.
* Thread affinity (`thread()`, `moveToThread`) was the only non-façade pattern.

Actions:

* Introduced façade wrappers for thread management.
* Eliminated direct UI access to QObject internals.

Result:

UI interacts with `Graph` strictly through façade-supported methods.

---

### ✅ F4 — Algorithm / UI Separation Enforced

Formal boundary:

Algorithm slices under `src/graph/`:

* Compute data only
* May use QtCore
* Do **not** construct QtWidgets / QtCharts objects
* Do **not** emit UI signals

UI orchestration under `src/graph/ui/`:

* Builds Qt UI objects
* Handles PNG export
* Emits signals to `MainWindow`

Applied to prominence distribution subsystem.

All QtCharts construction removed from algorithm slice.

Golden and benchmark parity confirmed.

> **Gap found and closed during WS3 (#254, 2026-07):** F4 separated *who* constructs QtCharts
> objects (only `src/graph/ui/` — verified true, confirmed by a full `src/graph/` sweep during
> the #254 fix) but not *which thread* that code runs on. Since `graph_ui_prominence_distribution.cpp`'s
> functions are plain `Graph::` methods (same QObject, same thread affinity as `Graph` itself),
> they ran on whichever thread called them — which was always the GUI thread, because every
> `MainWindow` → `Graph` call was (incorrectly) a direct synchronous call, not a real cross-thread
> dispatch. F4 was safe by accident, not by design. Once #254 made `Graph`'s slow methods actually
> execute on `graphThread` (its real, intended thread affinity), this UI façade code started
> constructing QtCharts objects off the GUI thread — undefined behaviour, causing a real crash.
> Fixed with `Graph::runOnGuiThread(std::function<void()>)` (`graph_ui_facade.cpp`): the three
> `uiProminenceDistribution{Spline,Area,Bars}()` functions now wrap their entire body in it,
> guaranteeing GUI-thread execution regardless of the calling thread. This is now the standard
> pattern for any future UI façade code that constructs real Qt GUI objects — see
> `roadmap_ws3_architecture_performance.md`'s #254 section for the full writeup.

---

## Architectural State After WS2

The runtime flow for analytics now follows:

```
UI
 ↓
Graph (façade)
 ↓
Algorithm slice / engine
 ↓
UI façade (if rendering required)
 ↓
Signal to MainWindow
```

`Graph` is:

* State holder
* Invariant guardian
* Delegation layer
* UI signal coordinator

It is no longer an algorithm host in the structural sense.

---

## What WS2 Enables

WS2 establishes:

* Clear separation between computation and presentation
* Stable façade surface for future refactors
* Safer ground for:

  * WS3 (Domain model split)
  * WS4 (IO boundary clarification)
  * Further engine extraction

WS2 is considered structurally complete.

Future work should build on this boundary rather than weaken it.

---

## Optional Future Step (Not Part of WS2)

~~If/when we want to reduce the “helper surface” between `Graph` and engines, introduce a
small `GraphAccess` / `GraphSSSPContext` struct passed into the engine as a dependency.~~

**Delivered by WS3:** The concept above was realised differently. Rather than a
`GraphAccess` context object passed from `Graph`, WS3 Phase 1 introduced `PerSourceScratch`
— a self-contained per-source struct owned by `DistanceEngine` itself. WS3 Phase 2
extended this with `ThreadLocalState` for parallel execution. The `friend` relationship
between `DistanceEngine` and `Graph` remains for now; narrowing it further is deferred
to WS3 M2+.

---
