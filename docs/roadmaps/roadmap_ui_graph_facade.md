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

### F2 — Split Graph.cpp into “Coordinator vs Services” (mechanical, safe) (ACTIVE)

This is the core WS2 work: progressively turning `Graph` into orchestration glue.

**Work strategy**

* No logic refactoring.
* No signature changes.
* Methods remain `Graph::` members.
* Purely mechanical extraction into new compilation units.
* Golden comparisons + benchmarks after every slice.

---

### Extracted domains (so far)

The following subsystems have been mechanically extracted from `graph.cpp`
into dedicated translation units under `src/graph/`:

- reporting/
- layouts/
  - basic
  - force-directed
- reachability/
- generators/
- crawler/
- cohesion/ (cliques)
- clustering/
  - triad census
  - clustering coefficients
  - hierarchical clustering
- similarity/ (similarity & dissimilarity matrix builders)
- distances/
  - distance façade
  - distance cache / SSSP helpers
- prominence/
  - centrality
  - prestige
  - prominence distributions
- matrices/
  - adjacency matrix builders
  - adjacency inverse builders
- util/
  - matrix/metric/clustering type ↔ string helpers
  - htmlEscaped()

---

### What remains inside Graph.cpp (intentionally)

For now, the following remain in `graph.cpp`:

- storage & invariants
  - vertices
  - edges
  - relations
  - enable/disable logic
- UI wiring
  - signals/slots
  - MainWindow / GraphicsWidget coordination
- file IO
  - load/save formats
- graph-level state flags
  - modified/loaded/saved
- basic structural queries
  - density
  - reciprocity
  - symmetry
  - directed/undirected toggles

These are expected to remain until WS3 (domain separation) and WS4 (IO separation).

---

### Definition of done (ongoing for F2)

- `graph.cpp` continues to shrink materially.
- Extracted slices:
  - compile as independent translation units
  - are listed in `GRAPH_SOURCES` in CMake
  - pass golden comparisons
  - remain within benchmark guardrails
- No behavioral changes introduced.

`Graph` now clearly trends toward a façade/coordinator role rather than an algorithm host.

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
