# Report CSV Export (WS16)

## Goal

Let analysis reports be exported as CSV, not just HTML — issue #113: currently every
`Analyze → ...` report is an HTML file opened in the browser or the app's internal `TextEditor`
(some with charts/prose embedded), with no plain-tabular option for opening results in a
spreadsheet.

## Status

📋 Scoped, Step 0 (baseline benchmarking) done. Steps 1-3 (matrix-family CSV, centrality/prestige
CSV + dedup, long tail) not started.

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

## What Remains Open

- **Step 1 — Matrix-family CSV export.** New `Graph::writeMatrixCSVTable()` sibling to
  `writeMatrixHTMLTable()`; thread a `ReportFormat` parameter through `writeMatrix()` and its 4
  sibling writers plus `writeMatrixAdjacency()`; new Settings control
  (`Graph::m_reportsOutputFormat`/`setReportsOutputFormat()`); update ~15 `MainWindow` call sites
  and the `distances` interactive-command duplicate.
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
