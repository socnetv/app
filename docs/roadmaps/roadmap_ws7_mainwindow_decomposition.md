# MainWindow Decomposition Roadmap (WS7)

**Tracked as:** [#257](https://github.com/socnetv/app/issues/257)

## Goal

Reduce `MainWindow` from a 16,289-line `.cpp` (plus 838-line `.h`) into a thin coordinator that
delegates UI concerns to focused sub-controllers and panel widgets, while preserving all existing
behavior and user-visible functionality.

**This is a maintainability/testability workstream, not a performance one** — stated explicitly
rather than left implicit. Checked directly: none of the ~304 `appSettings[...]` lookups scattered
through `mainwindow.cpp` occur inside a per-node/per-edge loop or a repaint path (grepped for loop
context around every occurrence); the scatter is a real maintenance cost, not a measured
performance one. If a genuine performance finding turns up during MW1-MW7 execution, it gets its
own evidence and completion criteria the same way WS3/WS5 findings did — none is assumed here.

## Status

📋 Scoped (MW1–MW7 milestone plan exists), not started. One related finding (progress-dialog
duplication + unwrapped GUI-blocking operations) moved to WS15 — see Background below.

## Background

### Motivation

After WS2, `Graph` is a clean façade. The UI side has not yet received the same treatment.
Confirmed by direct reading:

* `MainWindow` owns all menus, toolbars, dock widgets, status bar, and dialogs.
* ~235 slots follow the `slotXxx` naming convention in `mainwindow.h` (one `public slots:` block,
  lines 151-586).
* `statusBar()->showMessage(...)` / `statusMessage(...)` is called **359 times** directly.
* `appSettings[...]` is accessed **304 times** directly, across ~37 different settings-related
  methods — no single owner.
* 20 distinct `Dialog*` classes are instantiated directly with `new` (21 call sites) — no shared
  lifecycle management.
* `initMenuBar()` is 448 lines; `initToolBar()` is 84 lines — both inline in `MainWindow`.
* 14 direct `connect(graphicsWidget, ...)` call sites wire canvas interactions into `MainWindow`.

This makes it hard to test UI behavior, hard to reason about responsibilities, and expensive to
change any single feature.

### Non-Goals

* No behavior changes.
* No visual/UX changes.
* No Qt version changes.
* No new features.
* No dependency on WS3, WS4, or WS5 completion (WS7 is structurally independent).

### Prerequisites

* WS2 complete (Graph façade stable). ✅
* CLI headless baseline exists (WS4/P2), to anchor regression during UI changes.
* WS9 complete (UI stabilized) ✅ — this was the stated gate for starting WS7; it's now satisfied,
  so WS7 is unblocked in principle. Priority ranking (see `ARCHITECTURAL_REFACTORING_ROADMAP.md`)
  is a separate question from whether it's blocked.

### Guiding Principles

* Decompose by **responsibility**, not by line count.
* Each extracted sub-controller or panel must compile independently.
* No slot or signal signature changes during extraction (pure movement).
* Golden + benchmark comparisons must pass after each milestone.
* MainWindow remains the single owner of the `Graph` instance throughout WS7.

### Target Shape (End State)

```
MainWindow  (thin coordinator, ~2000 lines)
  │
  ├── AppMenuController       — menu bar construction + action routing
  ├── AppToolbarController    — toolbar management
  ├── StatusBarController     — status bar updates
  │
  ├── CanvasPanel             — GraphicsWidget host + canvas interactions
  ├── LeftPanel               — node/edge property panel
  ├── RightPanel / StatsPanel — analysis results + chart display
  │
  ├── DialogManager           — dialog instantiation + lifecycle
  └── AppSettingsController   — settings persistence + apply logic
```

MainWindow wires these together but does not own their internal logic.

### Related Finding: Progress-Dialog Duplication & Unwrapped GUI-Blocking Operations

Found while scoping WS5 A5 (cancellation-aware algebra kernels) — a behavioral finding, not a pure
structural extraction, and it turned out to be tangled up with a broader cancellation/threading
problem bigger than either WS5 or WS7's own scope. **Moved to
[`roadmap_ws15_cancellation_progress_unification.md`](roadmap_ws15_cancellation_progress_unification.md)
(P3)** — the full call-site survey (Group A double-dialog sites, Group B unwrapped/GUI-blocking
sites) lives there now, alongside the cancellation-delivery fix and crash-bug fix it shares root
cause with.

### Sequencing Note

WS7 is independent of WS3, WS4, and WS5. Its stated prerequisite (UI stabilized post-WS9) is now
satisfied — WS9 shipped. It remains unstarted only because of relative priority, not a blocker;
see `ARCHITECTURAL_REFACTORING_ROADMAP.md`'s Priorities list. MW1's remaining per-slot
categorization can begin any time.

## What WS7 Delivered

Nothing shipped yet. MW1's slot-count inventory below (the six bullets under Motivation) is
preliminary scope-sizing only, not the full per-slot responsibility-bucket categorization MW1
itself requires — see What Remains Open.

## What Remains Open

### MW1 — Audit and Categorize MainWindow Slots (partially done as part of this design pass)

The counts in Background/Motivation above (~235 slots, 359 status-bar calls, 304 settings
accesses, 20 dialog classes, 14 canvas connects, menu/toolbar line counts) are MW1's inventory
output. Full per-slot responsibility-bucket categorization (Graph-state reaction / UI-state
reaction / dialog launcher / settings apply / canvas interaction / analysis display) is the
remaining work before MW2 starts — the counts above scope *how much* of each category exists;
assigning every individual slot to its bucket is the actual MW1 deliverable, not yet done.

Definition of Done:

* A documented slot inventory exists (comment block in `mainwindow.h` or a separate doc), covering
  every one of the ~235 `slotXxx` methods.
* No code changes. Build passes.

### MW2 — Extract StatusBarController

**Objective:** first small extraction — low risk, high signal. 359 confirmed call sites
(`statusBar()->showMessage(...)` / `statusMessage(...)`) is the concrete scope.

Deliverables:

* `StatusBarController` class owns all status bar update logic.
* MainWindow constructs it and passes it relevant signals.
* All 359 call sites routed through it.

Definition of Done:

* MainWindow no longer calls `statusBar()` directly except in initialization.
* Golden + benchmarks pass. No behavior change.

### MW3 — Extract AppMenuController

**Objective:** separate menu construction from business logic. Concrete scope: `initMenuBar()`
(448 lines, `mainwindow.cpp`) and `initToolBar()` (84 lines).

Deliverables:

* `AppMenuController` owns menu bar construction and action object creation.
* Actions are still connected to MainWindow slots (no slot moves yet).
* Menu structure is driven by controller, not inline `MainWindow::initMenuBar()`.

Definition of Done:

* Menu construction code no longer lives in `MainWindow::initMenuBar()`/`initToolBar()`.
* Actions remain triggerable; no UX change. Golden + benchmarks pass.

### MW4 — Extract DialogManager

**Objective:** centralize dialog lifecycle management. Concrete scope: 20 distinct classes, 21
`new Dialog*` call sites (`DialogClusteringHierarchical`, `DialogDataSetSelect`,
`DialogDissimilarities`, `DialogEdgeDichotomization`, `DialogEdgeEdit`, `DialogExportImage`,
`DialogExportPDF`, `DialogFilterEdgesByWeight`, `DialogNodeFind`, `DialogPreviewFile`,
`DialogQueryBuilder`, `DialogRandErdosRenyi`, `DialogRandLattice`, `DialogRandRegular`,
`DialogRandScaleFree`, `DialogRandSmallWorld`, `DialogSettings`, `DialogSimilarityMatches`,
`DialogSimilarityPearson`, `DialogSystemInfo`, `DialogWebCrawler`).

Deliverables:

* `DialogManager` owns instantiation, `exec()`/`show()`, and cleanup of all 20 dialog classes.
* MainWindow calls `m_dialogManager->openNodeEditDialog(...)` instead of `new DialogEdgeEdit(...)`
  inline, etc.
* DialogManager receives the minimum context it needs (Graph façade reference, parent widget).

Definition of Done:

* No `new Dialog*` calls remain directly in MainWindow slot bodies.
* All 20 dialogs still open and function correctly. Golden + benchmarks pass.

### MW5 — Extract AppSettingsController

**Objective:** isolate settings persistence and apply logic. Concrete scope: `appSettings =
initSettings(...)` (`mainwindow.cpp:137`), `saveSettings()` (`mainwindow.cpp:667`), and ~37
other settings-related methods, 304 direct `appSettings[...]` accesses total.

Deliverables:

* `AppSettingsController` owns read/write of `QSettings` (via `initSettings()`/`saveSettings()`).
* Owns the "apply settings to Graph + UI" logic currently scattered across the ~37 settings
  methods.
* MainWindow calls `m_settingsController->apply(...)` on startup and settings dialog close.

Definition of Done:

* No direct `appSettings[...]` accesses remain in MainWindow outside the controller.
* Settings dialog still functions correctly. Golden + benchmarks pass.

### MW6 — Extract CanvasPanel

**Objective:** encapsulate canvas interactions. Concrete scope: 14 `connect(graphicsWidget, ...)`
call sites in `mainwindow.cpp`.

Deliverables:

* `CanvasPanel` wraps `GraphicsWidget` and owns canvas-level slot handling: zoom, fit, pan
  signals/slots; node/edge click/hover reactions; canvas resize handling.
* MainWindow holds a `CanvasPanel*` and forwards Graph signals to it.

Definition of Done:

* None of the 14 `connect(graphicsWidget, ...)` sites remain directly in MainWindow.
* Canvas interactions unchanged. Golden + benchmarks pass.

### MW7 — Reduce MainWindow to Coordinator

**Objective:** ensure MainWindow is now a wiring layer only.

Deliverables:

* MainWindow connects sub-controllers to Graph signals and to each other.
* No display logic, no dialog construction, no settings reads remain inline.
* Slot count in MainWindow reduced to coordinator-level wiring only.
* `mainwindow.cpp` target: under 3,000 lines (down from 16,289).

Definition of Done:

* All prior golden + benchmark comparisons pass.
* `mainwindow.h` / `mainwindow.cpp` reflect coordinator role clearly.
* Responsibility boundaries documented.

## Work Rules

* Sub-controllers must not access `Graph` internals directly — only via the façade API.
* Sub-controllers may receive `Graph*` (façade) as constructor parameter.
* Sub-controllers must not own the `Graph` instance.
* New files live under `src/ui/controllers/` and `src/ui/panels/`.
* No new signals/slots added to `Graph` during WS7.
* For every milestone: golden metric comparisons must pass (algorithms are not touched, but load
  flow must stay intact); performance benchmarks must remain within tolerance; manual smoke test —
  load a dataset, run a centrality, open a dialog, export a report.
