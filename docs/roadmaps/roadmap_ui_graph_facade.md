## Graph as Façade / Coordinator Roadmap

### Goal

Make `Graph` a thin coordinator:

* keeps **state + invariants**
* exposes a **stable façade API** for UI and CLI
* delegates algorithms to **engines/services**
* progressively removes “algorithm host” responsibilities from `Graph.cpp`

### Non-goals

* No behavior changes.
* No mass renaming.
* No “big bang” new domain model (that’s WS3).
* No IO/parser overhaul yet (WS4 later).

---

## Current shape (as seen in `graph.h`)

`Graph` currently mixes:

* UI coordination (Qt signals/slots to `MainWindow`, `GraphicsWidget`, `Parser`)
* storage + graph invariants (vertices/edges, relations, enabled/disabled)
* algorithms (distances/centralities, reachability/walks, layouts, random networks)
* exports/reporting
* crawler/web stuff

We have already completed the most important move: **SSSP kernel ownership moved into `DistanceEngine`** (BFS + Dijkstra), and golden compares pass.

---

## Roadmap milestones

### ✅ F0 — Freeze a “Façade Contract” (DONE)

**Deliverables**

* A short list of what UI/CLI is allowed to call on `Graph` going forward.
* Everything else becomes “internal”, even if it still physically lives in `Graph` for now.

**Rules**

* UI code should only call things we consider FACADE API.
* New UI features must use façade calls only.
* Engines/services must not call Graph signals/slots or UI wiring.
* New engines must use a narrow “read-only / controlled-write” surface.


**Definition of done**

* Comment blocks in `graph.h` marks the façade region.

---

### F1 — Tighten Engine Boundary (Style A: minimal-risk) ✅ (continue)

You already started this style.

**Pattern**

* Engines do *not* touch `m_graph`, `vpos`, `Stack`, `sizeOfNthOrderNeighborhood` directly.
* Instead, `Graph` exposes tiny “SSSP helpers”, e.g.:

  * `ssspStackPush/pop/top/...`
  * `ssspNthOrderIncrement/value/...`
  * read-only edge access via `GraphVertex::outEdges()/inEdges()`

**Next target**

* Do the same for any remaining direct accesses in `DistanceEngine`

  * eliminate remaining “engine reaches into Graph internals” step-by-step

**Definition of done**

* `DistanceEngine` compiles with **no direct field access** except through these narrow helpers.
* Any remaining `friend` use must be justified and explicitly documented.

---

### F2 — Split Graph.cpp into “Coordinator vs Services” (mechanical, safe)

This is the actual WS2 “Graph becomes glue” work.

**Work strategy**

* Do not refactor logic yet.
* Just move blocks into new compilation units. Methods remain `Graph::` members (no logic or ownership change).

**Slices based on your `graph.h` grouping**

1. `GraphFacade` layer stays in:

   * `graph.h/.cpp` (thin public methods only)
2. Extract into new modules (no logic changes):

   ✅ `src/graph/reporting/graph_reports.cpp`  ← from **REPORT EXPORTS**
   ✅ `src/graph/layouts/graph_layouts_basic.cpp`  ← from **LAYOUTS**
   ✅ `src/graph/layouts/graph_layouts_force.cpp`  ← from **LAYOUTS** (force-directed + helpers)
   ✅ `src/graph/reachability/...` ← from **REACHABILITY AND WALKS**
   ✅ `src/graph/generators/graph_random_networks.cpp` ← from **RANDOM NETWORKS**
   ✅ `src/graph/crawler/graph_crawler.cpp` ← from **CRAWLER**
3. Leave storage/invariants inside `Graph` for now:

   * **RELATIONS / VERTICES / EDGES / INIT AND CLEAR**

**Definition of done**

* `graph.cpp` shrinks materially as slices move out.
* Extracted slices compile as independent translation units.
* Guardrails: golden compares + benchmarks run after every slice.
* No behavior change (golden compares identical to baseline).
* Major analysis domains extracted (cohesion, clustering, similarity).

#### Current state:

Extracted slices (all mechanical, no behavior change):

- reporting/
- layouts/basic
- layouts/force-directed
- reachability/
- generators/
- crawler/
- cohesion/ (cliques)
- clustering/
  - triad census
  - clustering coefficients
  - hierarchical clustering
- similarity/ (matrix builders)
- distances/
  - distance façade
  - distance cache/SSSP helpers
centrality/
  - centrality
  - prestige

All slices:
- compile as independent translation units
- are included via GRAPH_SOURCES in CMake
- pass golden comparisons
- remain within benchmark guardrails

`graph.cpp` has been materially reduced and now trends toward a façade/coordinator role.


---

### F3 — Kill UI-to-internals access paths (incremental)

**Goal**

* UI code should stop depending on “Graph internals by accident”.

**Actions**

* Introduce façade calls where UI currently reads/writes deep state.
* Replace “public member-ish” patterns with getters/setters.
* Start marking internal-only methods `private` or moving them behind services.

**Definition of done**

* UI code compiles without needing incidental Graph internals.
* “Façade Contract” is enforced socially + by structure (headers).

---

### F4 — Reduce “god-object” coupling (signals/slots cleanup boundary)

This is still WS2 (because it’s about Graph being coordinator, not algorithm host).

**Actions**

* Make a deliberate boundary:

  * UI wiring stays in `Graph` (signals/slots)
  * algorithms stay out
* For each extracted service:

  * no Qt UI includes
  * no signals
  * any progress reporting uses sinks/interfaces (like you did with distances)

**Definition of done**

* Algorithm modules don’t include UI headers.
* Graph stays the only place that “knows” about UI orchestration.

---

## Optional future step: Style B (bigger cleanup, later)

When you’re ready to reduce the helper explosion:

**Style B idea**

* Create a small `GraphAccess` / `GraphSSSPContext` struct (engine-facing)

  * exposes only what SSSP needs
  * constructed by `Graph`, passed into `DistanceEngine`
* This shrinks the number of tiny forwarding methods and helps remove `friend`.

Not now; only after WS2 stabilizes.
