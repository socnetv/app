# SocNetV Developer Notes

Technical reference for contributors. For the architectural direction and workstream roadmap, see:

* [`ARCHITECTURAL_REFACTORING_ROADMAP.md`](ARCHITECTURAL_REFACTORING_ROADMAP.md)

Detailed workstream plans:

```
docs/roadmaps/
```

> **Before committing any structural change:** run `./scripts/run_golden_compares.sh` (see
> "Regression discipline" under Development Workflow, below, for the full script list). All golden
> JSON baselines must still pass.

---

# Architecture Overview

SocNetV is a Qt-based desktop application for social network analysis and visualization.

The architecture is layered across two threads:

```
┌─ main thread ──────────────────────────────────────────────────┐
│                                                                 │
│  MainWindow + dialogs          (menus, panels, status bar)      │
│       ↕ signals                                                 │
│  GraphicsWidget                (QGraphicsView canvas)           │
│    └─ GraphicsNode / GraphicsEdge  (QGraphicsItem scene items)  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
         ↕ queued cross-thread signals (setNodePos, etc.)
┌─ graphThread ──────────────────────────────────────────────────┐
│                                                                 │
│  Graph                         (façade — state, invariants)     │
│    └─ Algorithm slices         (src/graph/<domain>/)            │
│    └─ UI façade layer          (src/graph/ui/ — charts, export) │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

`Graph` runs on `graphThread`; all canvas mutation goes through queued
signals to the main thread. Do **not bypass this flow** when adding new features.

---

# Graph as Façade

`Graph` is a state coordinator and invariant guardian — not a monolith. It holds graph state and delegates all computation to algorithm slices.

`graph.cpp` contains only:

```
Graph::Graph(...)
Graph::~Graph()
Graph::clear(...)
```

Everything else lives under `src/graph/`, organized by responsibility:

```
centrality/
clustering/
cohesion/
core/
crawler/
distances/
filters/
generators/
io/
layouts/
matrices/
prominence/
reachability/
relations/
reporting/
similarity/
storage/
ui/
util/
```

---

# Structural Boundary Inside `src/graph/`

A strict separation is enforced between computation and rendering.

## Algorithm slices

- compute data only
- may use QtCore
- must **not** construct QtWidgets / QtCharts objects
- must **not** emit UI signals directly

Examples:

```
src/graph/prominence/graph_prominence_distribution.cpp
src/graph/centrality/graph_centrality.cpp
```

## UI façade layer (`src/graph/ui/`)

- constructs QtWidgets / QtCharts objects
- renders visualizations
- exports PNG charts
- emits UI update signals to `MainWindow`

Example:

```
src/graph/ui/graph_ui_prominence_distribution.cpp
```

**Must run on the GUI thread.** `src/graph/ui/` functions are plain `Graph::` methods — same
`QObject`, same thread affinity as `Graph` itself, which runs on `graphThread`, not the GUI thread.
Constructing real Qt GUI objects (e.g. `QtCharts`) off the GUI thread is undefined behaviour. Wrap
the entire function body in `Graph::runOnGuiThread(std::function<void()>)`
(`graph_ui_facade.cpp`) to force GUI-thread execution regardless of the calling thread — see the
`uiProminenceDistribution{Spline,Area,Bars}()` functions for the pattern.

## Rule for New Code

If you add new analytics:

1. **compute results in algorithm slices**
2. **render in the UI façade**

This separation is mandatory.

---

# Distance Engine

Shortest-path algorithms run through a dedicated engine:

```
src/engine/
  distance_engine.cpp
  distance_progress_sink.h
  graph_distance_progress_sink.cpp
  per_source_scratch.h        ← introduced in WS3 Phase 1
  thread_local_state.h
```

The engine runs from both the GUI and the CLI regression harness.

Call flow:

```
DistanceEngine::compute(computeCentralities, considerWeights, inverseWeights, dropIsolates)
  initRun()
  runAllSources()          ← parallel via QtConcurrent::blockingMap (WS3 Phase 2)
  finalize()
```

Scratch state is layered by scope — never held on `Graph`/`GraphVertex` directly:

| Struct | Scope | Location |
|---|---|---|
| `DistanceScratch` | Per run (N/E snapshots, iterators, connectivity) | `distance_engine.h` |
| `PerSourceScratch` | Per source vertex — reused within a thread | `src/engine/per_source_scratch.h` |
| `ThreadLocalState` | Per worker thread — owns a `PerSourceScratch` + partial BC/SC arrays | `src/engine/thread_local_state.h` |

**WS3 parallelisation is complete (Phases 1 and 2 shipped).** The source loop in
`DistanceEngine::runAllSources()` runs concurrently across all CPU cores via
`QtConcurrent::blockingMap`. Each worker thread owns a `ThreadLocalState` holding a reused
`PerSourceScratch`, partial BC/SC accumulators, and running totals for graph-wide aggregates. A
single-threaded reduction step after the loop merges everything into graph state.

Benchmark results (Debug build, 24-core Linux): 2.7×–8.3× speedup depending on network size and
whether centralities are computed.

**APSP results (geodesic distances, sigma counts) live in `Graph::m_apspDist`/`m_apspSigma`**
(`QHash<relation_id, Matrix>`, row = source vertex position, column = target vertex position;
read via `Graph::apspDistance()`/`apspShortestPaths()`) — not on `GraphVertex` (WS5 A2 migrated this
off a per-vertex `QHash` for lookup speed and memory; see
`roadmap_ws5_matrices_modernization.md`).

Do not add new per-source mutable state to `Graph` or `GraphVertex` — put it in `PerSourceScratch`.

---

# Matrix Storage

`Matrix` (`src/matrix.h`/`.cpp`) is the general-purpose dense matrix used throughout SocNetV for
adjacency, distance, similarity, and sociomatrix data (`AM`, `invAM`, `DM`, `WM`, and the other
named fields on `Graph`).

**Storage: 2D access on a 1D array, via a precomputed row-pointer index.** Every cell lives in one
contiguous allocation (`m_data`, row-major) — one allocation for the whole grid, not one per row.
A second, much smaller array (`m_rowPtr`) records where each row starts inside `m_data`, rebuilt
once whenever the matrix is constructed or resized (`rebuildRowPtr()`). Reading or writing cell
`(r,c)` is then: look up row `r`'s start address in `m_rowPtr` (one array read), step `c` cells
forward from it (one pointer offset) — no multiplication at access time, regardless of matrix size.
This matters because `m_cols` is a runtime value, not a compile-time constant: computing a cell's
address as `r*m_cols+c` on every access is a genuine multiply every time, not something the compiler
can fold away — precomputing each row's address once avoids repeating that multiply on every single
cell access in the O(N²)/O(N³) algorithm loops that dominate `matrix.cpp`.

**API:** `item()`/`setItem()`/`clearItem()` (copy-in/copy-out) are the normal read/write path used
everywhere in the codebase. `operator[]` (`a[i][j]`, returns a raw `qreal*` via `m_rowPtr`) exists
only because `ludcmp()`/`lubksb()`/`inverse()` need to modify cells in place with compound
assignment (`a[i][j] -= x`), which a copy-in/copy-out getter/setter pair can't express — every other
caller should use `item()`/`setItem()`.

**`QHash` vs `Matrix` for per-relation lookup tables** (`Graph::m_apspDist`/`m_apspSigma`, see
Distance Engine above): `Matrix` always allocates N² cells regardless of reachability; `QHash` only
stores entries for reachable pairs. Measured across connected/disconnected/realistic topologies:
`Matrix` wins on both memory and lookup speed for any topology dominated by one giant connected
component — which is what real networks look like. `QHash` only wins on memory for artificial
graphs with many equal-sized disconnected islands, a shape that doesn't occur in practice. Default
to `Matrix` for this kind of table unless a specific network's topology is known to be many
disconnected islands.

**A win in isolated cell-access throughput does not guarantee an end-to-end win.** Confirmed when
evaluating the `Matrix` flat-buffer refactor: raw `item()`/`setItem()` throughput improved measurably
in isolation, but `DistanceEngine`'s real BFS-driven compute showed no measurable end-to-end change
— old and new code overlapped well within normal run-to-run noise. Cause: `DistanceEngine`'s BFS
traversal is O(N·(V+E)), matrix writes are only O(N²); for typical graphs traversal work dominates,
so a real win in matrix-access cost alone doesn't move the total by enough to rise above measurement
noise. When evaluating a `Matrix`-layer optimization, measure the actual call site's end-to-end time,
not just isolated access throughput — the two can disagree.

---

# Parsing and I/O

The parsing pipeline:

```
Parser
↓
IGraphParseSink
↓
Graph
```

Key components:

```
src/graph/io/graph_parse_sink.h
src/graph/io/graph_parse_sink_graph.cpp
```

Typical mutation calls:

```
createNode(...)
createNodeAtPosRandom(...)
createNodeAtPosRandomWithLabel(...)
createEdge(...)
setRelation(...)
addNewRelation(...)
removeDummyNode(...)
fileLoaded(...)
```

GUI and CLI share the same mutation pipeline — deterministic behavior is guaranteed. Legacy
Qt mutation signals from `Parser` to `Graph` no longer exist; `IGraphParseSink` is the sole
mutation plane.

**Waiting for a load to finish (headless loaders):** block on `Graph::signalGraphLoaded`
(preferred) with `Parser::finished(QString)` as a fallback — `finished` is lifecycle completion
only, `signalGraphLoaded` is the actual "mutations are done" signal.

**Format parsers** live one-per-translation-unit under `src/parser/` (`parser.cpp` is just the
constructor/destructor, sink wiring, and the `load()` dispatcher):

```
src/parser/
  parser_common.cpp     ← shared helpers (isComment, ...)
  parser_edgelist.cpp
  parser_adjacency.cpp
  parser_dl.cpp          (UCINET)
  parser_pajek.cpp
  parser_graphml.cpp
  parser_gml.cpp
  parser_dot.cpp         (GraphViz)
```

Adding a new format: add its own `parser_<format>.cpp`, wire it into `Parser::load()`'s dispatcher,
add a golden `io_roundtrip` baseline (`socnetv-cli --kernel io_roundtrip ... --dump-json`,
committed under `src/tools/baselines/io_roundtrip/`, wired into `run_golden_compares.sh`).

---

# Filter Layer

Non-destructive node/edge visibility filtering — the graph itself is never mutated, only
visibility state:

- `vertexFilterByEgoNetwork()`, `vertexFilterBySelection()`, `vertexFilterByAttribute()`,
  `vertexFilterByQuery()`
- `edgeFilterByWeight()`, `edgeFilterByAttribute()`, `edgeFilterByQuery()`
- `vertexFilterRestoreAll()` — drains the full stack back to the base graph

Every filter pushes a `FilterSpec` (`src/graph/filters/filter_spec.h`) onto
`m_visibilityHistory`. **Arbitrary removal**, not just undo-the-last-one: `vertexFilterRemoveAt(int
stackIndex)` restores the base graph, then replays every *remaining* spec in order via
`vertexFilterReplaySpec()` — a full dispatch table over every `FilterSpec::Type`. This only works
for filter types that store enough in their spec to replay (`Attribute`, `EdgeAttribute`,
`EdgeWeight`, `Query`, `EdgeQuery`); `Selection`/`Ego`/`Centrality` chips currently only support
removal from the top of the stack.

Attribute/structural queries are built from `FilterCondition` (`src/graph/filters/filter_condition.h`
— scope/key/op/value, numeric-aware comparison) and `GraphQuery` (`src/graph/filters/graph_query.h`
— a list of conditions, AND-only today).

**Subgraph extraction** (non-destructive filtering's natural counterpart — makes a filtered view
permanent): `Graph::subgraphExtract()` / `subgraphExtractFromSelection()`, both built on private
`Graph::subgraphFromVertexList()` — renumbers vertices from 1, mirrors all relations, preserves
custom attributes.

**Structured table data** (node/edge table dock, CSV/JSON import/export) lives in
`src/graph/io/table_export.*` / `table_import.*` (`TableExport`/`TableImport`, free functions,
QtCore-only, any `QAbstractItemModel*`) plus `NodeTableModel`/`EdgeTableModel`/`GraphTableWidget`
(`src/widgets/`) for the UI side.

---

# Dispatching Long-Running Graph Operations (MainWindow → Graph)

`Graph` lives on `graphThread`, not the GUI thread — but a plain C++ method call always executes on
the *caller's* thread regardless of the callee `QObject`'s thread affinity. Calling a slow `Graph`
method directly from a `MainWindow` slot freezes the whole application for the computation's
duration (confirmed: computing a weighted/inverted centrality index on a 7,343-node network showed
795-900% CPU with the window completely unresponsive to input or repaint for several minutes — #254).

**Always dispatch long-running `Graph` operations via
`MainWindow::runGraphOperationAsync(operation, waitMessage, doneMessage)`** (`mainwindow.cpp`),
which posts `operation` onto `graphThread` via `QMetaObject::invokeMethod(activeGraph, ...,
Qt::QueuedConnection)` and shows an indeterminate, `ApplicationModal` `QProgressDialog` for the
duration — which also prevents the user from mutating `Graph` from the GUI thread mid-operation,
since `Graph` has no internal synchronization against that. Not `QtConcurrent::run()`:
`DistanceEngine::compute()` already parallelizes internally via `blockingMap` over the *global*
thread pool, so a second `QtConcurrent::run()` task would contend with those worker tasks for the
same pool.

Two gotchas already hit with this pattern:
- Close the progress dialog with `reset()`, not `close()` — `close()` triggers `QProgressDialog`'s
  internal `cancel()` path, which silently marks the (actually successful) operation as cancelled.
- Any `src/graph/ui/` function invoked this way that constructs real Qt GUI objects (charts, etc.)
  must wrap its body in `Graph::runOnGuiThread()` — see "UI façade layer" above.

---

# Graph → Canvas Rendering System

## Threading model

`Graph` runs on a dedicated `graphThread`. All communication with
`GraphicsWidget` (canvas) is via **queued signal/slot connections** across
the thread boundary.

```
graphThread: Graph
     ↓  emit setNodePos(nodeNum, x, y)   [queued — cross-thread]
main thread: GraphicsWidget::moveNode()
     ↓  node->setPos(x, y)
     ↓  ItemPositionHasChanged → adjust() on all connected edges
```

`moveNode()` is intentionally one line. Do not add logic there.

## Layout signal discipline

Force-directed layouts (Eades, Fruchterman-Reingold, Kamada-Kawai) run all
iterations on `graphThread` and emit `setNodePos` **once per node at the
end**, not once per node per iteration. This avoids flooding the main
thread's event queue.

Static layouts (radial, levels, BC-based) emit exactly N signals in a
single pass — the minimum possible without a new bulk signal type.

**Do not add per-iteration `setNodePos` emissions** to any layout.

## Scene index method

`QGraphicsScene` defaults to `BspTreeIndex`, which rebuilds a spatial BSP
tree on every `prepareGeometryChange()` call. On large networks this is the
dominant performance cost: selecting or dragging nodes triggers O(E) edge
`adjust()` calls, each paying an O(log N) BSP update.

**The app defaults to `NoIndex`** (set via `appSettings["canvasIndexMethod"]`
at startup). This eliminates BSP overhead entirely. Hit-test cost becomes
O(N) per click, which is imperceptible for network analysis workloads.

The setting is user-controllable via **Settings > Canvas**. `BspTreeIndex`
remains available for small, static networks where fast hit-testing matters.

## Known hot path: node selection

`GraphicsNode::itemChange(ItemSelectedHasChanged)` calls `setSize()` and
`setColor()`, both of which call `prepareGeometryChange()` and trigger
`adjust()` on every connected edge. For N selected nodes this is O(N × avg
degree) geometry work, synchronous on the main thread. With `NoIndex` this
is fast in practice; with `BspTreeIndex` on large networks it causes
visible lag.

## Canvas performance knobs

All rendering flags are user-configurable via **Settings > Canvas**:

| Setting | Key | Default |
|---------|-----|---------|
| Scene index method | `canvasIndexMethod` | `NoIndex` |
| Viewport update mode | `canvasUpdateMode` | `Full` |
| Antialiasing | `antialiasing` | `true` |
| Cache background | `canvasCacheBackground` | `false` |
| Save painter state | `canvasPainterStateSave` | `false` |
| Edge highlighting | `canvasEdgeHighlighting` | `true` |

Do not hard-code scene rendering flags in `GraphicsWidget`'s constructor.
Apply them via the corresponding `slotOptionsCanvas*` methods, which read
from `appSettings` at startup.

## `edgesHash` keying

`GraphicsWidget::edgesHash` (mapping a graph edge to its `GraphicsEdge` scene item — UI-layer
only, unrelated to `Graph`'s own data model) is keyed by `GraphicsWidget::edgeKey()`, a `quint64`
packed as `relation * 10^16 + v1 * 10^8 + v2`: a 3-digit relation field and two 8-digit node-number
fields, read left-to-right like the old `"relation:v1>v2"` string it replaced. This is positional
encoding, not a mixing hash — collisions are structurally impossible as long as each field stays
under its digit budget (relation < 1000, node numbers < 100,000,000 each), enforced by
`Q_ASSERT_X` in `edgeKey()`. Practical consequence: a single loaded network is capped at
99,999,999 nodes — not a real-world limit (parsers always assign sequential `1..N` node numbers
regardless of a source file's own IDs, and a `QGraphicsView` canvas with anywhere near that many
interactive items is unusable long before the cap matters).

---

# Regression Safety Harness

SocNetV ships a headless CLI tool for deterministic regression testing:

```
socnetv-cli
```

Documentation: [`docs/SOCNETV_CLI_REGRESSION_TOOL.md`](SOCNETV_CLI_REGRESSION_TOOL.md)

Scripts:

```
scripts/run_golden_compares.sh
scripts/run_benchmarks.sh
scripts/run_golden_io_roundtrip.sh
scripts/run_io_roundtrip_shipped_datasets.sh
```

These must pass after any structural change.

---

# CLI Kernel Architecture

The harness is organized around **kernel modules**. Each kernel covers a specific algorithm family and emits a versioned, deterministic JSON schema. Schemas are never modified after release.

Current kernels:

```
kernel_distance_v1      — geodesic distances + centralities
kernel_reachability_v2  — reachability matrix
kernel_walks_v3         — walks matrix A^K
kernel_prominence_v4    — all node-level centrality + prestige indices
kernel_io_roundtrip_v5  — load → export → reload signature comparison
kernel_clustering_v6    — clustering coefficient, triad census, clique census
kernel_connectivity_v7  — weakly connected components count + per-node IDs
```

Each kernel owns its execution logic, JSON schema, and comparison logic.

---

# Logging

All debug-level logging uses Qt's category system (`qCDebug(category)`) — never plain `qDebug()`.
`qDebug()` unconditionally constructs a `QDebug` object and formats every argument before the
result is ever discarded; `qCDebug(category)` compiles to a check that skips argument evaluation
entirely when the category is disabled (~0.4 ns/call disabled vs. 618-2,193 ns/call for `qDebug()`,
depending on output destination — a ~1,500× difference). This matters most in hot loops:
`DistanceEngine`'s inner BFS/Dijkstra loops and its O(N²) `finalize()` pass were, at one point, the
single largest measured performance cost in the codebase, purely from unconditional `qDebug()`
formatting — see `roadmap_ws14_logging_cost.md` for the full numbers.

**Categories**: one per `src/graph/<domain>/` slice directory (`lcCentrality`, `lcDistances`,
`lcMatrices`, etc. — declared in `graph.h`, defined once in `graph.cpp`); file-local for other
standalone files (`lcMainWindow`, `lcEngine` for `distance_engine.cpp`, `lcMatrix` for
`matrix.cpp`); or a small shared header for files with too few calls each to justify their own
category (`lcParser` for all `src/parser/` files via `parser.h`; `lcForms` for `src/forms/*.cpp` via
a small `forms_logging.h`; `lcGW` shared by `GraphicsWidget` and the small canvas-item files via
`graphicswidget.h`).

**Release builds are quiet by default** — a no-flag launch disables all `*.debug` categories via
`QLoggingCategory::setFilterRules()`. `-d 1`/`-d 2` re-enable them at runtime — this is why
`QT_NO_DEBUG_OUTPUT` was never used instead: it compiles logging out entirely, with no way to turn
it back on for a bug report from a shipped build.

**Adding logging to a new file**: reuse the existing category for that `src/graph/<domain>/`
directory, or declare a new file-local one (`Q_LOGGING_CATEGORY(lcYourFile, "socnetv.yourfile")`)
following the existing pattern — never plain `qDebug()`.

---

# Doc-Comment Convention

Doxygen `@brief`/`@param`/algorithm-explanation comments live on the **`.cpp` definition**, not
the `.h` declaration. Headers (`graph.h`, `matrix.h`, ...) stay bare signature lists — this is
consistent across the whole `Graph`/`Matrix` API surface, not an accident of one file. Rationale:
keeps headers skimmable, and keeps the explanation physically next to the logic it describes, so a
change to an algorithm and its doc comment land in the same diff instead of drifting apart across
two files.

Where a measure's explanation is more than "what does this parameter mean" — every
centrality/prestige function's doc comment follows a fixed shape, in order: **Meaning** (what the
measure actually captures, in plain words), **When to use** (the concrete research situation it
fits), **Weights** (optional — only for measures that take `considerWeights`/`inverseWeights`:
whether this measure is shortest-path-based, where inverting a strength-type weight is correct
(a strong tie should act like a short/cheap path), or walk/matrix-based, where inverting is wrong
(it would make the strongest ties count least) — see `graph_distance_cache.cpp`'s BC/SC/EC/CC/PC
block and `graph_centrality_katz.cpp` for one example of each), **Compare to** (the other
measure(s) it's most easily confused with, and how it differs — skip this section only if there's
genuinely no close neighbor), then **Math** (the formula). See
`src/graph/centrality/graph_centrality.cpp` for the reference examples.

---

# Math Notation in Markdown Files (CHANGELOG.md, docs/*.md)

GitHub does **not** render bare `$ ... $` as inline math — that's a LaTeX/Astro-KaTeX convention,
not GitHub's. Left as bare `$ ... $`, GitHub falls through to plain Markdown parsing, where
underscores and carets inside the expression (e.g. `\lambda_{max}`) get misread as emphasis
markers, mangling the text instead of rendering a formula.

GitHub's actual supported syntax:
- Inline: `` $`...`$ `` (backtick-wrapped, no space after the opening backtick-dollar)
- Block: a fenced ` ```math ` code block, for anything long enough to warrant its own line(s)

---

# AddressSanitizer (ASan) Debug Builds

For chasing crashes/dangling-pointer bugs that don't show up in the golden/benchmark suite (those
exercise algorithm correctness, not memory safety) - a separate ASan-instrumented build catches
use-after-free, double-free, and heap-buffer-overflow bugs with an exact allocation/free stack
trace, rather than a bare crash address. Found and root-caused three real, previously-undetected
bugs this way during WS7 MW0 (dangling `graphicsWidget`/`scene` pointers and a `printerPDF`
double-free, all in `MainWindow::closeEvent` - see `roadmap_ws7_mainwindow_decomposition.md`).

**Build** (separate directory - keep it alongside the normal `build/`, never mix flags into it):

```bash
cmake -S . -B build-asan \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/macos/lib/cmake \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer -g" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address"
cmake --build build-asan -j$(nproc)
```

**Run headlessly** (fastest iteration loop - combine with `--interactive-script`, see
`roadmap_ws12_cli_scripting_mode.md`, to drive a specific scenario deterministically):

```bash
ASAN_OPTIONS="detect_leaks=0:abort_on_error=1:halt_on_error=1" \
QT_QPA_PLATFORM=offscreen \
./build-asan/SocNetV.app/Contents/MacOS/SocNetV --interactive-script my_repro.txt
```

`detect_leaks=0` matters on this codebase specifically: `MainWindow` is heap-allocated in
`main.cpp` and never explicitly deleted (the process just exits after `app.exec()` returns), so a
plain leak-check run reports the entire object graph as "leaked" noise on every single run -
real UAF/double-free/overflow bugs are what this build is for, not that.

**Run the real GUI** (needed for anything that only reproduces through actual window-manager
interaction, e.g. close/quit teardown ordering):

```bash
QT_QPA_PLATFORM=cocoa ASAN_OPTIONS=abort_on_error=1 ./build-asan/SocNetV.app/Contents/MacOS/SocNetV
```

**Reading a report**: `AddressSanitizer: heap-use-after-free`/`double-free` reports include both
the current (bad) access site and the original `freed by thread T0 here:`/`previously allocated by
thread T0 here:` stack traces with exact file:line - go straight to those two allocation sites, no
guessing needed. A `SEGV`/`DEADLYSIGNAL` report (no alloc/free trace) means the bad pointer wasn't
allocated through ASan's own tracked allocator at the time of the crash - for a live-register-level
root cause in that case, attach `lldb` directly (disable ASan's own signal handler first so `lldb`
gets the raw fault: `ASAN_OPTIONS=handle_segv=0:handle_sigbus=0:allow_user_segv_handler=1`), then
`bt`/`register read`/`disassemble --frame`/`memory read` at the fault to identify the actual
receiver object and compare its address against candidate objects captured via breakpoints earlier
in the same run.

---

# Development Workflow

## Build

CMake + Qt6 (Linux / macOS / Windows). Keep changes incremental.

## Regression discipline

After any structural change:

```
./scripts/run_golden_compares.sh
./scripts/run_benchmarks.sh
```

Golden outputs and performance must remain stable.

## Current focus

See [`docs/ARCHITECTURAL_REFACTORING_ROADMAP.md`](../ARCHITECTURAL_REFACTORING_ROADMAP.md)
for active workstreams, priorities, and the long-term architecture direction.

## Continuous Integration (GitHub Actions)

`.github/workflows/build-ci.yml` (triggered by `[ci]`/`[gha]` in a commit message on `develop`)
and `build-release.yml` (triggered by pushing a tag) both drive Qt via
[`jurplel/install-qt-action`](https://github.com/jurplel/install-qt-action) (aqtinstall), which is
**independent of the target distro's own Qt6** — Launchpad PPA builds (see below) and the OBS RPM
build (`socnetv.spec`, unversioned `pkgconfig(Qt6*)` requirements) both build against whatever
Qt6 the distro's own package repos provide, not anything from these workflows. Changing the
`qt-version` matrix here only affects: the CI compile-compatibility smoke test, and the actual
macOS DMG / Windows installer / Linux AppImage artifacts that `build-release.yml` publishes for
tagged releases.

`build-ci.yml` tests both **qmake** and **cmake** per OS (two separate steps, each gated only on
`matrix.os`) — this used to be piggybacked on having two `qt-version` matrix entries (older
version → qmake, newer → cmake), which is why the `if:` conditions on those steps look qt-version
gated in git history; they no longer are, since the matrix currently carries a single version.

If bumping the pinned Qt version: check `https://download.qt.io/official_releases/qt/<minor>/`
for open-source availability first — Qt's newest LTS patch releases are sometimes commercial-only
for a window before reaching the open-source channel that `aqtinstall` draws from.

---

# Launchpad PPA builds

```
Ubuntu 22.04 LTS (Jammy)
Ubuntu 24.04 LTS (Noble)
```
