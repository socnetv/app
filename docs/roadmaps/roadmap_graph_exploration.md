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

* **#37** — Color nodes by metric: extend the existing Node Color gradient mechanism (currently only covers the 12 centrality/prestige indices in `slotLayoutNodeColorByProminenceIndex`) to also include clustering coefficient and other cohesion measures. The gradient machinery is already in place; the work is adding the missing index types to the mapping options.
  * Note: this is distinct from the existing "Visualize by prominence index → Node Color" layout type, which already maps centrality scores to a blue→red gradient. #37 is about extending the same mechanism to non-prominence metrics (clustering coefficient, community membership, custom numeric attributes).

**Deferred post-3.6:**

* Community-based layouts — requires a community detection algorithm (Louvain / modularity) wired to the layout engine; significant algorithmic work independent of WS9. No dedicated issue exists yet; open one when scoping begins.
* Edge bundling — complex QPainter/GPU rendering concern. No dedicated issue yet.

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
* **Blocked on**: tab-based multi-graph UI (Feature 1 Phase 3, no issue yet) — a significant infrastructure investment. Deferred until the tab UI is designed and implemented.

### Phase 7 — Query System (#221) *(v3.6 target)*

Planned in two phases (both for v3.6):

**Phase 0 — Arbitrary chip removal (prerequisite):**

* Extend `GraphVisibilitySnapshot` with a `FilterSpec` field that records replay parameters (type + condition) for each filter applied.
* Store the spec alongside every push to `m_visibilityHistory`.
* On arbitrary chip removal: drain the stack to the base state, then replay the remaining filter specs in order.
* Enable `×` on all replayable chips (attribute and query types). Selection/ego/centrality chips retain top-only removal because their parameters may have changed since application (selected vertices, computed scores).
* `chipCloseRequested` signal extended to carry chip index so MainWindow can identify which spec to skip during replay.

**Phase 1 — Visual Query Builder:**

* `DialogQueryBuilder` (`src/forms/dialogquerybuilder.*`) — dynamic list of condition rows (scope + key + op + value), Add / Remove row buttons, AND logic (OR deferred as post-3.6 DSL item).
* `GraphQuery` struct (`src/graph/filters/graph_query.h`) — `QList<FilterCondition>` + `Logic` enum.
* `Graph::vertexFilterByQuery(const GraphQuery &)` — intersects visible sets from all conditions; pushes a single compound snapshot carrying a query-type `FilterSpec`.
* Menu: **Edit → Filter → Query Builder…** (`Ctrl+X, Q`).
* Filter bar: one chip `"Query: N conditions ×"`; clicking the label reopens the dialog with current conditions prefilled for editing.

**Deferred from #221:**

* OR logic between conditions.
* Text-based DSL for scripting / CLI tool.
* Arbitrary removal for selection / ego / centrality chips (depends on #31 structural undo or explicit parameter storage for those filter types).

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

#31 is a long-standing user request for general undo on canvas operations. Its relationship to WS9:

* **Filter-level undo** is already implemented via the `m_visibilityHistory` snapshot stack — every non-destructive filter (centrality, ego, selection, attribute) can be undone via "Restore All" or, after Phase 7 Phase 0, via arbitrary chip removal.
* **Structural edit undo** (add/delete nodes, attribute mutations, weight changes) requires a proper `QUndoStack` integrated across the full Graph mutation API. This is a WS3-level concern (domain model split) and is explicitly deferred post-3.6.
* The bulk edit gap (noted in Feature 3 Phase 5) is part of structural undo and similarly deferred.

### Temporal Data (#222)

Future extension — consumer of #221 query infrastructure:

* Date / datetime attributes (ISO 8601); time intervals (start + end).
* Filtering by date ranges and interval overlap (e.g. `start_date ≤ 1900 AND end_date ≥ 1850`) maps directly to the `Lte`/`Gte` operators in `FilterCondition` — implement #221 first and #222 gets basic date range filtering for free.
* Timeline slider UI and network-over-time animation are complex and **deferred post-3.6**.
* No implementation work needed before #221 is complete.

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

### v3.6 completion sequence

The roadmap is largely done. The remaining v3.6 work in priority order:

1. **#235** — Canvas: Shift+click node adds to current selection (multi-select). Small change; enables better use of the subgraph extraction workflow.
2. **#221 Phase 0** — Arbitrary chip removal: extend `GraphVisibilitySnapshot` with `FilterSpec`; enable × on replayable chips; replay on removal.
3. **#221 Phase 1** — `DialogQueryBuilder`: multi-condition composer → one compound snapshot.
4. **#37** — Color nodes by clustering coefficient and other non-prominence metrics (extend the existing gradient machinery).
5. **Documentation debt** — manual updates for #228, #234, #236–#238, #220 (table already tracked at the end of this roadmap).

### Short-term

* Reuse existing functionality
* Refactor incrementally

### Mid-term

* Introduce shared filtering logic
* Decouple UI from graph operations

### Long-term

* Introduce explicit projection layer
* Support multiple graph views (tabs)
* Structural undo / redo (#31) — depends on WS3 domain model split

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

All WS9 features shipped up to and including v3.5 (#209–#227, #232) have been
documented in the manual. No outstanding documentation debt for those issues.

The following features are **implemented in the v3.6 development cycle** (also tracked
with full detail in the roadmap phases above) and will need manual coverage before the
next release:

| Feature | Issue | Roadmap section | Notes |
|---|---|---|---|
| In-app bulk editing of node and edge attributes | #228 | Feature 3 Phase 5 | New Data Table toolbar buttons (Set property, Add attribute, Remove attribute); canvas context menu entries; `DialogBulkEdit`; canvas ↔ table selection sync |
| UI declutter & UX improvements | #234 | Feature 1 Phase 2 + Cross-cutting UX | See notes below |
| GraphViz DOT export & parser roundtrip | #236 | Feature 4 Phase B1 | Manual's "Supported Formats" page needs: describe DOT export, note single-relation limitation, document `pos=` coordinate roundtrip and custom-attribute passthrough; see capability matrix below |
| UCINET DL export | #237 | Feature 2 Phase 5 | Manual's "Supported Formats" page needs: describe DL export, note FULLMATRIX format used, multi-relation support via NM blocks, node-labels preserved in ROW/COLUMN LABELS; see capability matrix below |
| Edge List export (weighted + simple) | #238 | Feature 2 Phase 5 | Manual's "Supported Formats" page needs: describe weighted (source target weight) and simple (source target) variants; note active-relation-only limitation; note spaces in labels are exported as underscores; see capability matrix below |
| Subgraph save format expansion | #220 | Feature 2 Phase 5 | Manual's "Save subgraph" section needs: update to show all 7 formats now available in the Save Subgraph As… dialog; document which formats lose custom attributes (all except GraphML/DOT); document which formats export only the active relation (Adjacency, DOT, both Edge List variants) |

### Export format capability matrix

What each supported export format can preserve from a SocNetV graph.
Use this table when writing manual copy and when deciding which format to recommend to users.

| Format | Node labels | Node colors/shapes | Custom node attrs | Edge weights | Edge labels | Custom edge attrs | Multi-relation |
|--------|:-----------:|:------------------:|:-----------------:|:------------:|:-----------:|:-----------------:|:--------------:|
| GraphML | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| Pajek | ✓ | ✓ | ✗ | ✓ | ✗ | ✗ | ✓ (matrix blocks) |
| Adjacency | ✗ | ✗ | ✗ | optional | ✗ | ✗ | ✗ (active only) |
| GraphViz DOT | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✗ (active only) |
| UCINET DL | ✓ | ✗ | ✗ | ✓ | ✗ | ✗ | ✓ (NM blocks) |
| Edge List (weighted) | ✓ (label col) | ✗ | ✗ | ✓ | ✗ | ✗ | ✗ (active only) |
| Edge List (simple) | ✓ (label col) | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ (active only) |

When a format cannot preserve something the graph contains, the export dialog warns the user before writing the file.

### #234 — UI/UX changes requiring manual & screenshot updates

The following UI areas changed visually and/or behaviourally and need updated
screenshots and text in the manual:

- **Control Panel → Layout section**: Apply buttons are gone. Screenshots showing
  the old Apply buttons must be replaced. Text should explain that selecting from
  any combobox applies the layout immediately; that Type defaults to "None"; and
  that choosing a Force-Directed model clears a Radial/Level type and vice versa.

- **Toolbar**: Filter actions are no longer in a single flat group — node-filter
  icons appear alongside node actions, edge-filter icons alongside edge actions.
  Any toolbar screenshot in the manual needs updating.

- **Node Properties / Edge Properties toolbar buttons**: Behaviour is now
  context-sensitive (hint / single dialog / bulk-edit). The manual section
  describing how to edit node or edge properties should document all three
  states.

- **Statistics Panel**: The panel is now divided into five collapsible sections
  (NETWORK, SELECTION, CLICKED NODE, CLICKED EDGE, DISTRIBUTION), each with a
  ▾/▴ toggle header. The In-Degree/Out-Degree rows and Weight/Reciprocal rows
  are hidden until a node/edge is clicked. Any manual screenshot of the right
  panel needs to be retaken.

