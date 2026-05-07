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

# Feature 1 — Visualization & Decluttering (#209) ✔

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
* ✔ Ego-centered radial layout (#214) — `Graph::layoutEgoRadial()`, Layout menu (`Ctrl+Alt+E`) and node right-click context menu

### Phase 3 — Advanced Visualization

* Community-based layouts
* Edge bundling

---

# Feature 2 — Filtering & Subgraphs (#215)

## Goal

Enable exploration of large graphs through non-destructive filtering, attribute-based and structural queries, and subgraph extraction.

## Key Concept

Introduce a **Graph View / Projection Layer**:

```
Graph (data) → Filter / Projection Layer → UI
```

The underlying graph remains unchanged; filtering operates on visibility state.

## Dependencies

* Node/edge attribute system (#96, #130)
* Structured data workflows (#223)
* Graph façade (WS3)
* Parser improvements (WS4)

---

## Phases

### Phase 1 — Structural Filtering ✔

* ✔ Extend existing node filtering (centrality, degree) (#216) — centrality filter integrated into snapshot/restore history stack
* ✔ Integrate edge filtering by weight — `Graph::edgeFilterReset()`, `editFilterEdgesRestoreAllAct` (`Ctrl+E, Ctrl+R`)
* Filter edges by relation type — switch active relation hides cross-relation edges (existing behaviour); dedicated "show only relation X" action is future work

### Phase 2 — Attribute Filtering ✔

* ✔ Filter nodes and edges by attribute (#217):
  * `FilterCondition` struct (scope, key, op, value; `label()` for chip text) in `src/graph/filters/filter_condition.h`
  * `DialogFilterByAttribute` — scope radio (Nodes/Edges/Both), editable key combo populated from graph attributes, operator dropdown (`=` `≠` `>` `<` `≥` `≤` `contains`), value field; emits `userChoices(FilterCondition)`
  * `Graph::vertexFilterByAttribute(const FilterCondition &)` — non-destructive, snapshot/restore stack (`Ctrl+X, Ctrl+A`)
  * `Graph::edgeFilterByAttribute(const FilterCondition &)` — same snapshot/restore stack as node filters
  * Numeric-aware evaluation: compares as `double` when both sides parse; falls back to lexicographic; `contains` is case-insensitive substring
  * Filter: combo added to Control Panel (Network group) for quick access
  * Filter toolbar group: dedicated icons for each filter action

### Phase 3 — Unified Filtering System ✔

* ✔ Persistent **filter bar** (#219) — thin strip between toolbar and canvas, auto-shows/hides:
  * Each active condition shown as a chip: `Nodes: type = investor ×`
  * `FilterBarWidget` (`src/widgets/filterbarwidget.h/.cpp`): chips + "Clear all" button
  * ×-close enabled only on the most recently applied chip (stack limitation — arbitrary removal deferred to #221)
  * "Clear all" drains the full node snapshot stack and resets the edge filter
  * All filter actions emit a chip: centrality, ego network, selection, weight, attribute
  * Bar syncs with menu/toolbar restore actions (`Restore All Nodes`, `Restore All Edges`)
  * Styled via `default.qss`
* Logical composition (AND/OR): deferred to #221 (query system); sequential stack already gives AND semantics by effect

### Phase 4 — Subgraph Extraction (#218)

* Create subgraph from selection or current filtered view (#218)

**Current approach:**
* Subgraph = visible subset (same graph object); no copy created

**Future options:**
* Copy-out as independent `Graph` object (serialise visible nodes/edges)
* Optional: open subgraph in a new window
* Preferred long-term: tab-based multi-graph UI (switch between subgraphs)

### Phase 5 — Export Filtered / Extracted Graph (#220)

* Export the currently visible (filtered) subset to any supported format (#220)
* Save a named subgraph to file for later reload
* Basis for save/load subgraph workflows (see Phase 6)

### Phase 6 — Persistent Named Subgraphs

* Maintain multiple named subgraph views derived from the same base graph
* Switch between subgraphs without reloading
* Save and reload named subgraphs (persisted alongside or inside the graph file)
* Relates to: tab-based multi-graph UI (Feature 1 Phase 3), MDI long-term plan

### Phase 7 — Query System (#221)

* Visual no-code query builder — compose conditions across structural and attribute dimensions
* Example query: `degree > 5 AND type = "investor"`
* Arbitrary chip removal (pop any snapshot from the stack, replay remaining — prerequisite for full query semantics)
* Optional DSL (long-term): text-based query language for scripting and the CLI tool

---

# Feature 3 — Data Workflows (#223)

## Goal

Treat graphs as structured datasets.

---

## Phases

### Phase 1 — Attribute Editing ✔

* ✔ Improve node/edge attribute editing (#224) — closes #224
  * Phase A: Single-key node attribute API (`Graph::vertexCustomAttributeSet`, `vertexCustomAttributeRemove`)
  * Phase B: Edge custom attribute storage (`GraphVertex::m_outEdgeCustomAttributes`, `Graph::edgeCustomAttributesSet`)
  * Phase C: `DialogEdgeEdit` — edge properties dialog with custom key/value table (label, weight, color, attributes)
  * Phase D: GraphML roundtrip for edge custom attributes (`d2000+` key definitions on export; parser collects and stores on import)
  * Phase E: `Graph::vertexFilterByAttribute(key, value)` — Filter menu `Ctrl+X, Ctrl+A`; foundation for #217

### Phase 2 — Table Views ✔

* ✔ Node/edge data table dock (#225) 
  * `NodeTableModel` (`QAbstractTableModel`): caches all node rows; fixed
    columns (#, Label, Visible, Shape, Size, Color) plus dynamic custom attrs.
    Read-only cells (#, Visible, Shape) rendered with a muted background.
    `setData()` writes back via `vertexLabelSet`, `vertexSizeSet`, `vertexColorSet`,
    `vertexCustomAttributeSet`.
  * `EdgeTableModel`: caches edge rows for the current relation; fixed columns
    (Source, Target, Relation, Weight, Label, Color) plus dynamic custom attrs.
    Read-only cells (Source, Target, Relation) shaded. `setData()` writes back
    via `edgeWeightSet`, `edgeLabelSet`, `edgeColorSet`, `edgeCustomAttributesSet`.
  * `GraphTableWidget`: `QTabWidget` (Nodes | Edges); each tab has a live-search
    bar (QSortFilterProxyModel, all columns, case-insensitive), a Refresh button,
    and a sortable `QTableView` with inline editing on double-click.
    Emits `nodeSelected(int)` on row click.
  * `QDockWidget` at `BottomDockWidgetArea`; toggled by **Ctrl+T** (`Options`
    menu and `Edit` menu, `viewDataTableAct`). Auto-refreshes on file load and
    graph reset when visible.
  * `viewDataTableAct` has a dedicated `data_table_48px.svg` icon.

### Phase 3 — Structured Export ✔

* ✔ CSV / JSON export (#226) — closes #226
  * `TableExport::toCSV(model, path)` / `TableExport::toJSON(model, path)` —
    free functions in `src/graph/io/table_export.*`; accept any
    `QAbstractItemModel*`; QtCore only, no UI.
  * **Export CSV** / **Export JSON** buttons in each tab of `GraphTableWidget`;
    export the proxy model (currently visible/search-filtered rows); tooltip
    makes the scope explicit.
  * `Network → Export to other...` gains **Nodes as CSV**, **Edges as CSV**,
    **Nodes as JSON**, **Edges as JSON** — always export all rows (source
    model, unfiltered); models are refreshed from `activeGraph` before writing.
  * `GraphTableWidget::exportStatusMessage` signal wired to the MainWindow
    status bar.

### Phase 4 — Structured Import ✔

**Cross-cutting workflow unlocked by Phases 3+4:** spreadsheet-based bulk attribute editing (#232) — export table to CSV/JSON, edit freely in any spreadsheet tool, re-import. Each node/edge gets its own values; native columns (Label, Size, Color, Weight) are routed to their proper setters; read-only columns (Visible, Shape, Relation) are silently skipped; full roundtrip with no data loss or duplicate columns.

* ✔ CSV / JSON attribute import (#227) — refs #169
  * `TableImport::fromCSV(path)` / `TableImport::fromJSON(path)` — free functions in `src/graph/io/table_import.*`; return `ParsedTable{headers, rows, ok, errorString}`; QtCore only, no UI
  * `DialogImportAttributes` (`src/forms/dialogimportattributes.*`) — file-browse + preview table (first 8 rows) + column-mapping controls; parameterised by scope (Nodes / Edges) and format (CSV / JSON); `Import` button disabled until a valid file is loaded
    * Nodes scope: **ID column** combo + **Match by** radio (Node number / Node label)
    * Edges scope: **Source column** + **Target column** combos; auto-selects columns named `source`/`target`/`src`/`tgt`/`dest`
  * `Graph::vertexAttributesImport(headers, rows, idColumn, matchByLabel)` in `graph_vertex_style.cpp` — iterates rows, matches vertices by number or label, calls `vertexCustomAttributeSet()` for each non-ID column; returns matched count
  * `Graph::edgeAttributesImport(headers, rows, srcColumn, tgtColumn)` in `graph_edge_style.cpp` — matches edges by source/target number, merges new attributes via `edgeCustomAttributesSet()`; returns matched count
  * **Import CSV** / **Import JSON** buttons added to each tab of `GraphTableWidget`; invoke `DialogImportAttributes`, call the appropriate Graph method, refresh the table, emit `importStatusMessage`; MainWindow wires `importStatusMessage` → status bar

### Phase 5 — Bulk Editing (#228)

**Goal:** in-app same-value-to-many-targets operations — complementary to the spreadsheet workflow (#232) which already handles heterogeneous per-row editing.

* Assign a single attribute value to all currently selected or filtered nodes/edges
  * Context menu entry: "Set attribute for selection…"
  * Bulk-edit dialog: attribute name field (combo from existing keys) + value field + scope (selection / visible subset)
  * Calls `vertexCustomAttributeSet` / `edgeCustomAttributesSet` for each target in a single transaction
* Remove an attribute key from multiple nodes/edges at once
  * "Remove attribute from selection" — calls `vertexCustomAttributeRemove` per target
* Add a new attribute key (with an optional default value) to all nodes/edges
  * Useful for initialising a new column before importing real values

**Selection sources:**
* Nodes/edges manually selected on canvas
* Current filtered/visible subset (integrates with #215 filter stack)
* Multi-row selection in the Data Table (#225)

**UX rules:**
* Operations must not alter the filter stack (non-destructive)
* Table auto-refreshes after each bulk operation
* All operations must be undoable (undo stack integration — dependency on future undo/redo system)

### Phase 6 — Transformations (#229)

* Derived fields: compute a new attribute value from one or more existing attributes (e.g. `full_name = first + " " + last`)
* Value normalization: min-max or z-score scaling of a numeric attribute across all nodes/edges
* Type coercion: convert stored string values to canonical types (integer, float, boolean) — useful before filtering with numeric operators

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

---

## Documentation Debt — Website & Manual Updates

The SocNetV website and manual live in a separate public repo at `~/socnetv/website/`
(GitHub: `https://github.com/socnetv/website`). Core pages and manual content are under
`src/content/docs/`.

The following WS9 features are **implemented and shipped in v3.5** but not yet reflected
in the website or manual:

| Feature | Issue | Manual page(s) to update |
|---|---|---|
| Focus on selection (hide non-selected nodes) | #210 | Filtering & exploration section |
| Ego network (k=1) filter | #211 | Filtering & exploration section |
| Edge filtering by weight | #213 | Filtering & exploration section |
| Non-destructive node filter restore (Restore All Nodes) | #216 | Filtering & exploration section |
| Ego-centered radial layout | #214 | Layouts reference |
| Right-click context menu: ego, focus, restore | #209/#211 | Usage / interaction reference |
| Attribute-based filtering (nodes + edges) | #217 | Filtering & exploration section |
| Filter bar with chips | #219 | Filtering & exploration section |
| Edge Properties dialog with custom attributes | #224 | Attributes & metadata section (new) |
| Node/edge data table dock (Ctrl+T) | #225 | Data management section (new) |
| Structured CSV/JSON export | #226 | Data management section, Export reference |
| Structured CSV/JSON import | #227 | Data management section, Import reference |
| Spreadsheet bulk editing workflow | #232 | Data management section (new workflow page) |

**Priority order for documentation:**
1. Data table dock + export/import (most user-facing, no equivalent docs exist)
2. Attribute-based filtering + filter bar (users will search for "how do I filter by attribute")
3. Spreadsheet workflow (new pattern, deserves its own how-to page)
4. Ego network, focus on selection, restore all nodes
5. Remaining layout and edge-weight filtering entries

