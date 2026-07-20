# Changelog

All notable changes to this project are documented in this file. 

## [3.7] – Aug 2026

### New Features

  - **Geodesic distance distribution report** (#89): new **Analyze → Cohesion →
    Geodesic Distribution** action (also in the Control Panel combo) writes an
    HTML table of pair-count vs. integer distance bucket across all node pairs.
    Computation is cache-aware: reuses the APSP result when distances have
    already been calculated in the same session.

  - **Shortest path reconstruction and canvas highlighting** (#139): when
    computing the geodesic distance between two specific nodes (**Analyze →
    Cohesion → Distance**), the dialog now shows the full sequence of
    intermediate nodes (e.g. *A → C → D → B*) in addition to the distance
    value. BFS is used for unweighted graphs, Dijkstra for weighted ones.
    The path edges are simultaneously **selected on the canvas** — exactly as
    if the user had Shift-clicked each one — unlocking move, inspect, and
    context-menu operations on the whole path. Only edges are selected (not
    nodes) to avoid highlighting unrelated connected edges.

  - **Tomita pivot selection in clique census** (#64): the Bron–Kerbosch
    algorithm now selects a pivot vertex $ u \in P \cup X $ that maximises
    $ |N(u) \cap P| $ before each recursive level, and iterates only over
    $ P \setminus N(u) $. This can reduce branch count to a single candidate
    per level on dense graphs, giving dramatic speedups on real-world networks
    without changing the set of maximal cliques reported.

### Bug Fixes

  - **HCA uses BFS on unweighted networks** (#193): the Hierarchical Clustering
    Analysis dialog always ran Dijkstra (weighted distances), even on unweighted
    graphs. `considerWeights` is now derived from `activeGraph->isWeighted()`,
    so BFS is used on unweighted networks, matching the behaviour of all other
    distance-based analyses.

  - **Bezier curves toggle now applied at startup** (#246): edges always
    rendered as straight lines regardless of the saved setting in
    **Settings → Edge**. The toggle is now correctly applied on first load.

  - **Remove misleading "session only" labels** (#247): several preferences
    in the Edit and Options menus were annotated "session only" despite being
    persisted between sessions. Labels removed.

  - **Preserve aspect ratio for custom node images** (#122): non-square
    images set as node icons via **Edit → Node → Change shape → Custom**
    were stretched to fill the node bounding box. They are now scaled with
    `Qt::KeepAspectRatio` and centred within the node.

  - **Canvas zoom anchor stabilisation** (partial, #248): zoom in/out via
    slider or keyboard no longer causes the network to drift or vanish — the
    view re-centres on the content bounding box after each zoom step. The
    **Reset** button now works in a single click. Pan-position preservation
    across zoom steps is deferred to #248.

### Maintenance

  - WS3 roadmap renamed from "Domain Model Split" to **"Architecture &
    Performance"** (`docs/roadmaps/roadmap_architecture_performance.md`);
    title now reflects both the structural refactoring and the 2.7×–8.3×
    parallelisation speedup shipped in v3.6.
  - `docs/README_DEVELOPER_NOTES.md` "Current focus" section replaced with a
    single pointer to the architectural roadmap.
  - `AUTHORS`: added Andreas as Debian package maintainer.

## [3.6] – May 26, 2026

### New Features

  - **UCINET DL export** (#237):
    - New **Network → Export → DL…** action writes the current graph to a
      UCINET DL file in `FULLMATRIX DIAGONAL PRESENT` format.
    - Multi-relation graphs are fully supported: all relations are written as
      consecutive matrices in the `DATA:` section under a single `NM=k`
      header (no file-level separator needed — the parser already advances
      its relation counter when the source row counter exceeds N).
    - Node labels are exported in `ROW LABELS` / `COLUMN LABELS` sections,
      enabling label-preserving roundtrip.
    - `FileType::UCINET` added to `m_graphFileFormatExportSupported`; the
      CLI io_roundtrip kernel now exercises the format automatically.  Two
      existing baselines (`Bernard_Killworth_Fraternity__FT5`,
      `StokmanZiegler_Netherlands__FT5`) updated from `performed: false` to
      `performed: true, equivalent: true`.

  - **Edge List export — weighted and simple** (#238):
    - New **Network → Export → List…** action writes the active relation as a
      space-separated edge list file.
    - When the graph is weighted the user is asked whether to include weights:
      **Yes** → `source target weight` per line (`.wlst`),
      **No** → `source target` per line (`.lst`).
    - Node labels are used as identifiers; spaces in labels are replaced with
      underscores to keep the delimiter unambiguous.
    - A warning is shown when multiple relations are present (only the active
      relation is written).
    - Both `FileType::EDGELIST_WEIGHTED` and `FileType::EDGELIST_SIMPLE`
      added to the supported export list; three Tiny edge-list io_roundtrip
      baselines show `performed: true, equivalent: true`.

  - **GraphViz DOT export with full roundtrip** (#236):
    - New **File → Export → GraphViz DOT…** action writes a `.dot` file for
      the active relation (DOT has no native multi-relation concept; a warning
      is shown when more than one relation is present).
    - Export preserves: node label, color, shape, canvas coordinates
      (`pos="x,y"` — compatible with neato/fdp), and all custom node/edge
      attributes as DOT key=value pairs.
    - Edge weight, label, color, and custom attributes are written per edge.
      Undirected graphs suppress the reverse half of each symmetric pair.
    - Parser extended to read back everything that is written:
      `readDotProperties` now returns unknown attributes as a
      `QHash<QString,QString>` rather than discarding them; `pos=` is
      converted to node canvas coordinates; graph name quote-stripping fixes
      the roundtrip for quoted identifiers (`digraph "Name" { … }`).
    - `FileType::GRAPHVIZ` added to the supported export list so the CLI
      io_roundtrip regression kernel exercises the format automatically.

  - **Subgraph save format expansion** (#220):
    - The **Save Subgraph As…** dialog (reached via **Edit → Subgraphs →
      Save visible / Save selected**) now offers all seven supported export
      formats: **GraphML**, **Pajek**, **Adjacency**, **GraphViz DOT**,
      **UCINET DL**, **Weighted Edge List**, and **Simple Edge List**.
    - Format-aware fidelity warnings fire automatically before writing:
      - Custom node/edge attributes: a confirmation prompt is shown for any
        format that cannot preserve them (all formats except GraphML and DOT).
      - Single-relation-only formats (Adjacency, DOT, both Edge List variants)
        show an info notice when the subgraph has more than one relation.
    - Previously the dialog only offered GraphML, Pajek, and Adjacency.

  - **Subgraph extraction** (#218):
    - New **Edit → Subgraphs** submenu with two actions:
      - **Save visible nodes as subgraph…** — extracts all nodes currently
        visible on the canvas (i.e. not hidden by any filter) and the edges
        that run between them into an independent graph file. Always enabled
        when a network is loaded.
      - **Save selected nodes as subgraph…** — extracts the currently selected
        nodes and their inter-edges. Enabled when at least one node is selected.
    - Both actions prompt for a network name, pre-fill the save dialog filename
      from that name, and save to GraphML (which preserves all custom attributes
      and relations).
    - Vertices are renumbered from 1 in the extracted graph; all visual
      properties, custom node/edge attributes, and multi-relation structure
      are preserved.
    - Original graph remains unchanged (non-destructive).

  - **Compound AND-logic query filter** (#221):
    - New **Filter → Filter by Query…** action (`Ctrl+X, Ctrl+B`) opens the
      **Query Builder** dialog — a dynamic multi-condition filter composer
      that supports both nodes and edges.
    - The user picks a scope (Nodes or Edges), then adds one or more condition
      rows. Each row specifies an attribute key, an operator (=, ≠, >, <, ≥,
      ≤, contains), and a value. All rows are ANDed: only nodes/edges that
      satisfy every condition are kept visible.
    - Rows can be added ("+  Add condition" button) or removed (per-row "−"
      button; the last row's remove button disables itself automatically).
      The attribute key combo is populated from the graph's current node or
      edge attribute keys and is also freely editable.
    - Applies the same non-destructive snapshot/restore mechanism as all other
      filters: the result appears as a chip in the filter bar and can be
      individually removed or cleared with Restore All.
    - Numeric comparisons (>, <, ≥, ≤) automatically downcast to `double`
      when both sides are valid numbers; string comparisons fall back to
      case-insensitive lexicographic order; "contains" is always
      case-insensitive substring search.

  - **Connected components count and color-by-component layout** (#85):
    - **Analyze → Cohesion → Connectedness** now reports the number of weakly
      connected components when the network is disconnected, alongside a hint
      pointing to the new colorize action.
    - New **Layout → Node Color by Connected Component**
      (`Ctrl+L, Ctrl+C, Ctrl+0`) colors every node by its weakly connected
      component: nodes in the same component share the same color, making
      isolated sub-networks immediately visible. Up to 15 visually distinct
      palette colors are used; the palette cycles for networks with more
      components. If the network is already fully connected the action reports
      "one component" and leaves colors unchanged.
    - Both directed and undirected networks are supported. Weak connectivity is
      used throughout: two nodes are in the same component when there is an
      undirected path between them (edge directions are ignored). This is
      consistent with Gephi, igraph, and NetworkX defaults and answers the
      practical "how many disconnected islands?" question for both graph types.
      Strong connectivity (all-pairs directed reachability) remains available
      via the SSSP engine.
    - `Graph::graphWeaklyConnectedComponents()` implements the BFS, caches the
      count in `m_graphWeaklyConnectedComponents` and per-node component IDs in
      `m_vertexComponentId`. Cache is invalidated whenever the graph is
      structurally modified (same trigger as distances / centralities).
    - **Connectivity semantics** — what the app computes and reports:

      | Graph type | Topology | Components | Connected? | UI message |
      |---|---|---|---|---|
      | Undirected | All nodes reachable | 1 | yes | "connected (1 component)" |
      | Undirected | Isolated sub-networks | >1 | no | "disconnected (N components)" |
      | Directed | Every pair has a directed path | 1 | yes | "weakly connected (1 component)" |
      | Directed | A→B only, not B→A | 1 | yes | "weakly connected (1 component)" — one island, not strongly connected |
      | Directed | Two separate islands | >1 | no | "disconnected (N weakly connected components)" |

      The "connected" determination uses weak connectivity (`components == 1`)
      throughout, for both directed and undirected networks. Weak connectivity
      ignores edge direction and answers the practical "how many disconnected
      islands?" question consistently. Strong connectivity (all-pairs directed
      reachability) is a separate, stricter property available via the geodesic
      distances computation.

  - **Clustering Coefficient added to node color layout** (#37):
    - **Layout → Node Color by prominence index → Clustering Coefficient**
      (`Ctrl+L, Ctrl+C, Ctrl+G`) colors nodes by their local Watts-Strogatz
      clustering coefficient using the same blue→red gradient as all other
      prominence indices: nodes with higher clustering are warmer (red), lower
      clustering are cooler (blue).
    - Fully integrated at the same depth as the 12 existing centrality/prestige
      indices: appears in the Layout control panel combo, the analysis prominence
      combo, **Filter Nodes by Centrality** (filter and find by CLC threshold),
      and the prominence distribution chart.
    - `IndexType::CLC = 13` added to the index enum; `clusteringCoefficient()`
      now builds the `discreteCLCs` distribution map for charting.

  - **Zero-weight edge display settings** (#30):
    - New **Settings → Edges → Show zero-weight edges** checkbox (default: on):
      when unchecked, zero-weight edges are silently skipped on load — no ghost
      edges, no artifacts. The **Zero valued edge color** picker is automatically
      disabled when the checkbox is off.
    - Zero-weight edge color is now **user-configurable**: the hardcoded `"blue"`
      is replaced by `initEdgeColorZero`, driven by the existing
      **Settings → Edges → Zero valued edge color** picker which was already
      present but disconnected from `edgeCreate()`.

  - **Configurable edge arrow size** (#32):
    - New **Settings → Edges → Arrow size** spinbox (range 2–20, default 6)
      lets the user control the size of arrowheads on directed edges.
    - The change applies to all edges on the canvas immediately and is
      persisted for future sessions.

### Improvements

  - **SSSP parallelisation — major performance boost for large networks** (#241, #242):
    - All shortest-path computations (distances, betweenness centrality, stress
      centrality, closeness, eccentricity, power centrality) now run concurrently
      across all available CPU cores via `QtConcurrent::blockingMap`.
    - **Phase 1** (#241): per-source SSSP scratch state (BFS stack, predecessor lists,
      dependency accumulators, distance and sigma arrays, nth-order neighbourhood map)
      was extracted from `Graph` and `GraphVertex` into a self-contained
      `PerSourceScratch` struct. Replacing per-edge `QHash` lookups with contiguous
      `QVector` index reads improved cache locality and gave an immediate 14–23%
      speedup even on a single thread.
    - **Phase 2** (#242): the source loop in `DistanceEngine::runAllSources()` is now
      distributed across all CPU cores. Each worker thread owns a `ThreadLocalState`
      holding its own `PerSourceScratch`, partial BC/SC accumulator arrays, and running
      totals for graph-wide aggregates. A single-threaded reduction step after the loop
      merges everything into graph state with zero contention. APSP write-back is
      race-free without any mutex because each source vertex is processed by exactly
      one thread.
    - **Measured speedup** (Debug build, 24-core Linux):

      | Network | Centralities | Before | After | Speedup |
      |---------|-------------|--------|-------|---------|
      | BA directed N=500 E=1219 | ON | 679 ms | 255 ms | **2.7×** |
      | N=1000 A=10000 | OFF | 28 423 ms | 3 431 ms | **8.3×** |
      | N=1000 A=10000 | ON | 47 020 ms | 5 949 ms | **7.9×** |

      Speedup scales near-linearly with core count because sources are fully
      independent once per-source scratch state is thread-local.
    - All 36 golden regression baselines pass; numeric results are bit-for-bit
      identical to the sequential implementation.

  - **Canvas rendering performance for large networks** (#180, #240):
    - The dominant bottleneck on large networks (2000+ nodes, 40000+ edges)
      was `QGraphicsScene::BspTreeIndex` (the previous default): every
      `prepareGeometryChange()` call — triggered O(E) times per rubber-band
      drag or CTRL+A — paid an O(log N) BSP tree update. Result: selection
      and drag were nearly unresponsive on large networks.
    - **Scene index now defaults to `NoIndex`**, eliminating BSP overhead
      entirely. Rubber-band drag, CTRL+A, and full-network moves are now
      fast on large networks. Memory consumption is also reduced
       since the BSP tree structure is not built.
    - Existing users are migrated automatically on first launch
      (`settingsMigration = 2`); the setting remains user-controllable via
      **Settings → Canvas → Scene index method**.
    - `QPen` and `QBrush` objects in `GraphicsEdge` and `GraphicsNode` are
      now cached and rebuilt only on property changes, avoiding per-`paint()`
      heap allocations.
    - Nine bulk visibility-toggle methods (`setNodeLabelsVisibility`,
      `setEdgeArrowSize`, `setEdgeHighlighting`, etc.) now wrap their loops
      with `viewport()->setUpdatesEnabled(false/true)`, coalescing per-item
      `update()` calls into a single viewport repaint.
    - `scene()->items()` loops replaced with direct `edgesHash`/`nodeHash`
      iteration in `setEdgeOffsetFromNode` and `setAllItemsVisibility`.
    - Hot-path `qDebug` calls removed from `handleSelectionChanged`,
      `selectedNodes`, and `selectedEdges` (fired on every selection event).

  - **UI declutter & UX improvements** (#234):
    - Removed the two "Apply" buttons from the Layout section of the Control
      Panel. All three layout comboboxes (Prominence Index, Type, Force-Directed
      Model) now apply immediately on selection change, with no extra click
      required.
    - Layout Type defaults to "None" (was "Radial") so opening the panel no
      longer implies a radial layout is pending.
    - Mutual exclusion enforced between By-Prominence-Index (Radial/Level types)
      and Force-Directed Model: selecting a force-directed model resets the Type
      to None, and vice versa.
    - Toolbar filter actions regrouped: node-filter icons sit alongside other
      node actions; edge-filter icons sit alongside edge actions.
    - Node Properties and Edge Properties toolbar actions are now
      selection-aware: nothing selected → status-bar hint; exactly one
      node/edge → single-item dialog; multiple selected → bulk-edit dialog.
    - Statistics Panel sectioned into five collapsible groups (▾/▴ toggle):
      NETWORK, SELECTION, CLICKED NODE, CLICKED EDGE, DISTRIBUTION. Each
      section collapses and expands independently.
    - In-Degree/Out-Degree rows in the Clicked Node section and
      Weight/Reciprocal rows in the Clicked Edge section auto-hide until
      a node or edge is clicked.
    - Left Control Panel wrapped in a scroll area: when the Data Table dock
      reduces available vertical space the panel scrolls rather than
      overlapping its widgets.

  - **Better feedback during long operations**: the status bar now shows a
    "Computing … Please wait…" message for every analysis and layout operation
    that can take noticeable time (centrality measures, prestige indices,
    matrix computations, structural equivalence, force-directed and
    prominence-index layouts, and more). The message is painted to the screen
    immediately, so users always know the app is working.

### Bug Fixes

  - **UCINET DL two-mode networks now load correctly** (#63): three bugs in
    `parser_dl.cpp` caused two-mode (NR × NC affiliation) files to silently
    produce wrong node counts, wrong node numbers, and misrouted labels.
    Fixed: (1) `totalNodes` is now set to `NR + NC` when two-mode is detected;
    (2) row and col nodes are created with explicit sequential numbers (1..NR
    and NR+1..NR+NC) instead of a broken auto-numbered batch; (3) `COL LABELS:`
    is recognised as an alias for `COLUMN LABELS:`. Two golden IO-roundtrip
    baselines added (`TinyDL_TwoMode_3x3_labeled`, `TinyDL_TwoMode_3x3_nolabels`).
    **Note:** UCINET two-mode files always load as a bipartite directed graph
    in SocNetV (directed edges from mode-1 row nodes to mode-2 column nodes).
    Projection to person-network or event-network is not yet supported for the
    DL format; it is tracked as a separate feature request.

  - **Galaskiewicz famous network now loads correctly** (#15): the `.2sm`
    data file was missing from `data.qrc`, so `writeFamousNetwork()` returned
    early with an empty temp file — causing "no data rows found" for all three
    import modes. Added the resource entry and removed the now-dead hardcoded
    data rows from `graph_reports.cpp`.

  - **Edge mode now follows relation switches** (#53): switching to a
    different relation now restores that relation's directed/undirected
    state. Each relation tracks its own flag in `m_relationsDirected`;
    `relationSet()` saves the departing state and restores the incoming
    one, recomputes `m_graphIsSymmetric` from actual edges, and emits
    `signalGraphDirectedChanged` so the edge-mode combo and the arrows
    action in the toolbar update without mutating any edges.

  - **Zero-weight edges: five pre-existing computation bugs fixed** (#30):
    - `outEdgesCount()` now skips weight-0 edges → **density** no longer
      overcounted.
    - `graphReciprocity()` now skips weight-0 edges, preventing a false
      `edgeExists(v2,v1)==0` match against absent reverse edges that inflated
      the reciprocated-ties count and could produce a 0/0 arc-reciprocity ratio.
    - `clusteringCoefficientLocal()` now excludes weight-0 neighbours from the
      neighbourhood *k*, keeping the k*(k−1) denominator correct.
    - BFS (`bfsSSSP`) now skips weight-0 edges so they are not traversed as valid
      1-hop connections (affected geodesic distances, reachability, and all
      BFS-derived centralities).
    - Dijkstra (`dijkstraSSSP`) now skips weight-0 edges, preventing free paths
      (`dist_w = dist_u + 0`) and a division-by-zero crash when `inverseWeights`
      is enabled (`1/0 = ∞`).

  - **Canvas: Shift+click now adds a node to the current selection** (#235):
    Shift+left-clicking a node toggles it into or out of the selection without
    clearing previously selected nodes. Qt's item-level mouse handler in
    `RubberBandDrag` mode only treats `Ctrl` as additive; the fix intercepts
    `Shift+LeftButton` in `GraphicsWidget::mousePressEvent` and calls
    `node->setSelected(!node->isSelected())` directly, bypassing Qt's
    default clear-and-select path. Shift+click on empty canvas space (rubber-
    band extension) and plain click behaviour are unchanged.

  - **DOT parser: self-contained `graph [...]` block skips next node** (#236
    follow-up): when our exporter writes `graph [label="Name"];` on a single
    line, the parser was entering multi-line netProperties mode and then
    treating the very next node declaration as the block terminator, silently
    discarding it. The skipped node was re-created with its DOT identifier as
    label ("n1" instead of "1") when the first edge statement was processed,
    producing wrong edge endpoints in the reloaded graph. Fix: only enter
    multi-line mode when `]` is absent from the `graph [` line. All three
    TinyGraphviz DOT io_roundtrip tests now show `ROUNDTRIP_EQUIV=1`.

  - **Multi-relation graph incorrectly reported as non-weighted** (#236
    follow-up): `isWeighted()` correctly scans only the current relation
    (fixed in #82). However, the io_roundtrip regression kernel queried
    `isWeighted()` after iterating all relations and restoring to relation 0,
    so a graph whose relation 0 is binary but relation 1 is weighted was
    reported as `weighted: false`. Added `Graph::isAnyRelationWeighted()`,
    which checks every relation, and updated the kernel to use it for the
    `graph.weighted` JSON field.

  - **Keyboard shortcut conflicts resolved**:
    - `Ctrl+T` (Data Table toggle) conflicted with the `Ctrl+T, Ctrl+*` prefix
      used by all four Structural Equivalence analysis actions — those sequences
      were silently unreachable on every platform. Data Table is now `Ctrl+D`.
    - `Ctrl+X, Ctrl+S` (Focus on Selection filter, WS9) duplicated the
      pre-existing `Ctrl+X, Ctrl+S` shortcut for "Selected nodes → Star"
      subgraph operation. Focus on Selection is now `Ctrl+X, Ctrl+O`.
    - `Ctrl+X, Ctrl+Q` (Query Builder filter) mapped to `⌘+Q` on macOS —
      the system-level Quit — when used as the second chord of a sequence.
      Query Builder is now `Ctrl+X, Ctrl+B`.
    - Duplicate `Ctrl+Shift+Left/Right` shortcuts removed from the rotate
      toolbar buttons (the menu actions already own those sequences).

  - **Bezier curve edges now work** (#149): the **Options → Edges → Bezier
    Curves** toggle was present but non-functional. Fixed and fully implemented:
    edges now draw as smooth quadratic arcs, arrowheads follow the curve tangent
    at each endpoint (including on reciprocated edges), the lens-shaped fill
    artifact is gone, new edges drawn while the toggle is on are born as curves,
    and the setting persists across sessions.

## [3.5] – May 8, 2026

### New Features

  - **Graph exploration filters** (WS9):
    - Focus on Node (Ego Network): hides all nodes except the selected node
      and its direct neighbors, and all non-incident edges. Available in the
      Filter menu and node right-click context menu (#211).
    - Focus on Selection: hides all nodes not in the current selection and all
      edges whose endpoints are not both selected. Action `Ctrl+X, Ctrl+S`;
      available in Filter menu and node right-click context menu (#210).
    - Restore All Nodes: restores all nodes hidden by any filter. Available in
      Filter menu and node right-click context menu.
    - Restore All Edges: re-enables all edges hidden by the weight filter.
      Action `Ctrl+E, Ctrl+R`; available in the Filter menu (#213).
    - All node-visibility filters (ego network, selection, centrality) now
      share a unified non-destructive snapshot/restore history stack — Restore
      All works across all filter types (#216).

  - **Ego-centered radial layout**: places a selected node at the canvas
    center, its 1-hop out-neighbors on an inner ring, and all remaining nodes
    on an outer ring. Available via Layout menu (`Ctrl+Alt+E`) and node
    right-click context menu (#214).

  - **Node/edge attribute system** (#224):
    - Single-key node attribute API: `Graph::vertexCustomAttributeSet()` /
      `vertexCustomAttributeRemove()`.
    - Edge custom attribute storage: `GraphVertex::m_outEdgeCustomAttributes`,
      `Graph::edgeCustomAttributesSet()` / `edgeCustomAttributes()`.
    - Edge Properties dialog (`DialogEdgeEdit`): edit label, weight, color and
      arbitrary custom key/value pairs; accessible from the toolbar and edge
      right-click context menu.
    - GraphML roundtrip for edge custom attributes: unique keys exported as
      `d2000+` `<key for="edge">` definitions; per-edge `<data>` tags written
      on save and parsed back on load.
    - Filter Nodes By Attribute: `Graph::vertexFilterByAttribute(key, value)` —
      non-destructive snapshot/restore filter; available in the Filter menu
      (`Ctrl+X, Ctrl+A`).
  - **Attribute-based filtering** (#217):
    - `FilterCondition` struct: scope (Nodes/Edges/Both), key, operator
      (`=` `≠` `>` `<` `≥` `≤` `contains`), value; `label()` for future
      filter bar chips.
    - `DialogFilterByAttribute`: scope selector, editable key combo populated
      from the graph's existing node/edge attribute keys, operator dropdown,
      free-text value field.
    - `Graph::vertexFilterByAttribute(FilterCondition)`: refactored to accept
      the full condition struct; numeric-aware comparison (tries `toDouble()`,
      falls back to lexicographic).
    - `Graph::edgeFilterByAttribute(FilterCondition)`: hides edges not
      matching the condition; uses the same snapshot/restore stack as node
      filters.
    - Filter combo added to the Control Panel (Network group) for one-click
      access to all filter actions.
    - Dedicated toolbar filter group with distinct icons for each filter action.
  - **Filter bar with chips** (#219):
    - Persistent `FilterBarWidget` strip between toolbar and canvas; hidden when
      no filter is active, auto-shows when any filter is applied.
    - Each active filter condition appears as a labelled chip (e.g.
      `Nodes: ego network`, `Edges: weight filter`, `Nodes: type = investor`).
    - ×-close on the most recently applied chip removes it and pops one entry
      from the snapshot/restore stack. Earlier chips show a disabled × with a
      tooltip explaining the order constraint.
    - "Clear all" button drains the full node filter stack and resets the edge
      filter in one click.
    - All five filter actions emit chips: centrality, ego network, selection,
      attribute (Nodes/Edges/Both), and edge weight filter.
    - Bar stays in sync when filters are removed via menu or toolbar actions.

  - **Node/edge data table dock** (#225):
    - New `GraphTableWidget` dockable panel (Ctrl+T, Options menu and Edit
      menu) with two tabs — Nodes and Edges — each backed by a
      `QAbstractTableModel` cache.
    - Node tab: fixed columns (#, Label, Visible, Shape, Size, Color) plus one
      column per custom attribute key. Label, Size, Color and custom attribute
      cells are inline-editable (double-click); #, Visible, Shape are read-only
      and rendered with a muted background.
    - Edge tab: fixed columns (Source, Target, Relation, Weight, Label, Color)
      plus custom attrs. Weight, Label, Color and custom attribute cells are
      editable; Source, Target, Relation are read-only and shaded.
    - All edits write back to the graph immediately via the Graph API.
    - Live search bar filters all columns (case-insensitive); column headers
      are sortable; a Refresh button reloads data from the current graph.
    - Panel auto-refreshes on file load and graph reset when it is open.
    - Action now has a dedicated `data_table_48px.svg` icon.

  - **Structured CSV/JSON export** (#226):
    - `TableExport::toCSV()` and `TableExport::toJSON()` free functions
      (`src/graph/io/table_export.*`) — QtCore only, no UI dependency.
    - Each tab in the Data Table dock gains **Export CSV** and **Export JSON**
      buttons; they export the currently visible (search-filtered) rows so
      what you see is what you get. Tooltip makes the scope explicit.
    - `Network → Export to other...` gains four new actions: **Nodes as CSV**,
      **Edges as CSV**, **Nodes as JSON**, **Edges as JSON** — these always
      export all rows regardless of any active search filter.
    - Status bar reports the export path on success.

  - **Structured CSV/JSON attribute import** (#227, refs #169):
    - `TableImport::fromCSV()` and `TableImport::fromJSON()` free functions
      (`src/graph/io/table_import.*`) — RFC 4180 CSV parser and JSON
      array-of-objects parser; QtCore only, no UI dependency.
    - `DialogImportAttributes`: file-browse + 8-row preview table +
      column-mapping controls. Nodes scope: **ID column** selector (node
      number or label matching). Edges scope: **Source** and **Target** column
      selectors with auto-detection of common names (`source`, `src`, `target`,
      `tgt`, `dest`). Import button is disabled until a valid file is loaded.
    - `Graph::vertexAttributesImport()` and `Graph::edgeAttributesImport()`:
      smart column routing — editable native columns (`Label`, `Size`, `Color`
      for nodes; `Weight`, `Label`, `Color` for edges) are routed to their
      proper setters; read-only native columns (`Visible`, `Shape`, `Relation`)
      are silently skipped; all other columns become custom attributes.
    - Each tab in the Data Table dock gains **Import CSV** and **Import JSON**
      buttons; the table auto-refreshes and the status bar reports the number
      of matched rows after import.
    - Enables a full lossless export→import roundtrip: re-importing an
      exported file produces no duplicate columns and no data loss.

  - **Spreadsheet-based bulk attribute editing workflow** (#232):
    - Emergent capability unlocked by combining #226 (export) and #227
      (import): export the data table to CSV or JSON, edit it freely in any
      spreadsheet tool (Excel, LibreOffice, Google Sheets), and re-import to
      update attributes in bulk. Each node/edge can carry different values —
      unlike in-app bulk operations (#228) which assign one value to many.
    - Native columns updated via their proper setters on re-import; no
      duplicate custom-attribute columns created.

### Improvements

  - Force-directed layouts improved for large graphs:
    - Fruchterman-Reingold: pre-cached adjacency (O(1) edge lookup in inner
      loop), initial random placement, early convergence detection.
    - Kamada-Kawai: canvas clamping replaces random teleport on out-of-bounds
      particles.

### Bug Fixes

  - Fixed Kamada-Kawai crash when node filters are active.
  - Fixed crash on graph reset: guard edge creation in `setEdgeVisibility`
    when the edge has already been removed (#231).
  - Fixed visibility history stack not cleared on graph clear / `initApp`.
  - Fixed custom node attribute key/id mismatch in GraphML export (#208).
  - Fixed Pajek parser: use default node shape as fallback when no Pajek
    shape keyword is present (#179).
  - Fixed `DialogClusteringHierarchical` signal/slot mismatch (#194).
  - Fixed Node Properties dialog UX issues for custom attributes (#130).
  - Fixed `graphTriadCensus()` appending stale zeros on repeated runs.

### Refactoring

  - New `Graph::vertexOutNeighborsSet()`: returns enabled 1-hop out-neighbors
    in the current relation; parametric for directed/undirected use.
  - Renamed `vertexNeighborhoodList/Set` → `vertexReciprocalNeighborsList/Set`
    to reflect that only reciprocal edges are considered.

### Testing / CI

  - New `socnetv-cli` clustering kernel v6 with golden baselines and benchmark
    coverage.
  - Aligned clustering benchmarks with CLI behavior; documented `--type`
    semantics.

### Build / Packaging

  - RPM spec fixes: conditional `Qt5Compat` BuildRequires per distro family,
    correct `qt6-qttools-devel` for Fedora, dropped redundant license/doc
    macros.

## [3.4] – March 2026

### New Features
  - Two-mode sociomatrix import: correctly handles bipartite networks in parser (#15).
  - Faithful Eades (1984) Spring Embedder implementation (#207).
  - New `layoutRandomInMemory()` replaces `layoutRandom()` in force-directed pipelines (#206).

### Bug Fixes
  - Progress dialog / Cancel (#52): comprehensive fix across all computation paths:
    - Wired Cancel into centrality, prestige, reachability, walks, matrix, report, layout, clique, and subgraph computations.
    - Fixed cancel-then-retry regression (reset canceled flag + invalidate distance cache).
    - Fixed stacked progress dialogs in multi-phase computations (KK layout, matrix functions).
    - All `write*` report functions converted to `bool` return; MW slots guarded on cancel.
    - All random network generators (`Erdos-Renyi`, `Small-World`, `Scale-Free`, `Regular`, `Lattice`, `Ring-Lattice`) fixed: bool return, cancel guards, progress max corrections.
  - Layouts:
    - Fixed division-by-zero, NaN/Inf and logic errors in Kamada-Kawai layout (#198).
    - Fixed FR simmering temperature derivation from canvas width (#199).
    - Batched `setNodePos` emissions in all force-directed layouts (#205, #206).
  - Centrality:
    - Fixed eigenvector centrality isolate reset and N==0 handling (#202).
    - Fixed Information Centrality isolate handling and degenerate cases (#201).
    - Fixed wrong vertex checked for `isIsolated` in `createMatrixAdjacencyInverse()` (#190).
    - Fixed clustering coefficient computation for directed networks (#58).
    - Fixed wrong weighted flag when switching relations (#82).
  - Parsers / IO:
    - Fixed Pajek `*Matrix` header parsing for relation labels (#188).
    - Fixed Pajek multirelational export as `*Matrix` blocks (#184).
    - Fixed quoted relation name normalization in Pajek headers (#185).
    - Fixed inline GML node/edge block parsing (#186).
    - Fixed arc doubling when loading undirected DOT graphs (#187).
    - Fixed platform-dependent `weighted=true` from uninitialized `initEdgeWeight` in DOT parser (#189).
    - Fixed `Graph::setDirected()` logic bugs.
  - Fixed lattice network edge deduplication and progress tracking.
  - Fixed version comparison in update-check (component-wise instead of integer).
  - Fixed `#133` (see commit).

### Refactoring (WS4 – IO/Parser)
  - Completed WS4: IO/Parser refactor into focused modules:
    - Extracted edgelist, adjacency, UCINET DL, DOT, GML, Pajek, GraphML parsers into separate files.
    - Introduced `IGraphParseSink` explicit mutation surface and `GraphParseSinkGraph` bridge.
    - Switched GUI and headless load paths to sink-backed parser mutations.
    - Removed legacy `Parser→Graph` signal wiring.
    - `Parser::load` and adjacency parser use `ParseConfig`.

### Toolchain / Testing
  - New `socnetv-cli` schema v5 `io_roundtrip` kernel for IO/parser regression protection.
  - Added IO roundtrip timing regression to benchmarks.
  - Expanded golden comparison suite with many new IO roundtrip cases and small deterministic test networks.
  - Added `run_io_roundtrip_shipped_datasets.sh` and `run_golden_io_roundtrip.sh` scripts.
  - Added UCINET FT5 io_roundtrip golden baselines.
  - Fixed `run_golden_compares.sh` argument parsing.
  - Fixed headless parser lifetime and signal race condition.

### i18n
  - Added `update_translations.sh` script; updated translation files.

### Build / Packaging
  - Debian packaging: switched to CMake build, series-aware Qt6 deps, added OpenGL/Vulkan/XKB build deps.
  - CMake: `.qm` files now generated via `qt_add_lrelease`.
  - Fixed Windows linker `/VERSION` for PE header.
  - Help menu now links to `socnetv.org/manual/`.

  - Many bugfixes, see: [GitHub Issues](https://github.com/socnetv/app/issues?q=is%3Aissue%20state%3Aclosed%20milestone%3A3.4).
  

## [3.3] – February 2026

  - Major internal refactor: `Graph` is now a façade/coordinator; functionality has been split into focused `src/graph/*` modules.
  - Extracted and stabilized DistanceEngine; added deterministic golden regression outputs and performance benchmark guardrails.
  - New headless regression harness `socnetv-cli` (modular kernels + schema-versioned JSON):
    - distance (v1), reachability (v2), walks_matrix (v3), prominence (v4)
    - strict JSON dump/compare mode with committed baselines.
  - New feature: filter vertices by centrality and prestige indices.
  - Fixed Pajek parsing edge cases (mixed files with overlapping *Arcs/*Edges blocks).
  - Fixed UCINET/DL import edge cases (line wrapping, diagonal handling).
  - Fixed walks computation (`walksBetween()` / walks matrix parameter issues).
  - Improved tie/link counting semantics on load (canonical ties + derived SNA links; density exposed in regression JSON).
  - UI polish: improved disabled widget styling and custom SVG checkbox/radio styling.
  - Cross-platform build & packaging fixes (Qt6/CMake, Debian packaging updates, openSUSE spec fixes, macOS arm64 linker fix).
  - Many bugfixes, see: [GitHub Issues](https://github.com/socnetv/app/issues?q=is%3Aissue%20state%3Aclosed%20milestone%3A3.3).

## [3.2] – April 2025

  - Support custom attributes (metadata) in nodes (via the Node Properties dialog).
  - Support for node labels in adjacency matrix formatted files.
  - New CMake-based build system.
  - Updated look and behavior of Filter Edges by Weight functionality.
  - Many bugfixes, see: [GitHub Issues](https://github.com/socnetv/app/issues?q=is%3Aissue%20state%3Aclosed%20milestone%3A3.2).

## [3.1] – June 2023

  - Version 3.1 released, our first Qt6-only version.
  - Improved large file loading and responsiveness with large networks (>20,000 edges).
  - Reduced memory footprint.
  - Fixed edge filtering (see issue #140).
  - Enhanced "Find Node by Index Score" dialog for more meaningful comparisons.
  - Fixed numerous bugs.
  - Improved usability and help messages.


## [3.0] – July 2021

  - Version 3.0 released for Windows, macOS, and Linux, with improved graph calculation speed and new command-line parameters.
  - First version to support hardware-accelerated (OpenGL) rendering of networks.
  - Improved Web Crawler:
    - Tests for OpenSSL support in the OS and provides user hints if OpenSSL is missing.
    - Fixed delay between requests.
  - Fixed a serious bug in weighted network centrality computations (see issue #123).
  - Note: To run SocNetV 3.0 AppImage in Fedora 34 (which uses Wayland by default), use:
    `env GDK_BACKEND=x11 ./SocNetV-3.0-dev-x86_64.AppImage`
  - OBS repositories are working again. Fedora/openSUSE packages can be downloaded from:
    [OBS Repositories](https://download.opensuse.org/repositories/home:/oxy86/)


## [2.9] – June 2021

- Version 2.9 released, bugfixes
- Version 3.0 development. To run 3.0-dev in Fedora, use:
   env GDK_BACKEND=x11  ./SocNetV-3.0-dev-x86_64.AppImage


## [2.8] – Jan 2021

- Version 2.8, with some bugfixes


## [2.7] – Dec 2020

- Version 2.6 and 2.7 release with bugfixes and new features.


## [2.5] – Feb 2019

  - Version 2.5 released with new features and bugfixes.
  - Prominence scores distribution in reports and in-app mini chart.
  - Support for custom node icons (PNG, JPEG, SVG, etc).
  - Edge dichotomization algorithm.
  - High quality theme, inspired by Material Design, for uniform look and feel of SocNetV across all OSes.
  - Support for (double) edge weights in all formats.
  - Improved export to PDF and Image (with lots of new formats).
  - Improved web crawler.
  - Lattice network generator.
  - Improved memory consumption and faster measure computations.
  - Search and select multiple nodes by their numbers, labels, or prominence scores.
  - Many bug fixes.


## [2.4] – Feb 2018

- Version 2.4 released with many new features.
- New Force-Directed Placement layout: Kamada-Kawai.
- New layout type by prominence score: Node colors.
- Less clutter in visualization due to reciprocated edges. These are now being drawn in a single line.
- Improved memory consumption during user interaction with large networks
- Improved web crawler with pattern include and exclude options
- Improved Statistics Panel.
- Performance options in Settings dialog
- Improved UCINET format support (fullmatrix two-mode and edgelist).
- New "Check for updates" procedure.
- Much improved stability. See Changelog for bugs closed.


## [2.3] – Jul 2017

- Version 2.3 released with bugfixes and new features:
- Dyad and  Actor/Ego reciprocity
- Zero-weighted edge support and zero-weighted edge color selection functionality in Settings
- Bug Closed:
 - #28 Edges with values in [-1,0) are not visible
 - #29 Settings: Negative edge colour preferences break positive edge colours 


## [2.2] – Jan 2017

- Version 2.2 released with major new features.
- Hierarchical Clustering Analysis (HCA)
- Pearson correlation coefficients
- Actor Similarities
- Tie profile dissimilarities
- Maximal clique census
- New network symmetrization methods: Strong Ties, Cocitation
- Multi-relational data read and write in GraphML
- GML format support
- Support for EdgeLists with labels
- Support for Pajek multirelational directed networks
- Adjacency matrix plotting
- Better reports (in HTML with JS)
- Improved performance and GUI

## [2.1] – Sep 2016

- Version 2.1 released with a few new features but lots of bug fixes.
  This version brings a new algorithm for d-regular random network generation,
  and also a nice new dialog to control it.
  See ChangeLog for a complete log of new features and bugfixes.
- Version 2.0 released with major code overhaul, new GUI layout and lots of bugfixes and improvements.
  The new version brings stability, great performance boost, and nice new features such as separate modes
  for graphs and digraphs, permanent settings/preferences functionality, edge labeling, recent files, 
  keyboard shortcuts, etc. Also there are improvements in Force-Directed layouts, i.e. Fructherman-Reingold.
  See ChangeLog for a complete overview of the new features.
- The SocNetV Manual is now build with Doxygen and it is available at http://socnetv.sf.net/documentation

## [1.9] – June 2015

- Version 1.9 released with lots of bugfixes and a faster matrix inverse routine using LU decomposition. 
  Also Information Centrality is greatly improved in terms of computation speed.
  PageRank Prestige algorithm corrected to compute PR using the correct formula. The initial PR score 
  of each node is now 1/N.
  Bugs closed:
    #1463069 wrong average distance when there are isolates 
    #1365037 certain sparse matrices crash socnetv on invertMatrix method 
    #1365582 centralityInformation() is slow when network N>100 
    #1463095 edge filter works but the user cannot undo 
    #1464422 wrong pagerank results 
    #1464430 socnetv refuses to read pajek files not starting with *Network 
    #1465774 edges do not always follow relations 
    #1463082 edge color change is not taking place 
    #1464418 socnetv crashes on pagerank computation on isolated nodes 

- Version 1.8 released with the following new features: 
  New clique census routine to compute maximal cliques with up to 4 vertices.
  New Scale-free random generation methods. Improved Erdos-Renyi generation to include G(n,M) model. 
  Fixed bug in Clustering Coefficient - SocNetV now computes CluCof correctly in all cases.
  New improved dialogs for easy random network generation (Scale-free, Erdos-Renyi, and Small-World)
  Fixed bug in Node Properties dialog. It is now populated with current node settings.

## [1.7] – May 2015

- Version 1.7 released. New node group select/edit functionality and file previewer supporting  different codecs
- Version 1.6 released. New and improved web crawler functionality. See Changelog for more.

## [1.5] – Oct 2014

- Version 1.5 released. First version with dijkstra algorithm for the SSSP in weighted nets. See Changelog for more.

## [1.4] – Sep 2014

- Version 1.4 released. Brought new layout type (nodal size by prominence index), edgelist1 UCINET format import method and many bugfixes.

## [1.3] – Aug 2014

- Version 1.3 released.
- First time SocNetV works with multigraphs 

## [1.2] – Aug 2014

- Version 1.2 released. It features a major GUI overhaul and brings in a new "prominence indices" conceptualization based on Wasserman & Faust. 
  In general, Centrality indices focus on outLinks (choices given) while Prestige indices consider inLinks (choices received).
  Added 3 Prestige indices (Degree, Proximity and PageRank), new reachability measures (Walks, Connectedness, and Reachability Matrix) and fixed a slew of bugs in indices calculation. 
  All algorithms are now tested to report 100% correct results.
- Version 1.1 released with major bug fixes. See ChangeLog.
- First time distribution of a disk image for installation in Mac OS X

## [1.0] – Feb 2014

- Version 1.0 released, starting a new 1.x series based on Qt5. The 0.x series is no longer maintained. Please upgrade :)
- PageRank calculation and layout 
- SRS Documentation by Vagelis Motesnitsalis

## July 2013

- Moved project code to git/BB
- Started development for Qt5

## Oct 2010

- Version 0.90 released
- New Power & Information Centralities

## Jan 2010

- Version 0.80
- New List import feature
- New Triad Census feature
- Various Bug Fixes

## June 2009

- Version 0.70
- First web crawler implementation 

## May 2009

- Version 0.6 (release)
- GraphML becomes native SocNetV load format

## Feb 2009

- Version 0.51 (bugfix release)
- Version 0.50 (released)
- Small world creation
- Clustering coefficient
- Exporting to PDF
- Printing works OK.

## Jan 2009

- Version 0.49 (released)
- Ubuntu repository created. 

## Sep 2008

- New logo
- New openSUSE package repo.
- Version 0.48 released
- Version 0.47 released
- Version 0.46 released.
  Lots of bugfixes.
  New features:
  - Node sizes may reflect degree.

## Aug 2008

- New Debian Package
- Version 0.45 released. 
  New features:
  - GraphML initial support.
  - New man page and updated online documentation.
  - HtmlViewer renders online help with the help of QtWebKit (openSUSE: libQtWebKit-devel)
  - New widget for network rotation.
  - New widget for zooming replaces the old one.
  - Nodes may have 4 different shapes: circles, diamonds, triangles, boxes and ellipses are supported.
  - There was a bug in Qt 4.3 QGraphicsView causing redraw delays. Is fixed in Qt 4.4 :)
  - Cosmetic changes, i.e. new icons, new layout for the left dock.
  - Code clean-up in MainWindows Class and Matrix. 
  - Deleted obsolete members and functions such as nodeExists(), mousePosGW(), Dijkstra, etc. 
  - Bug-fixes on loading Pajek networks and layout algorithm.

## May 2008

- Version 0.44 released one year after v.0.43.
  New features: 
	Ported to Qt4: Code rewritten almost from scratch.
	Splitted MainWindow/GUI from algorithms via a new Graph Class. 
	Improved GUI with docks.
	Network zooming via mouse wheel.
	Spring Embedder: Dynamic network reallocation
	Thread support.
	Much faster calculation of distances and centralities (BFS/dijkstra).
	Betweenness centrality now is much more efficiently calculated. 
	Changed license to GPL3
	Layout in circles and levels by centrality.
	Better graphics and antialiasing (disabled - enable by pressing F8).
	New centrality index: Eccentricity.
	

## Sep 2006

- version 0.43 released with new layout features.

## June 2006

- version 0.42 released with updated help files.

## May 2006

- Did some work on the webpages at http://socnetv.org. Hope it is better now.

## March 2006

- version 0.41 released. 


## February 2006

- version 0.40 released. Efforts to be a pretty trustworthy release.
- sourceforge project downloads are more than enough daily, but there is no feedback yet for versions 0.38 and 0.39.
- version 0.39 released. Somewhat rushed release.
- constant changes in the homepage. 
- updated links in www.insna.org 


## January 2006

- version 0.38 released after one year of silence.
- The project moved to sourceforge.net
- The homepage is https://socnetv.org