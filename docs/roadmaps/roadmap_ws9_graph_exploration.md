# Roadmap: Graph Exploration (WS9)

## Goal

Evolve SocNetV from a visualization-focused application into a full graph exploration and data
workflow platform: non-destructive filtering, structural/attribute queries, subgraph extraction,
and structured (table/CSV/JSON) data workflows.

## Status

✅ Complete. All three feature tracks shipped in v3.5–v3.6. Current filter-layer architecture lives
in [`README_DEVELOPER_NOTES.md`](../README_DEVELOPER_NOTES.md)'s "Filter Layer" section, not here.

## What WS9 Delivered

- **Feature 1 — Visualization & decluttering** (#209) — focus-on-selection, ego networks (k=1),
  edge filtering by weight, non-destructive restore, ego-centered radial layout, color-by-metric
  (Clustering Coefficient added as a full prominence index).
- **Feature 2 — Filtering & subgraphs** (#215) — attribute-based filtering (`FilterCondition`), a
  persistent chip-based filter bar with arbitrary chip removal via a replay stack (`FilterSpec`), a
  visual query builder (`GraphQuery`, AND-only), subgraph extraction
  (`Graph::subgraphExtract()`/`subgraphExtractFromSelection()`) and export to all 7 supported
  formats.
- **Feature 3 — Data workflows** (#223) — attribute editing UI, a live-search node/edge table dock
  (`NodeTableModel`/`EdgeTableModel`/`GraphTableWidget`), CSV/JSON export and import
  (`TableExport`/`TableImport`, `src/graph/io/`) with column-mapping, and bulk editing
  (`DialogBulkEdit`) across canvas-selection/filtered/multi-row-table target sets.

## Known Gaps

- Bulk-edit operations (Feature 3) bypass the undo stack — tracked under
  [WS13](roadmap_ws13_undo_redo.md) (#31).
- Arbitrary chip removal doesn't cover selection/ego/centrality filters — those don't store enough
  in `FilterSpec` to replay after removal (needs either #31 structural undo or per-type parameter
  storage).

## What Remains Open

- **Persistent named subgraphs + tab-based multi-graph UI** (#245) — switch between multiple named
  derived views without reloading. Blocked on the tab UI, a significant infrastructure investment;
  not started.
- **Query Builder OR logic** and a **text-based query DSL** — `GraphQuery` is AND-only today.
- **Community-based node coloring** (#258) and **edge bundling** (#259) — deferred, each its own
  algorithmic/rendering effort independent of WS9.
- **In-app derived fields / normalization / type coercion** — deferred; the CSV/JSON roundtrip
  workflow already covers this via external spreadsheet tools.
- **Temporal data** (#222, general case #25) — `FilterCondition`'s `Lte`/`Gte` operators already
  support date-range queries once attributes carry typed dates; the timeline/animation layer is
  what's missing.
- **Attribute inspector panel / persistent filter panel** as real docked widgets (currently
  dialog-driven) — deferred until WS7's MainWindow decomposition is underway.
