# MainWindow Decomposition Roadmap (WS7)

## Goal

Reduce `MainWindow` from a 15,000-line monolith into a thin coordinator that
delegates UI concerns to focused sub-controllers and panel widgets,
while preserving all existing behavior and user-visible functionality.

---

## Motivation

After WS2, `Graph` is a clean façade. The UI side has not yet received the same
treatment. `MainWindow` currently:

* owns all menus, toolbars, dock widgets, status bar, and dialogs
* handles all user action slots (~200+ slots)
* wires signals from `Graph` directly into UI updates
* performs layout and panel management directly
* contains display logic, chart config, and settings persistence

This makes it hard to test UI behavior, hard to reason about responsibilities,
and expensive to change any single feature.

---

## Non-Goals

* No behavior changes.
* No visual/UX changes.
* No Qt version changes.
* No new features.
* No dependency on WS3 or WS4 completion (WS7 is structurally independent).

---

## Prerequisites

* WS2 complete (Graph façade stable). ✅
* CLI headless baseline exists (WS4/P2), to anchor regression during UI changes.

---

## Guiding Principles

* Decompose by **responsibility**, not by line count.
* Each extracted sub-controller or panel must compile independently.
* No slot or signal signature changes during extraction (pure movement).
* Golden + benchmark comparisons must pass after each milestone.
* MainWindow remains the single owner of the `Graph` instance throughout WS7.

---

## Target Shape (End State)

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

---

## Milestones

---

### MW1 — Audit and Categorize MainWindow Slots

**Objective:** understand what lives in MainWindow before touching anything.

Deliverables:

* Categorize all public/private slots by responsibility bucket:
  * Graph state reactions (graph loaded, vertex added, etc.)
  * UI state reactions (toolbar toggle, panel visibility, etc.)
  * Dialog launchers
  * Settings apply
  * Canvas interactions
  * Analysis result display
* Identify which slots are purely UI (no Graph call) vs. coordinator (calls Graph façade).
* Note any slots that do too much (mixed UI + Graph + display).

Definition of Done:

* A documented slot inventory exists (can be a comment block in `mainwindow.h` or a separate doc).
* No code changes.
* Build passes.

---

### MW2 — Extract StatusBarController

**Objective:** first small extraction — low risk, high signal.

Deliverables:

* `StatusBarController` class owns all status bar update logic.
* MainWindow constructs it and passes it relevant signals.
* All `statusBar()->showMessage(...)` and related calls routed through it.

Definition of Done:

* MainWindow no longer calls `statusBar()` directly except in initialization.
* Golden + benchmarks pass.
* No behavior change.

---

### MW3 — Extract AppMenuController

**Objective:** separate menu construction from business logic.

Deliverables:

* `AppMenuController` owns menu bar construction and action object creation.
* Actions are still connected to MainWindow slots (no slot moves yet).
* Menu structure is driven by controller, not inline MainWindow code.

Definition of Done:

* Menu construction code no longer lives in `MainWindow::setupMenuBar()` or equivalent.
* Actions remain triggerable; no UX change.
* Golden + benchmarks pass.

---

### MW4 — Extract DialogManager

**Objective:** centralize dialog lifecycle management.

Deliverables:

* `DialogManager` owns instantiation, `exec()`/`show()`, and cleanup of all dialog classes.
* MainWindow calls `m_dialogManager->openNodeEditDialog(...)` instead of `new DialogNodeEdit(...)` inline.
* DialogManager receives the minimum context it needs (Graph façade reference, parent widget).

Definition of Done:

* No `new Dialog*` calls remain directly in MainWindow slot bodies.
* All existing dialogs still open and function correctly.
* Golden + benchmarks pass.

---

### MW5 — Extract AppSettingsController

**Objective:** isolate settings persistence and apply logic.

Deliverables:

* `AppSettingsController` owns read/write of `QSettings`.
* Owns the "apply settings to Graph + UI" logic currently scattered in MainWindow.
* MainWindow calls `m_settingsController->apply(...)` on startup and settings dialog close.

Definition of Done:

* No direct `QSettings` calls remain in MainWindow outside of initialization.
* Settings dialog still functions correctly.
* Golden + benchmarks pass.

---

### MW6 — Extract CanvasPanel

**Objective:** encapsulate canvas interactions.

Deliverables:

* `CanvasPanel` wraps `GraphicsWidget` and owns canvas-level slot handling:
  * zoom, fit, pan signals/slots
  * node/edge click/hover reactions
  * canvas resize handling
* MainWindow holds a `CanvasPanel*` and forwards Graph signals to it.

Definition of Done:

* GraphicsWidget-related slots no longer live in MainWindow directly.
* Canvas interactions unchanged.
* Golden + benchmarks pass.

---

### MW7 — Reduce MainWindow to Coordinator

**Objective:** ensure MainWindow is now a wiring layer only.

Deliverables:

* MainWindow connects sub-controllers to Graph signals and to each other.
* No display logic, no dialog construction, no settings reads remain inline.
* Slot count in MainWindow reduced to coordinator-level wiring only.
* `mainwindow.cpp` target: under 3,000 lines.

Definition of Done:

* All prior golden + benchmark comparisons pass.
* `mainwindow.h` / `mainwindow.cpp` reflect coordinator role clearly.
* Responsibility boundaries documented.

---

## Regression Discipline

For every milestone:

* Golden metric comparisons must pass (algorithms are not touched, but load flow must stay intact).
* Performance benchmarks must remain within tolerance.
* Manual smoke test: load a dataset, run a centrality, open a dialog, export a report.

---

## Work Rules

* Sub-controllers must not access `Graph` internals directly — only via the façade API.
* Sub-controllers may receive `Graph*` (façade) as constructor parameter.
* Sub-controllers must not own the `Graph` instance.
* New files live under `src/ui/controllers/` and `src/ui/panels/`.
* No new signals/slots added to `Graph` during WS7.

---

## Sequencing Note

WS7 is independent of WS3, WS4, and WS5. It can proceed in parallel after WS4/P2.
MW1 (audit) can begin immediately.
