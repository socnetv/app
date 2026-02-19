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

### ✅ F1 — Tighten Engine Boundary (Style A: minimal-risk) (DONE)

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

### ✅ F2 — Split Graph.cpp into “Coordinator vs Services” (mechanical, safe) (DONE)

This was the core WS2 structural work: turning `Graph` into orchestration glue.

**Work strategy (applied)**

* No logic refactoring.
* No signature changes.
* Methods remain `Graph::` members.
* Purely mechanical extraction into new compilation units.
* Golden comparisons + benchmarks after every slice.

---

### Result

All major subsystems have been mechanically extracted from `graph.cpp`
into dedicated translation units under `src/graph/`, including:

- algorithms (distances, clustering, cohesion, similarity, prominence)
- layouts and generators
- crawler
- reporting
- vertex storage & styling
- edge storage, styling, and filtering
- structural metrics
- graph state flags
- metadata & modification state
- file IO wrappers
- reporting configuration setters

`graph.cpp` now contains only:

- `Graph::Graph(...)`
- `Graph::clear(...)`

All slices:
- compile as independent translation units
- are listed in `GRAPH_SOURCES`
- pass golden comparisons
- remain within benchmark guardrails

F2 structural extraction is complete.

---

### ✅ F3 — Eliminate UI-to-internal access paths (COMPLETED)

Objective:

Ensure UI interacts with `Graph` strictly through façade-supported methods.

What was audited:

* `MainWindow`
* `GraphicsWidget`
* Graphics items
* Dialog forms

Findings:

* All Graph interactions are centralized in `MainWindow`.
* Graphics and dialog layers do not directly access Graph internals.
* No UI component accessed Graph storage containers or helper internals.
* Thread-affinity operations (`thread()`, `moveToThread`) were the only non-façade access pattern.

Actions taken:

* Introduced façade wrappers for thread-affinity management.
* Removed direct UI calls to QObject internals on `Graph`.
* Verified compilation stability.
* Verified golden parity.

Outcome:

* UI layer is now structurally constrained to façade API.
* Internal-only helpers can be progressively reduced in visibility in future steps.
* Graph public surface is now intentional rather than incidental.

Key insight:

Mechanical extraction (F2) made F3 straightforward. Once responsibilities are sliced, façade enforcement becomes an audit task rather than a refactor task.

Note on sequencing:
- WS3 (domain model split) depends on `Graph` being a stable façade (WS2/F3/F4).
- Before starting WS3, we may perform light WS4 preparation work to clarify IO/parser boundaries
  (mechanical isolation only, no parser logic changes) so the domain split does not inherit Qt-signal entanglement
  
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
