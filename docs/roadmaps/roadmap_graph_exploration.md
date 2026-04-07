# Roadmap: Graph Exploration (WS9)

## Overview

This roadmap defines the evolution of SocNetV from a visualization-focused application into a full **graph exploration and data workflow platform**.

It consolidates three major feature tracks:

* **Feature 1 (#209)**: Visualization & decluttering
* **Feature 2 (#215)**: Filtering, querying & subgraphs
* **Feature 3 (#223)**: Structured data workflows

---

## Core Vision

SocNetV should support the full workflow:

```
Load → Visualize → Filter → Explore → Extract → Edit → Export
```

---

## Architectural Direction

### Current State

* Graph is tightly coupled with UI (MainWindow)
* Many operations are dialog-driven
* No unified filtering or projection layer

### Target Model

```
Graph (data)
    ↓
Filter / Projection Layer
    ↓
UI (GraphicsWidget, dialogs, tables)
```

### Key Principles

* **Non-destructive operations** (visibility instead of deletion)
* **Stateful filtering** (not one-off dialogs)
* **Reusable logic** across UI components

---

## Constraints

* Single-window application (Qt MainWindow)
* Graph is currently owned by MainWindow
* No multi-document interface (MDI)

### Design Decision (Current)

* Subgraphs are handled as **filtered views (in-place)**
* No new windows for subgraphs (yet)

### Future Direction

* Explore **tab-based multi-graph UI** (preferred over multiple windows)

---

## Feature Breakdown

---

# Feature 1 — Visualization & Decluttering (#209)

## Goal

Make large graphs readable and explorable.

## Phases

### Phase 1 — Immediate UX

* ✔ Focus on selection (#210) — `Graph::vertexFilterBySelection()`, `filterNodesBySelectionAct` (Ctrl+X, Ctrl+S)
* ✔ Ego networks (k=1) (#211) — `Graph::vertexFilterByEgoNetwork()`, `filterNodesByEgoNetworkAct` (Ctrl+X, Ctrl+F)
* ✔ Hide non-selected nodes (#212) — closed as duplicate of #210
* ✔ Edge filtering by weight (#213) — `Graph::edgeFilterByWeight()`, dialog + `Graph::edgeFilterReset()`, `editFilterEdgesRestoreAllAct` (Ctrl+E, Ctrl+R)

**Cross-cutting UX (Phase 1):**
* ✔ Non-destructive node filter restore — `Graph::vertexFilterRestoreAll()`, `filterNodesRestoreAllAct` (Ctrl+X, Ctrl+R)
* ✔ Right-click on node auto-selects it before context menu opens (`GraphicsWidget::mousePressEvent`)
* ✔ Ego network + Focus on Selection + Restore All Nodes wired into node right-click context menu

### Phase 2 — Layout Improvements (#214)

* ✔ Improved force-directed layout 
* ✔ Radial layout

### Phase 3 — Advanced Visualization

* Community-based layouts
* Edge bundling

---

# Feature 2 — Filtering & Subgraphs (#215)

## Goal

Enable meaningful exploration of large graphs.

## Core Concept

Filtering is a **persistent graph state**, not a temporary action.

---

## Phases

### Phase 1 — Structural Filtering

* Extend existing node filtering (centrality, degree) (#216)
* Integrate edge filtering

### Phase 2 — Attribute Filtering

* Filter by node/edge attributes (#217)
* Requires:

  * Attribute system (#96)
  * Metadata system (#130)

### Phase 3 — Unified Filtering System

* Combine node + edge + attribute filters (#219)
* Introduce logical composition (AND / OR)

### Phase 4 — Subgraph Extraction

* Create subgraphs from:

  * selection
  * filtered view (#218)

**Current approach:**

* Subgraph = visible subset (same graph)

**Future:**

* Tab-based subgraph views

### Phase 5 — Export Filtered Graph

* Export visible subset (#220)

### Phase 6 — Query System

* Visual query builder (#221)
* Optional DSL (long-term)

---

# Feature 3 — Data Workflows (#223)

## Goal

Treat graphs as structured datasets.

---

## Phases

### Phase 1 — Attribute Editing

* Improve node/edge attribute editing (#224)

### Phase 2 — Table Views

* Node table
* Edge table (#225)

### Phase 3 — Structured Export

* CSV / JSON export (#226)

### Phase 4 — Structured Import

* CSV / JSON import (#227)

### Phase 5 — Bulk Editing

* Batch attribute operations (#228)

### Phase 6 — Transformations

* Derived fields
* Value normalization (#229)

---

## Cross-Cutting Systems

### Attribute System (#96)

Foundation for:

* filtering
* editing
* export/import

### Metadata System (#130)

Defines:

* ingestion
* persistence
* usage of attributes

### Temporal Data (#222)

Future extension:

* time-based filtering
* timeline UI

---

## UI Evolution

### Current

* Dialog-driven workflows

### Target

* Persistent panels:

  * Filter panel
  * Table views
  * Attribute inspector

---

## Implementation Strategy

### Short-term

* Reuse existing functionality
* Refactor incrementally

### Mid-term

* Introduce shared filtering logic
* Decouple UI from graph operations

### Long-term

* Introduce explicit projection layer
* Support multiple graph views (tabs)

---

## Outcome

SocNetV evolves into:

* Graph visualization tool ✔
* Graph analysis tool ✔
* Graph exploration platform ✔
* Graph data workflow tool ✔

---

## Notes for Contributors

* Prefer non-destructive operations
* Avoid duplicating filtering logic
* Reuse attribute system consistently
* Keep UI simple and incremental

