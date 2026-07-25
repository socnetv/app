# Roadmap: Graph Exploration (WS9) ✔

## Overview

This roadmap documents the evolution of SocNetV from a visualization-focused application into a full **graph exploration and data workflow platform**. All three feature tracks shipped in v3.5–v3.6.

* **Feature 1 (#209)**: Visualization & decluttering ✔
* **Feature 2 (#215)**: Filtering, querying & subgraphs ✔
* **Feature 3 (#223)**: Structured data workflows ✔

---

## Core Vision

SocNetV now supports the full workflow:

```
Load → Visualize → Filter → Explore → Extract → Edit → Export
```

---

## Architectural Direction

### State at WS9 start (v3.4)

* Graph tightly coupled with UI (MainWindow)
* Operations dialog-driven
* No unified filtering or projection layer

### Achieved Architecture

```
Graph (data)
    ↓
Filter / Projection Layer  (m_visibilityHistory snapshot stack)
    ↓
UI (GraphicsWidget, dialogs, tables, filter bar, data table dock)
```

### Key Principles (applied throughout)

* **Non-destructive operations** (visibility instead of deletion)
* **Stateful filtering** (snapshot/restore stack, not one-off dialogs)
* **Reusable logic** across UI components (`FilterCondition`, `FilterSpec`, `GraphQuery`)

---

## Constraints

* Single-window application (Qt MainWindow)
* Graph is currently owned by MainWindow
* No multi-document interface (MDI)

### Design Decision (v3.6)

* Subgraphs are handled as **filtered views (in-place)**
* No new windows for subgraphs — tab-based multi-graph UI is the preferred long-term direction (see WS9 Debt)

---

## Feature Breakdown

---

# Feature 1 — Visualization & Decluttering (#209) ✔

## Goal

Make large graphs readable and explorable.

## Phases

### Phase 1 — Immediate UX (#209) ✔

* ✔ Focus on selection (#210) — `Graph::vertexFilterBySelection()`, `filterNodesBySelectionAct` (Ctrl+X, Ctrl+S)
* ✔ Ego networks (k=1) (#211) — `Graph::vertexFilterByEgoNetwork()`, `filterNodesByEgoNetworkAct` (Ctrl+X, Ctrl+F)
* ✔ Hide non-selected nodes (#212) — closed as duplicate of #210
* ✔ Edge filtering by weight (#213) — `Graph::edgeFilterByWeight()`, dialog + `Graph::edgeFilterReset()`, `editFilterEdgesRestoreAllAct` (Ctrl+E, Ctrl+R)

**Cross-cutting UX (Phase 1):**
* ✔ Non-destructive node filter restore — `Graph::vertexFilterRestoreAll()`, `filterNodesRestoreAllAct` (Ctrl+X, Ctrl+R)
* ✔ Right-click on node auto-selects it before context menu opens (`GraphicsWidget::mousePressEvent`)
* ✔ Ego network + Focus on Selection + Restore All Nodes wired into node right-click context menu

### Phase 2 — Layout Improvements (#214, #234) ✔

* ✔ Improved force-directed layout 
* ✔ Ego-centered radial layout (#214) — `Graph::layoutEgoRadial()`, Layout menu (`Ctrl+Alt+E`) and node right-click context menu
* ✔ Remove Apply buttons from Layout panel (#234) — all three comboboxes (Prominence Index, Type, Force-Directed Model) apply immediately on change; Type defaults to "None"; selecting a Force-Directed model resets Type to None and vice versa

**Cross-cutting UX (#234):**
* ✔ Toolbar filter actions regrouped — node-filter icons sit alongside node actions; edge-filter icons alongside edge actions
* ✔ Node Properties / Edge Properties toolbar actions are selection-aware — nothing selected → status-bar hint; exactly one node/edge → single-item dialog; multiple selected → `DialogBulkEdit`
* ✔ Statistics Panel sectioned into five collapsible groups (▾/▴ toggle): NETWORK, SELECTION, CLICKED NODE, CLICKED EDGE, DISTRIBUTION — each section collapses/expands independently; In-Degree/Out-Degree rows and Weight/Reciprocal rows auto-hide until a node/edge is clicked
* ✔ Left Control Panel wrapped in `QScrollArea` — panel scrolls rather than overlapping when the Data Table dock reduces available vertical space

### Phase 3 — Advanced Visualization

**Scoped deliverable for v3.6:**

* ✔ **#37** — Color nodes by metric: `IndexType::CLC = 13` added to the prominence index enum; `clusteringCoefficient()` now populates `discreteCLCs` via `resolveClasses`; `layoutByProminenceIndex`, `isCentralityIndexComputed`, `vertexFilterByCentrality`, `prominenceDistribution`, and the vertex find/filter switches all handle `CLC`; `layoutNodeColorProminence_CLC_Act` (`Ctrl+L, Ctrl+C, Ctrl+G`) added to the Node Color menu and `prominenceIndexList`. Clustering Coefficient now appears in the Layout control panel combo, the analysis combo, Filter Nodes by Centrality, and the distribution chart — identical integration depth as the 12 centrality/prestige indices.

**Deferred post-3.6:**

* Community-based layouts — requires a community detection algorithm (Louvain / modularity) wired to the layout engine; significant algorithmic work independent of WS9. Tracked as #258.
* Edge bundling — complex QPainter/GPU rendering concern. Tracked as #259.

---

# Feature 2 — Filtering & Subgraphs (#215) ✔

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

### Phase 4 — Subgraph Extraction (#218) ✔

* ✔ Save visible nodes as subgraph — `Graph::subgraphExtract()`, **Edit → Subgraphs → Save visible nodes as subgraph…** (always enabled)
* ✔ Save selected nodes as subgraph — `Graph::subgraphExtractFromSelection()`, **Edit → Subgraphs → Save selected nodes as subgraph…** (enabled when ≥ 1 node selected)
* ✔ Shared extraction logic in private `Graph::subgraphFromVertexList()`: vertices renumbered from 1; all relations mirrored; custom node/edge attributes preserved; canvas dimensions propagated via `canvasSizeSetQuiet()` so GraphML export produces correct normalized coordinates
* Output: save to GraphML file (preserves all attributes and relations)
* Deferred: open subgraph in new window/tab — preferred long-term direction is tab-based multi-graph UI (Phase 6)

### Phase 5 — Export Filtered / Extracted Graph (#220) ✔

* ✔ Export the currently visible (filtered) subset to any supported format — **Network → Export** menu now supports GraphML, Pajek, Adjacency, GraphViz DOT, UCINET DL, Weighted Edge List, and Simple Edge List (#236, #237, #238)
* ✔ Save a named subgraph to file for later reload — **Edit → Subgraphs → Save visible / Save selected** dialog now offers all seven formats with format-aware fidelity warnings (#218, #220)
* ✔ Basis for save/load subgraph workflows established (see Phase 6 for persistent named subgraphs)

### Phase 6 — Persistent Named Subgraphs *(deferred post-3.6)*

* Maintain multiple named subgraph views derived from the same base graph
* Switch between subgraphs without reloading
* Save and reload named subgraphs (persisted alongside or inside the graph file)
* **Blocked on**: tab-based multi-graph UI (#245) — a significant infrastructure investment. Deferred until the tab UI is designed and implemented.

### Phase 7 — Query System (#221) ✔

Both phases shipped for v3.6.

**Phase 0 — Arbitrary chip removal ✔:**

* `FilterSpec` struct (`src/graph/filters/filter_spec.h`) — replay descriptor embedded in every `GraphVisibilitySnapshot`; types: `Attribute`, `Selection`, `Ego`, `Centrality`, `EdgeAttribute`, `EdgeWeight`, `Query`, `EdgeQuery`.
* Every filter (node AND edge) pushes exactly one snapshot with a fully populated spec, establishing a strict `barIndex == stackIndex` invariant.
* `FilterCondition::matches()` inline method — shared by all filter/query implementations; eliminates duplicated matching logic.
* `Graph::vertexFilterRemoveAt(int stackIndex)` — drain stack → restore base → replay all remaining specs; handles mixed node/edge stacks correctly.
* `Graph::vertexFilterReplaySpec()` — dispatch table covering all eight spec types including `EdgeAttribute` and `EdgeWeight`.
* `edgeFilterByWeight()` now snapshot-backed (was non-destructive UI reset only before).
* `chipCloseRequested` now carries `(barIndex, scope)`; MainWindow uses `barIndex` directly as `stackIndex`.
* `m_filterChips: QList<QPair<QString,Scope>>` in MainWindow — unified chip tracking replaces separate `m_nodeFilterChips` + `m_edgeFilterChipLabel`; bar rebuild preserves original application order.
* `slotFilterNodesRestoreAll` and `slotEditFilterEdgesReset` now use `vertexFilterRemoveAt` (no more orphaned snapshots when edge chips are removed).

**Phase 1 — Visual Query Builder ✔:**

* `GraphQuery` struct (`src/graph/filters/graph_query.h`) — `QList<FilterCondition>`; all conditions carry the same scope (set by dialog).
* `Graph::vertexFilterByQuery(const GraphQuery &)` — AND logic for nodes; pushes single compound snapshot with `FilterSpec::Type::Query`.
* `Graph::edgeFilterByQuery(const GraphQuery &)` — AND logic for edges; pushes single compound snapshot with `FilterSpec::Type::EdgeQuery`.
* `DialogQueryBuilder` (`src/forms/dialogquerybuilder.{h,cpp}`) — pure-C++ dialog (no .ui), dynamic condition rows (key combo / op combo / value edit / − button), scope radio (Nodes / Edges), "Add condition" button, Apply. Opens with one row; at least one row always kept.
* Menu: **Edit → Filter → Query Builder…** (`Ctrl+X, Ctrl+B`).
* Filter bar: one chip `"Nodes: query (N condition(s)) ×"` or `"Edges: query (N condition(s)) ×"`; arbitrary removal via the unified replay stack.

**Deferred from #221:**

* OR logic between conditions.
* Text-based DSL for scripting / CLI tool.
* Clicking a query chip label to reopen the dialog with current conditions prefilled.
* Arbitrary removal for selection / ego / centrality chips (depends on #31 structural undo or explicit parameter storage for those filter types).

---

# Feature 3 — Data Workflows (#223)

## Goal

Treat graphs as structured datasets.

---

## Phases

### Phase 1 — Attribute Editing ✔

* ✔ Improve node/edge attribute editing (#224)
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
  * `QDockWidget` at `BottomDockWidgetArea`; toggled by **Ctrl+D** (`Options`
    menu and `Edit` menu, `viewDataTableAct`). Auto-refreshes on file load and
    graph reset when visible.
  * `viewDataTableAct` has a dedicated `data_table_48px.svg` icon.

### Phase 3 — Structured Export ✔

* ✔ CSV / JSON export (#226) 
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

### Phase 5 — Bulk Editing (#228) ✔

**Goal:** in-app same-value-to-many-targets operations — complementary to the spreadsheet workflow (#232) which already handles heterogeneous per-row editing.

* ✔ `DialogBulkEdit` (`src/forms/dialogbulkedit.*`) — adaptive stacked-widget dialog; scope-dependent property combo (Label, Size, Color, Shape for nodes; Label, Weight, Color for edges) plus all existing custom attribute keys; value widget switches per property type (QLineEdit, QSpinBox, QDoubleSpinBox, color picker, shape combo).
* ✔ **Set property** on selected/visible nodes — `GraphTableWidget::onNodeSetPropertyClicked`; routes built-in properties to `vertexLabelSet` / `vertexSizeSet` / `vertexColorSet` / `vertexShapeSet`; custom keys to `vertexCustomAttributeSet`. Also available from canvas context menu via `MainWindow::slotEditNodeSetPropertyForSelection`.
* ✔ **Set property** on selected/visible edges — `GraphTableWidget::onEdgeSetPropertyClicked`; routes to `edgeLabelSet` / `edgeWeightSet` / `edgeColorSet` / `edgeCustomAttributesSet`. Canvas shortcut: `slotEditEdgeSetPropertyForSelection`.
* ✔ **Add attribute** — `onNodeAddAttributeClicked` / `onEdgeAddAttributeClicked`; two `QInputDialog` prompts (key, value); calls `vertexCustomAttributeSet` / `edgeCustomAttributesSet` for all targets.
* ✔ **Remove attribute** — `onNodeRemoveAttributeClicked` / `onEdgeRemoveAttributeClicked`; collects unique keys across targets, offers `QInputDialog::getItem`, calls `vertexCustomAttributeRemove` (nodes) or get-remove-set pattern (edges, no dedicated API).

**Selection sources (all three implemented):**
* Canvas selection → table sync via `slotCacheSelection` → `syncNodeSelection` / `syncEdgeSelection` (O(model rows + selected), always runs even when dock is hidden).
* Filtered/visible subset — `resolveNodeTargets` / `resolveEdgeTargets` fall back to all proxy-visible rows when no table rows are explicitly selected (integrates with #215 filter stack).
* Multi-row table selection — `ExtendedSelection` on both views; `Ctrl+click` and drag-select supported.

**Canvas ↔ table sync:**
* `GraphicsWidget::userSelectedItems` → `slotCacheSelection`: auto-switches Data Table to Nodes tab unless selection is *entirely* edges; syncs selection into table even when dock is hidden so bulk operations target the correct rows when the user later opens the dock.
* `GraphTableWidget::refresh()` re-applies `graph->getSelectedVertices()` / `getSelectedEdges()` after each model reset (model reset clears all view selections).
* Clicking a node row emits `nodeSelected(int)`; clicking an edge row emits `edgeSelected(int, int)` — both wired to the status bar.
* Context menu: "Edit Selection in Data Table" (≥ 1 selected item) and "Set property for selection…" wired for both node and edge selections.
* Data Table emptied on `initApp()` (new network or close) regardless of dock visibility.

**Known gap — undo:** bulk operations bypass the undo stack. Undo support for attribute mutations is a broader infrastructure gap tracked under #224 (attribute system) and is deferred until a general undo/redo architecture is introduced.

### Phase 6 — Transformations (#229) *(deferred post-3.6)*

* Derived fields: compute a new attribute value from one or more existing attributes (e.g. `full_name = first + " " + last`)
* Value normalization: min-max or z-score scaling of a numeric attribute across all nodes/edges
* Type coercion: convert stored string values to canonical types (integer, float, boolean) — useful before filtering with numeric operators
* **Rationale for deferral**: the CSV/JSON import-export workflow (Phases 3+4) already covers the practical need — users can export to CSV, transform freely in any spreadsheet tool, and re-import. In-app derived fields are a data-platform concern beyond SocNetV's core identity.

---

## Cross-Cutting Systems

### Attribute System (#96) ✔

Foundation for:

* filtering
* editing
* export/import

### Metadata System (#130) ✔

Defines:

* ingestion
* persistence
* usage of attributes

### Undo / Redo (#31)

#31 is a long-standing user request for general undo on canvas operations.

* **Filter-level undo** ✔ — implemented via the `m_visibilityHistory` snapshot stack; every non-destructive filter can be undone via "Restore All" or arbitrary chip removal (×).
* **Structural edit undo** (add/delete nodes, attribute mutations, weight changes) — requires a proper `QUndoStack` across the full Graph mutation API; WS3-level concern, deferred post-3.6 (see WS9 Debt).

### Temporal Data (#222)

Deferred post-3.6 (see WS9 Debt). The `Lte`/`Gte` operators in `FilterCondition` (shipped in #221) already support date-range queries once attributes carry typed date values — the query infrastructure is ready; the timeline animation layer is what remains.

---

## UI Evolution

### v3.5–v3.6 (shipped)

* Filter bar (persistent, chip-based) ✔
* Data table dock (node + edge, live-search) ✔
* Dialog-driven filtering and attribute editing ✔

### Post-3.6 (deferred — see WS9 Debt)

* Persistent filter panel dock
* Attribute inspector panel
* Tab-based multi-graph UI (#245)

---

## Outcome

WS9 is **complete**. SocNetV has evolved into:

* Graph visualization tool ✔
* Graph analysis tool ✔
* Graph exploration platform ✔
* Graph data workflow tool ✔

---

## WS9 Debt — Deferred Items

Items explicitly deferred out of WS9 scope. Each has a home in the roadmap phase that generated it.

### Filtering & Query (#221)
* **OR logic** between conditions in the Query Builder — `GraphQuery` currently evaluates all conditions as AND. OR requires a mode toggle in `DialogQueryBuilder` and a separate evaluation path in `vertexFilterByQuery` / `edgeFilterByQuery`.
* **Text-based query DSL** — scripting / CLI tool interface for query filters; depends on the CLI regression tool infrastructure.
* **Chip-label click to reopen dialog** — clicking a query or attribute chip label should reopen the originating dialog pre-filled with the saved conditions from the `FilterSpec`.
* **Arbitrary removal for selection / ego / centrality chips** — these filter types do not store enough parameters in `FilterSpec` to replay after removal; requires either `#31` structural undo or explicit parameter storage per type.

### Subgraphs & Multi-graph UI (#215 Phase 6)
* **Persistent named subgraphs** — maintain multiple named derived views from the same base graph; switch without reloading; persist alongside or inside the graph file. **Blocked on tab-based multi-graph UI** (#245).
* **Tab-based multi-graph UI** (#245) — preferred long-term direction over multiple windows; significant infrastructure investment.

### Layouts (Feature 1 Phase 3)
* **Community-based node coloring** — color by community membership (Louvain / modularity); requires a community-detection algorithm wired to the layout engine. Tracked as #258.
* **Edge bundling** — complex QPainter/GPU rendering concern. Tracked as #259.

### Data Workflows (Feature 3 Phase 6)
* **In-app derived fields** — compute a new attribute from existing ones (e.g. `full_name = first + " " + last`); value normalization (min-max, z-score); type coercion. Deferred: the CSV/JSON roundtrip workflow already covers the practical need via external spreadsheet tools.

### Infrastructure
* **Structural undo / redo (#31)** — general `QUndoStack` across the full Graph mutation API; WS3-level concern (architecture & performance), explicitly deferred post-3.6. Filter-level undo is already covered by the `m_visibilityHistory` snapshot stack.
* **Temporal data (#222)** — date/datetime attributes, interval filtering, timeline slider, network-over-time animation. The `Lte`/`Gte` operators in `FilterCondition` already support date-range queries once attributes are typed; the animation layer is the deferred part.
* **Attribute inspector panel** and **persistent filter panel** as docked widgets — currently dialog-driven; full panel approach deferred until the UI decomposition (WS7) is underway.

---