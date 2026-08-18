# Report CSV Export (WS16)

## Goal

Let analysis reports be exported as CSV, not just HTML — issue #113: currently every
`Analyze → ...` report is an HTML file opened in the browser or the app's internal `TextEditor`
(some with charts/prose embedded), with no plain-tabular option for opening results in a
spreadsheet.

## Status

🚧 In progress. Steps 0-1 done (baseline benchmarking, matrix-family CSV export). Steps 2-3
(centrality/prestige CSV + dedup, long tail) not started.

## Background

A three-agent codebase investigation established the actual shape of the problem before any
design work:

- All ~27 report writers are `Graph::write*()` methods in
  `src/graph/reporting/graph_reports.cpp`, each opening a `QFile` and writing a full HTML
  document (`htmlHead` + content + `htmlEnd`, built once in the `Graph` constructor,
  `graph.cpp:159-275`). `MainWindow::slotAnalyze*` slots call these via the standard
  `runGraphOperationAsync` dispatch, then open the result via `QDesktopServices::openUrl()` or an
  internal `TextEditor`, gated by `appSettings["viewReportsInSystemBrowser"]`.
- **Matrix-family reports** (~15 report types: Adjacency, Distances, Geodesics, Reachability,
  Laplacian, Degree, Cocitation, Transpose, Adjacency Inverse, Walks, Similarity/Pearson/Matching,
  Dissimilarities) already funnel through one shared renderer, `Graph::writeMatrixHTMLTable()`
  (`:5959`), called from `writeMatrix()` (`:5548`, cleanly split into a compute phase and a render
  phase joined only by a `Matrix&` reference) and four sibling writers. This is the cheap slice —
  one new renderer function covers all of it.
- **Centrality/Prestige reports** (12 functions — `writeCentralityDegree`, `...Closeness`,
  `...ClosenessInfluenceRange`, `...Betweenness`, `...Stress`, `...Eccentricity`, `...Power`,
  `...Information`, `...Eigenvector`, `writePrestigeDegree`, `...Proximity`, `...PageRank`) have
  **no shared table renderer** — each hand-rolls an identical 5-column
  (Node/Label/Raw/Normalized/%Normalized) `<table>` scaffold, complete with copy-pasted
  `tableSort()` JS wiring and isolate-drop placeholder logic. Harder slice — needs a genuinely new
  shared renderer, which also deduplicates 12x copy-pasted HTML as a side effect.
- **No headless path reaches any `write*()` function except `--interactive-script`.**
  `socnetv-cli`'s kernels only ever emit JSON. The `distances` interactive command
  (`mainwindow.cpp:6175-6208`) was the only existing example of a report writer benchmarked
  end-to-end (compute + HTML file I/O) before this workstream.
- CSV escaping already exists and should be reused: `TableExport::toCSV(QAbstractItemModel*,
  path)` (`src/graph/io/table_export.cpp:27-76`) has a private `csvQuote()` helper (quote-only-
  if-needed) used today for the Data Table dock's node/edge export. Matrix reports never need it
  (row/column headers are always plain vertex numbers); centrality/prestige reports do, since node
  labels are free text.
- Settings: `DialogSettings`' existing `reportsGroupBox` (`src/forms/dialogsettings.ui`, inside
  `generalTab`) already has 3 report-output controls (label length, real-number precision, chart
  type), each wired `DialogSettings` signal → `MainWindow` connect → a `Graph::setReportsX()`
  setter storing a `Graph`-private member. A 4th control (output format) fits this exact groupbox
  and wiring pattern.

**Design decisions**: one global "Report output format" setting (HTML default / CSV), not
per-report toggles; CSV output is lean (table only, no prose/summary-stats/chart — a report with a
chart just skips that step for CSV); CSV escaping reused via a new `TableExport` overload, never
reimplemented; `MainWindow` decides the file extension and always opens CSV via
`QDesktopServices::openUrl()` (the internal `TextEditor`'s plain-text mode isn't a useful CSV
view); `writeMatrixAdjacencyPlot()` (glyph-based visual plot) is excluded — not tabular data;
purely narrative/single-value reports (Connectedness, Node/Graph Connectivity's κ) stay HTML-only.

## What WS16 Delivered

### Step 0 — Baseline benchmarking infrastructure

Since `--interactive-script` is the only headless path that can reach a `Graph::write*()`
function, and nothing benchmarked the centrality/prestige family before this:

- New interactive-script command `report-centrality-degree [weights] [dropisolates]`
  (`mainwindow.cpp`), mirroring `slotAnalyzeCentralityDegree()` exactly, same shape as the
  existing `distances` command — the first centrality/prestige report ever exercised headlessly.
- New script `scripts/run_report_export_bench.sh`, modeled on `scripts/run_render_perf_bench.sh`
  (the WS6.6 precedent for GUI-driven, `--interactive-script`, `QT_QPA_PLATFORM=offscreen`
  benchmarks `run_benchmarks.sh`'s CLI-kernel-only design can't reach). Deliberately a rough
  capture-and-print tool, not a CI-threshold-gated regression kernel like the render-perf one —
  formal threshold-gating can follow later if this becomes a standing regression concern.
- Two fixtures: `scripts/fixtures/report_export_bench_small.txt` (N=500, E=2500) and
  `..._large.txt` (N=2000, E=40000, matching the existing render-perf fixture's scale).

**Baseline numbers** (median of 5 runs, macOS arm64, Debug build, offscreen):

| Fixture | `distances` (writeMatrix, HTML) | `report-centrality-degree` (writeCentralityDegree, HTML) |
|---|---|---|
| small (N=500, E=2500) | 1166 ms | 106 ms |
| large (N=2000, E=40000) | 27092 ms | 776 ms |

Note: `distances`' timing is dominated by APSP computation (DistanceEngine), not the HTML-writing
step itself — the two are not separated in this measurement. Later steps should expect these
end-to-end numbers to stay essentially flat (compute unchanged) rather than looking for a large
drop; the CSV path's own timing (once it exists) is the more informative comparison.

### Step 1 — Matrix-family CSV export

- New `Graph::writeMatrixCSVTable()` (`graph_reports.cpp`), sibling to `writeMatrixHTMLTable()` —
  same value-formatting rules (RAND_MAX sentinel, #266 large-magnitude scientific notation,
  precision), comma-delimited, vertex-number header row, no escaping needed.
- New `ReportFormat { Html, Csv }` enum (`global.h`), threaded as a parameter through
  `writeMatrix()`, `writeMatrixWalks()`, `writeMatrixDissimilarities()`,
  `writeMatrixSimilarityMatching()`, `writeMatrixSimilarityPearson()`, and `writeMatrixAdjacency()`
  — compute phase untouched in every case, CSV branch is self-contained and returns early.
  `writeMatrixAdjacency()` doesn't reuse `writeMatrixCSVTable()` (it computes cells live via
  `edgeExists()`, not from a pre-built `Matrix`, to preserve node numbers after deletions) — has
  its own small comma-delimited loop instead.
- New Settings control: `reportsGroupBox` in `DialogSettings` gained a 4th row ("Output format",
  HTML/CSV combo), wired via the same `DialogSettings` signal → `MainWindow` connect →
  `Graph::setReportsOutputFormat()` pattern as the 3 existing Reports settings.
  `appSettings["initReportsOutputFormat"]` persists it (default `"0"` = HTML).
- **Bug found and fixed along the way**: `writeMatrixAdjacency()` and `writeMatrixWalks()` both
  returned `void`, unlike every other writer in this family. Their 3 `MainWindow` call sites
  (`slotNetworkViewSociomatrix`, `slotAnalyzeWalksLength`, `slotAnalyzeWalksTotal`) gated the
  "open the report" step on `activeGraph->progressCanceled()` alone — which only reflects the user
  clicking Cancel, not a file-open failure. A silently failed write (bad path, permissions, disk
  full) was reported to the user as "saved" even though nothing was written. Both functions now
  return `bool`; all 3 call sites check the real success flag.
- All 14 `MainWindow` call sites updated: pick `.html`/`.csv` extension from the setting, pass
  `format` through, CSV always opens via `QDesktopServices::openUrl()`. The `distances`
  interactive-script command gained a `csv` token (explicit, not settings-read, since a script has
  no Settings dialog) so it can't silently diverge from the real menu action.

**Verification**: `./scripts/run_golden_compares.sh` clean (Phase A/compute untouched everywhere).
Manual content check: `distances weights csv` on an 8-node network produced a correct
comma-delimited, symmetric distance matrix with a 0 diagonal, matching the HTML sibling's values
exactly. Live GUI check of the new Settings control caught and fixed a real bug (see below) before
it shipped. Benchmark re-run (`run_report_export_bench.sh`) confirmed the HTML path is unchanged
within normal run-to-run noise (small: 1166→1243ms, 776→767ms; large: 27092→27481ms,
776→767ms — all within the ~5-10% variance already present run-to-run). A single-run CSV-vs-HTML
`distances` comparison on the large fixture (28991ms CSV vs. 24495ms HTML) isn't a real signal
either way — both fall inside that same variance band, and this benchmark measures APSP compute
time (identical in both branches), not report-writing cost specifically; isolating the write-only
cost would need a different measurement, not attempted here.

**Settings UI bug found and fixed during manual verification**: the `.ui` file defined the new
combo box's two items (`HTML`, `CSV`) *and* the constructor called `addItems()` with the same two
strings — the combo showed four duplicate entries at runtime. Fixed by removing the static `.ui`
items, matching the existing `reportsChartTypeSelect` pattern (items added purely in code).

## What Remains Open

- **Step 2 — Centrality/Prestige CSV export + dedup.** New shared per-node score-table renderer
  pair (`writeScoreTableHTML()`/`writeScoreTableCSV()`) replacing the 12x copy-pasted table
  scaffold; new `TableExport::toCSV(headers, rows, path)` overload for label escaping; retrofit
  12 `MainWindow` call sites; benchmark commands for the remaining 11 writers.
- **Step 3 — Long tail.** Reciprocity, Eccentricity, Connectedness, Node/Graph Connectivity,
  Clique Census, Triad Census, Clustering Coefficient, Hierarchical Clustering — case-by-case once
  Steps 1-2's two renderer patterns exist; some map onto the score-table pattern, some need their
  own small fixed-shape CSV, some (Connectedness, κ values) likely stay HTML-only permanently.

## Work Rules

- `./scripts/run_golden_compares.sh` after every step (no computational change expected — confirm,
  don't assume, since C++ code is touched).
- `scripts/run_report_export_bench.sh` before and after each step, compared against the Step 0
  baseline above.
- Manual GUI smoke test each step: run affected reports in both HTML/CSV mode, confirm file
  contents and that the correct viewer opens.
- CSV escaping goes through `TableExport`, never reimplemented independently in
  `graph_reports.cpp`.
