# MainWindow Decomposition Roadmap (WS7)

**Tracked as:** [#257](https://github.com/socnetv/app/issues/257)

## Goal

Make `MainWindow`'s implementation navigable and fast to build incrementally by physically
splitting its monolithic `.cpp` into focused per-responsibility files under `src/mainwindow/` —
mirroring WS2/F2's mechanical split of `graph.cpp`. **Not a class-decomposition effort**:
`MainWindow` remains exactly one class, same signatures, same behavior, same `mainwindow.h`.
(Originally scoped as a bigger controller-extraction effort — MW1–MW7, see Investigated and
Deferred below — that turned out not to be needed for the actual problem: file size and
navigability, not testability or coupling. No standing plan to revisit that.)

## Status

✅ Complete. `mainwindow.cpp`: 18,540 → 588 lines (constructor + destructor only, matching
`graph.cpp`'s post-F2 shape). ~19,100 lines across 28 files under `src/mainwindow/`.

## Background

### Motivation

After WS2, `Graph` is a clean façade split across `src/graph/`. The UI side had not received the
same treatment: `MainWindow` owned all menus, toolbars, dock widgets, status bar, and dialogs, all
implemented in one 18,540-line `.cpp`. Confirmed by direct reading before starting: ~235 slots
follow the `slotXxx` naming convention; `statusBar()->showMessage(...)`/`statusMessage(...)` is
called 359 times directly; `appSettings[...]` is accessed 304 times across ~37 methods; 20 distinct
`Dialog*` classes are instantiated directly with `new`; `initMenuBar()` alone was 448 lines. This
made the file expensive to navigate and slow to incrementally rebuild — the concrete, felt pain
that prompted this workstream.

### Non-Goals

* No behavior changes, no visual/UX changes, no Qt version changes, no new features.
* No new classes — `MainWindow` stays one class throughout.

### Related Finding: Progress-Dialog Duplication & Unwrapped GUI-Blocking Operations

Found while scoping WS5 A5 (cancellation-aware algebra kernels) — a behavioral finding, not a pure
structural extraction, and it turned out to be tangled up with a broader cancellation/threading
problem bigger than either WS5 or WS7's own scope. **Moved to
[`roadmap_ws15_cancellation_progress_unification.md`](roadmap_ws15_cancellation_progress_unification.md)
(P3)** — the full call-site survey lives there now, alongside the cancellation-delivery fix and
crash-bug fix it shares root cause with.

## What WS7 Delivered

### MW0 — Mechanical File Split (F2-style)

Same shape as WS2/F2: `mainwindow.h` untouched throughout (single 870-line header); method bodies
moved verbatim (no internal restructuring) into new files under `src/mainwindow/<domain>/`, grouped
by the existing `slotXxx` naming convention (which already mirrors the menu structure). Landed in 6
commits, one per directory/batch, each independently build-verified, golden-compared, and
smoke-tested (headless `--interactive-script` + a full ASan pass) before moving to the next:

| Directory | Files | What moved |
|---|---|---|
| `lifecycle/` | 1 | `closeEvent`, `terminateThreads`, `resizeEvent` |
| `init/` | 5 | Startup: settings load/save, all `QAction` construction, menu/toolbar, dock panels, remaining `init*()` |
| `network/` | 5 | Network menu: file I/O, import/export, random generation, web crawler, view |
| `edit/` | 6 | Edit menu: node, edge, selection/drag-mode, relation, filters, 3 shared helpers |
| `analyze/` | 8 | Analyze menu: centrality/prestige, distance, matrices, cohesion, clustering, structural equivalence, prominence, shared edge-weights prompt |
| `dispatch/`, `scripting/`, `layout/`, `options/`, `help/` | 1 each | WS15 async-dispatch helpers, WS12 interactive-script dispatcher, Layout menu, Options/Settings, Help menu |

Two real, pre-existing bugs were found and fixed along the way (not introduced by the move itself —
confirmed via `lldb`/AddressSanitizer against unmodified `develop` HEAD in both cases):

* **`closeEvent` dangling-pointer crashes**: `graphicsWidget` and `scene` were explicitly deleted
  mid-handler while Qt's own window-close teardown continues dispatching events to them immediately
  afterward, in the same close sequence. Neither delete served a purpose beyond what the OS already
  reclaims at exit (`MainWindow` itself is heap-allocated in `main.cpp` and never explicitly
  destructed). A related crash, `editNodePropertiesAct`/`editNodeRemoveAct` deleted while still
  attached to a live toolbar, was fixed the same way.
* **`printerPDF` double-free**: `slotNetworkExportPDF()` deleted the member pointer internally
  without nulling it, so `closeEvent()`'s own delete on quit double-freed it on any session that
  had exported a PDF at least once.

One process finding: two overloaded methods (`runGraphOperationAsync`, and the 4 `slotLayout*`
`...ByProminenceIndex` methods) were initially only half-moved by the extraction script — its
dedup logic correctly discards a same-name match that's just a self-referencing `qCDebug` log
line, but incorrectly discarded a second match that was a genuine C++ overload. Caught by a
post-split sweep for leftover `MainWindow::` definitions in `mainwindow.cpp`; both stranded
overloads moved to sit beside their sibling.

**AddressSanitizer debug builds** (used throughout to verify each slice) are now documented as a
standing tool in `README_DEVELOPER_NOTES.md`, not a one-off — configure/build/run instructions and
how to read a report.

## Investigated and Deferred

**MW1–MW7 (controller/panel extraction)**: the original plan carved `MainWindow` into
`StatusBarController`, `AppMenuController`, `DialogManager`, `AppSettingsController`,
`CanvasPanel`, and a thin coordinator `MainWindow`. Considered again once MW0 landed — same
conclusion the project already reached for `Graph` in WS3 (see `roadmap_ws3...md`'s M3/M4): the
stated, concrete pain was file size/navigability, which MW0 fully addresses. Controller extraction
would buy testability/decoupling with no current concrete blocker behind it, in a codebase that has
no unit-test framework to make "testability" pay off directly — speculative architecture, not a
measured problem. Deferred indefinitely; revisit only if a concrete need appears (e.g. a real bug
traced to tangled responsibilities, or an actual testability blocker).

## What Remains Open

Nothing actively scoped.
