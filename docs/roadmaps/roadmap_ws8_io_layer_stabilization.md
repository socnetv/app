# IO Layer Stabilization Roadmap (WS8)

## Goal

Consolidate per-format IO dispatch — parsing, export, file-dialog filters, and single-relation
warnings — behind a single `FormatHandler` registry, replacing four hand-maintained switch
statements that currently must be kept in sync by hand (see Background: the GML/TWOMODE export
gap is exactly the kind of drift that produces silently). One handler per format, registered once:

```cpp
struct FormatHandler {
    FileType type;
    QString displayName;                    // "Pajek", "GraphViz DOT", ...
    QString fileDialogFilter;                // "Pajek (*.net *.paj *.pajek)"
    bool supportsExport;
    bool singleRelationOnly;                 // drives the mainwindow.cpp:12162 warning
    std::function<bool(const QByteArray&, const ParseConfig&, IGraphParseSink*)> parse;
    std::function<bool(Graph*, const QString&)> exportTo;  // nullptr if !supportsExport
};
```

A single `QList<FormatHandler>` (or `QHash<FileType, FormatHandler>`) becomes the one source of
truth: `Parser::load()`'s switch becomes a lookup + uniform call; `Graph::saveToFile()`'s switch
becomes the same; the two `mainwindow.cpp` switches become lookups into the same table's
`displayName`/`fileDialogFilter`/`singleRelationOnly` fields.

**Non-goals (matching WS4's own constraints):** no parsing behavior changes, no numeric/semantic
output changes — this is purely consolidating *dispatch*, not touching what any format parser does
internally. Golden regression parity required throughout, same as every other workstream.

## Status

📋 Design drafted, not yet started. Previously three aspirational bullet points in
`ARCHITECTURAL_REFACTORING_ROADMAP.md` with no dedicated file — fleshed out here after confirming
via direct code reading that the scope is real and distinct from WS4 (not a duplicate/leftover).

## Background

### Relationship to WS4

WS4 (`roadmap_ws4_io_parser_refactor.md`, complete) achieved:
- A shared mutation contract (`IGraphParseSink`) — parsing mutations flow through one interface.
- A shared `ParseConfig` struct instead of long parameter lists.
- Translation-unit separation — each format's parsing logic lives in its own `parser_*.cpp`.

WS4 never touched **dispatch** — how the app decides *which* format-specific function to call, and
where each format's metadata (display name, file extensions, capabilities) lives. That's WS8.

### Current Reality — four places hand-maintain the same per-format knowledge

Confirmed by direct reading, not assumption:

1. **`Parser::load()`** (`src/parser.cpp:207`) — a 10-case `switch(fileFormat)` calling
   `parseAsGraphML(rawData)`, `parseAsPajek(rawData)`, `parseAsAdjacency(rawData, cfg, delimiter)`,
   `parseAsEdgeListWeighted(rawData, delimiter)`, etc. **The per-format functions don't even share
   a uniform signature** — some take just `rawData`, others take `rawData, delimiter`, others
   `rawData, cfg, delimiter`. Adding a new format means adding a new case here by hand.
2. **`Graph::saveToFile()`** (`src/graph/io/graph_io.cpp:225`) — a parallel 7-case
   `switch(fileType)` calling `saveToPajekFormat(...)`, `saveToAdjacencyFormat(...)`, etc. Only 7 of
   the 9 importable formats have an export case (no `GML` or `TWOMODE` export) — this asymmetry is
   currently only discoverable by reading both switches side by side.
3. **`MainWindow`'s file-dialog filter switch** (`src/mainwindow.cpp:6395`) — a third switch mapping
   each `FileType` to its display name and file-extension filter string (e.g.
   `"Pajek (*.net *.paj *.pajek);;All (*)"`), hand-kept in sync with the two above.
4. **`MainWindow`'s single-relation-warning switch** (`src/mainwindow.cpp:12162`) — a fourth,
   partial switch mapping a subset of formats (`ADJACENCY`, `GRAPHVIZ`, `EDGELIST_WEIGHTED`,
   `EDGELIST_SIMPLE`) to a human-readable name, used only to warn the user that a multi-relation
   graph will lose relations on export to a single-relation-only format.

Every one of these four locations must be updated by hand whenever a format is added, renamed, or
gains/loses a capability (e.g. export support, multi-relation support). Nothing enforces they stay
in sync — the GML/TWOMODE export gap above is exactly the kind of drift this produces silently.

## What WS8 Delivered

Nothing shipped yet.

## What Remains Open

### Milestones

- **W8.1 — Inventory and uniform signature.** Catalog the exact current signature of every
  `parseAs*` function; design the single uniform signature (likely
  `bool parse(const QByteArray&, const ParseConfig&, IGraphParseSink*)`, since `ParseConfig`
  already carries `delim` — meaning the delimiter-only-vs-full-config inconsistency in point 1
  above can be resolved as part of this step, not left as-is). Adjust each `parseAs*` function to
  match. No dispatch changes yet — completion criteria: build passes, golden regression unchanged.

- **W8.2 — Introduce `FormatHandler` struct and registry.** Add the struct above (see Goal) and a
  `QList<FormatHandler>` populated with all 9 import formats + 7 export formats, built from the
  actual current per-format metadata (extensions, display names, capability flags) gathered in
  W8.1. Registry exists but nothing consumes it yet. Completion criteria: build passes.

- **W8.3 — Migrate `Parser::load()` to registry-backed dispatch.** Replace the `switch` with a
  lookup + uniform call through the registry. Completion criteria: `run_golden_compares.sh` and
  `run_golden_io_roundtrip.sh` pass unchanged.

- **W8.4 — Migrate `Graph::saveToFile()` to registry-backed dispatch.** Same treatment for export.
  Completion criteria: golden IO roundtrip baselines unchanged.

- **W8.5 — Migrate both `mainwindow.cpp` switches to registry lookups.** File-dialog filter string
  and the single-relation-only warning both read from the same registry instead of their own
  hand-maintained copies. Completion criteria: manual smoke test — open each supported format via
  the file dialog, confirm filter strings unchanged; trigger the multi-relation export warning on
  one of the four affected formats, confirm the message is unchanged.

- **W8.6 — Close the GML/TWOMODE export gap, or document it as intentional.** Once the registry
  makes the asymmetry visible in one place (`supportsExport: false` for those two), decide
  explicitly whether to implement the missing exporters or document why they're intentionally
  import-only, rather than leaving it as an undocumented gap only discoverable by reading two
  switches side by side.

### Related, but distinct — #8 Enhance GML format support

Not a dispatch-consolidation item like W8.1–W8.6 above — this is parsing-*depth* work on the GML
format specifically. The current GML parser supports a minimal command set (`graph`, `comment`,
`directed`, `node`/`id`/`label`, `edge`/`source`/`target`/`label`); the
[GML technical report](http://www.fim.uni-passau.de/fileadmin/files/lehrstuhl/brandenburg/projekte/gml/gml-technical-report.pdf)
defines a substantially larger command set. Grouped here because it's the same file-format-support
territory as WS8, not because it's part of the registry-consolidation work — worth picking up
alongside or after the `FormatHandler` migration, not blocked on it.

## Work Rules

- No parsing/export behavior changes — this is dispatch consolidation only, matching WS4's own
  non-goals.
- Migrate one switch at a time (W8.3 → W8.4 → W8.5), golden-regression-verified after each, not as
  one large combined change.
- `run_golden_compares.sh` and `run_golden_io_roundtrip.sh` must pass after every milestone.
