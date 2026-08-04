# Cancellation & Progress-Dialog Unification Roadmap (WS15)

## Goal

- Make cooperative cancellation actually interrupt a running computation, not just compile.
- Fix a live, reproducible crash caused by graph mutation racing an active background computation.
- Retire the redundant/incomplete linear `QProgressDialog` system in favor of the indeterminate one.

## Status

🚧 In progress. P1 and P2 ✅ done and live-verified. P3 not started. Split off from WS5 (A5's live
cancellation test) and WS7 (the progress-dialog-duplication finding) once both turned out to be the
same underlying problem, bigger than either workstream's own scope. No GitHub issue filed —
everything here is unreleased-cycle (3.7) behavior, not a regression against a shipped version, per
standing project convention.

## Context

WS5's A5 gave `Matrix::inverse()`/`powerIteration()` an optional `cancelCheck` callback, wired to
`[this]{ return progressCanceled(); }` at their three real call sites. Live GUI testing (large
network, Information/Eigenvector Centrality, click Cancel mid-computation) showed it doesn't
actually interrupt anything — the operation always runs to completion, then a report opens anyway.
Investigating why surfaced two separable problems, plus a related structural finding from WS7's own
scoping pass:

1. **The cancel flag can't be delivered while it's needed** (P1 below) — a signal-delivery/threading
   bug, not a `Matrix` bug.
2. **A live, reproducible crash**: graph mutation (e.g. "New") racing an in-flight background
   computation, exposed by the same threading model (P2 below) — found while testing (1), confirmed
   pre-existing via `git stash` A/B testing, unrelated to any change made this cycle.
3. **Two redundant progress-dialog systems coexist**, one of them silently leaving ~20 operations
   both unresponsive to Cancel *and* still blocking the GUI thread outright (P3 below) — found while
   tracing A5's call sites for WS7.

**Ordering note:** P2 must land before P3's Group B migration (not just before P3 by priority — a
real dependency). Group B's ~20 operations currently run synchronously on the GUI thread, which
makes them slow but *immune* to P2's race (you can't click "New" while the GUI thread itself is busy
running one of them). Migrating them onto `runGraphOperationAsync` first would make them fast and
newly racy, before the guard that's supposed to prevent that race exists. P2 first closes the race
for everything already on `graphThread` today; P3's Group B migration is then safe by construction.

## Milestones

### P1 — Atomic flag + `Qt::DirectConnection` (make cancellation delivery work) ✅ Done

**Root cause:** `Graph::m_progressCanceled` (`graph.h:1523`) is a plain `bool`, set by
`Graph::slotCancelComputation()` (`src/graph/ui/graph_ui_facade.cpp`). The Cancel button's
`canceled()` signal is connected to it with Qt's default auto-resolved connection type — since the
dialog lives on the GUI thread and `Graph` lives on `graphThread` (see `Graph::moveToThreadFacade()`),
that resolves to a **queued** connection. A queued slot only runs when the receiving thread's event
loop is free to process its queue — but `graphThread`'s event loop is exactly what's blocked for the
whole duration of `Matrix::inverse()`/`powerIteration()`'s loop (dispatched there via
`QMetaObject::invokeMethod(..., Qt::QueuedConnection)`). Net effect: `m_progressCanceled` cannot
become `true` until the loop has already finished on its own — the same problem A5 set out to fix,
just moved one level down. Confirmed live: every successful-looking cancel actually landed at an
earlier, pre-existing *coarse* check (e.g. right after `createMatrixAdjacency()`), never inside
`inverse()`/`powerIteration()` itself.

**Rejected approach:** pumping `QCoreApplication::processEvents()` inside `progressCanceled()` so
`graphThread`'s queue gets a chance to drain without waiting for the whole operation. Perf-verified
clean, but unsafe: `processEvents()` is only safe to call from the thread that owns `QApplication`
when real window/modal state is involved — calling it from `graphThread` let real GUI input reach
the app while the wait cursor was still showing, breaking the `Qt::ApplicationModal` dialog's
blocking guarantee. Abandoned outright, fully reverted.

**Actual fix:**

- `Graph::m_progressCanceled` (`graph.h:1523`) → `std::atomic<bool>` (add `#include <atomic>`).
- `Graph::progressCanceled()`, `Graph::slotCancelComputation()`, `Graph::resetProgressCanceled()`
  (`src/graph/ui/graph_ui_facade.cpp`) — unchanged shape, now atomic load/store.
- Both `connect(..., &QProgressDialog::canceled, activeGraph, &Graph::slotCancelComputation)` sites
  in `mainwindow.cpp` (`slotProgressBoxCreate()` at `:15634-15635`, `runGraphOperationAsync()` at
  `:15732-15733`) gain an explicit `Qt::DirectConnection` 4th argument. A direct cross-thread
  connection executes the slot **synchronously on the emitting (GUI) thread**, at click-time — no
  queueing, no dependency on `graphThread`'s event loop being free. Safe here because the slot body
  is just an atomic store, nothing GUI-thread-owned.

**Extension found necessary during live verification:** the first live test (atomic flag +
`Qt::DirectConnection` only, `ludcmp()` untouched) still didn't work — Cancel set the flag correctly,
but `inverse()`'s own per-column `cancelCheck` is only reached *after* `ludcmp()` returns, and
`ludcmp()` (the one-time O(n³) decomposition, dominant cost of `inverse()`) had zero cancellation
awareness at all. At N=1,000 with `-d min` (heavy per-element `qCDebug` logging in `ludcmp()`'s inner
loop), that's long enough to matter — confirmed live: cursor stayed busy, no cancel log line, and a
concurrent "New" (P2's repro) put the app into the live-race state before the process was killed
(no crash that time, but the race window was real - `Graph::clear()` had already run on the GUI
thread while `graphThread` was still inside `ludcmp()`). Fixed by giving `ludcmp()` (`matrix.cpp`,
`matrix.h`) its own `cancelCheck` parameter, checked once per outer-loop iteration in both its O(n²)
scaling pass and its O(n³) Crout's-method pass, forwarded from `inverse()`'s own parameter. A
canceled `ludcmp()` returns `false` — identical to its existing singular-matrix return — so
`inverse()` cannot and does not try to tell the two apart from that return value; both
`createMatrixAdjacencyInverse()` and `centralityInformation()` already check `progressCanceled()`
directly after the call (I2's existing fix), which is what actually disambiguates it.

**Live-verified** (N=1,000, `1000actors-10000arcs.graphml`, `-d min`): Information Centrality,
Cancel clicked shortly after starting — log showed `Matrix::ludcmp() - canceled at Crout column 1`
within a fraction of a second, cursor returned to normal, app usable again (confirmed by actually
adding a node, not just eyeballing the cursor). Eigenvector Centrality: cancel also worked (app
stayed responsive, no delayed report), though `powerIteration()`'s loop converges fast enough on
this network that the log line itself is hard to catch in the act — timing artifact, not a
correctness gap; `powerIteration()`'s own `cancelCheck` was already unaffected by this change.

**Remaining scope limits** (still real gaps, not part of P1):

- `Matrix::solve()` (used by `layoutForceDirected()`'s stress majorization, `graph_layouts_force.cpp:754`)
  also calls `ludcmp()`, but wasn't part of A5's original wiring and isn't passed a `cancelCheck`
  here either — same uninterruptible-`ludcmp()` exposure, different call site. Not fixed now; small,
  same-shaped follow-up if it turns out to matter in practice.
- Does **not** extend to `DistanceEngine`'s parallel BFS (`QtConcurrent::blockingMap`), which
  deliberately skips cancel checks inside worker threads (see `distance_engine.cpp:485-486`'s own
  comment, performance-motivated). An atomic flag *would* be safe to read from those worker threads
  (unlike `processEvents()`), so this is a plausible future extension — not proposed here.

**Verification:** `run_golden_compares.sh` / `run_benchmarks.sh` — clean, no regression (both after
the atomic-flag change and again after the `ludcmp()` extension). Live GUI retest — done, see above.

### P2 — Global "graph busy" guard (crash fix) ✅ Done

**Not caused by anything in this cycle's cancellation work** — reproduced identically (crash
signature, thread roles) on builds with and without the P1 `processEvents()` prototype (2 crash
reports each, 4 total, via `git stash`/`git stash pop` A/B testing). Introduced when long
computations moved onto `graphThread` via `runGraphOperationAsync` (#254); 3.6 only had the old
fully-synchronous/linear-dialog model, where the GUI thread was blocked for the operation's whole
duration, structurally preventing this race from ever being reachable.

**Repro (confirmed reliable, 4+ times across two builds):**

1. Load a large network (`2000actors-40000edges.graphml` or similar — needs several real seconds of
   compute).
2. Trigger a long computation (Betweenness Centrality confirmed; Information Centrality also
   implicated, see near-miss below).
3. Click Cancel while it's computing.
4. Click/trigger "New Network" (toolbar or Ctrl+N) shortly after.
5. Crashes — or, once, entered a live dangerous state before eventually crashing (see near-miss).

**Root cause:** `DistanceEngine`'s parallel BFS never checks cancellation inside its worker threads
(deliberate, performance-motivated, `distance_engine.cpp:485-486`), so Cancel only sets a flag — the
background computation keeps running regardless. Separately, `MainWindow::slotNetworkNew()` →
`slotNetworkClose()` → `Graph::clear()` runs **directly on the GUI thread**, with no dispatch to
`graphThread` and no guard against a computation being in flight — confirmed from the crash
backtrace (no `QMetaObject::invokeMethod` frame between `QToolButton`/`QAction` click handling and
`Graph::clear()`). If that runs while a worker thread is still iterating the same
`GraphVertex`/`QMultiHash` data being deleted, that's a genuine use-after-free race.

Crash signature (identical across all 4 `.ips` reports, `EXC_BAD_ACCESS`/`SIGSEGV`): a `Thread
(pooled)` worker inside `DistanceEngine::bfsSSSP()`, iterating a per-vertex `QMultiHash`, while
thread 0 (confirmed real GUI thread — backtrace shows `QToolButton::mouseReleaseEvent` →
`QAction::activate` → `slotNetworkNew()`) is inside `Graph::clear()` → `GraphVertex::~GraphVertex()`
→ that same hash type's `clear()`/`freeData()`.

**Near-miss (single-threaded case, no `QtConcurrent` involved):** reproduced separately with
Information Centrality (`Matrix::inverse()`/`ludcmp()`, pure `graphThread` work). User waited for
`ludcmp()` to start, clicked Cancel (no effect — `ludcmp()` had no cancellation check at the time,
since fixed by P1), then pressed Ctrl+N. Canvas went empty (`Graph::clear()` completed on the GUI
thread) **while the console kept printing `ludcmp()` log lines** — `graphThread` was still actively
reading/writing `WM` (a `Graph` member) *after* `Graph::clear()` had already torn it down. Live
use-after-free in progress, confirmed by observation. Confirms the danger isn't specific to the
`QtConcurrent` parallel path — any long-running `graphThread` computation is exposed. Reproduced
again, independently, even after P1 landed (P1 makes cancellation land fast when it lands in time,
but doesn't stop a user from mutating the graph before clicking Cancel at all — a different failure
mode this guard is meant to close).

**Open question, not resolved:** exactly how "New" reaches `slotNetworkNew()` at all while the
progress dialog should still be `Qt::ApplicationModal`. Checked: the New action has no
`Qt::ApplicationShortcut`-style modal-bypass override (default `Qt::WindowShortcut`), so this isn't
a deliberate bypass by design. Root mechanism unclear — the fix below doesn't require understanding
it, so not chased further for now.

**Fix:** `MainWindow::setAppBusy(bool busy)` (`mainwindow.cpp`, declared `mainwindow.h`) — one
choke point, called at the top of `runGraphOperationAsync()` (`busy=true`, alongside
`setOverrideCursor()`) and in its completion lambda (`busy=false`, alongside
`restoreOverrideCursor()`). Disables `menuBar()`, `toolBar` (`mainwindow.h:676`), and
`graphicsWidget` (`mainwindow.h:646`) — the last one covers add-node/add-edge via direct mouse
interaction (`GraphicsWidget::mousePressEvent`/`mouseDoubleClickEvent`), which bypasses menu/toolbar
entirely. Single point of control: covers every current `runGraphOperationAsync` call site (Group A)
automatically, and every future one (Group B, once P3 migrates them) by construction — no per-action
audit needed, and no dependency on understanding why the modal dialog's own blocking has a gap.

**Resolved open questions:**
- Mechanism: the dedicated helper, not per-action flag checks (as above).
- UX: confirmed live — grayed-out menu/toolbar/canvas reads as "busy," consistent with the existing
  modal dialog + wait cursor, not "frozen."
- Canvas scope: included, same mechanism, zero extra cost.
- Multi-window: N/A, `MainWindow` has no multi-window support today.

**Live-verified:** menu, toolbar, and canvas (including double-click-to-add-node and the Ctrl+N
shortcut specifically) all confirmed unresponsive while a computation is running, in both the
"click Cancel first" and "don't click Cancel at all" variants of the original repro; all three
re-enable immediately on completion. `run_golden_compares.sh` / `run_benchmarks.sh` clean.

### P3 — Retire the linear progress-dialog system

Two systems currently coexist in `MainWindow`:

- **Linear** — `Graph::progressCreate()`/`progressUpdate()`/`progressFinish()` →
  `signalProgressBoxCreate`/`Update`/`Kill` → `MainWindow::slotProgressBoxCreate()`/
  `slotProgressBoxDestroy()` (`mainwindow.cpp:5282-5286`). A real `QProgressDialog` with a numeric
  0..N range, gated on `appSettings["showProgressBar"]` (defaults `"true"`, enabled out of the box).
- **Indeterminate** — `MainWindow::runGraphOperationAsync()` (`mainwindow.cpp:15726`). Its own local
  `QProgressDialog` in `min=max=0` ("busy") mode, created per wrapped operation. Established faster
  and is the one to standardize on.

**Call-site survey** (every `Graph::` method calling `progressCreate()`, checked against whether its
`MainWindow` entry point uses `runGraphOperationAsync`):

- **Group A — double-dialog (~13 sites), delete the redundant internal calls.** Already wrapped in
  `runGraphOperationAsync`, so these currently show both dialogs stacked: `writeCentralityBetweenness/
  Eccentricity/Eigenvector/Information/Power/Stress`, `writeEccentricity`,
  `writeMatrixSimilarityMatching`, all three `layoutForceDirected*`, `layoutByProminenceIndex` (4
  call sites). No behavior loss — the indeterminate dialog already covers them. Low risk.
- **Group B — linear-only (~20 sites), migrate onto `runGraphOperationAsync` first.**
  `writeCentralityDegree/Closeness/ClosenessInfluenceRange`, `writeCliqueCensus`,
  `writeClusteringCoefficient/Hierarchical`, `writePrestigeDegree/PageRank/Proximity`,
  `writeTriadCensus`, `writeMatrixAdjacency/AdjacencyPlot`, `writeReciprocity`,
  `verticesCreateSubgraph` (4 sites), every `randomNet*` generator (6 methods, several sites each),
  `layoutEgoRadial/RadialRandom/Random`. None of these go through `runGraphOperationAsync` today —
  called directly, synchronously, from `MainWindow` slots. The linear dialog is currently their
  *only* progress feedback, **and** they still block the GUI thread during computation — the same
  class of problem #254 found and `runGraphOperationAsync` was built to fix, just never applied to
  this list. Migrating them is a real behavior fix (unblocks the GUI thread), not pure cleanup.
  **Must wait until P2 lands** — see the Ordering note above; migrating these before the busy guard
  exists would newly expose them to P2's race.

**Sequencing:** Group A first (pure deletion, no dependency on P1/P2). Group B needs P2 landed
first, then each site migrated individually, golden/benchmark-verified per site, same discipline as
any other `runGraphOperationAsync` migration. Once Group B is done, the linear system
(`slotProgressBoxCreate`/`slotProgressBoxDestroy`/`signalProgressBox*`/`progressCreate`/
`progressUpdate`/`progressFinish`) has no remaining callers and can be deleted outright.

## Known Issues carried over from WS5

**I1 — Matrix algebra methods are not cancellation-aware.** ✅ Resolved by P1. A5 shipped the
`cancelCheck` plumbing in `Matrix::inverse()`/`powerIteration()`; P1 made the flag delivery actually
work and extended `cancelCheck` into `ludcmp()`, the piece that was still silently uninterruptible.
Live-verified. Remaining exposure: `Matrix::solve()`'s own `ludcmp()` call (see P1's "Remaining scope
limits") and `DistanceEngine`'s parallel BFS — both explicitly out of scope, not "still open by
omission."

**I2 — `createMatrixAdjacencyInverse()`'s mid-computation cancel check.** ✅ Resolved by P1, same
basis as I1 — live-verified via Information Centrality's cancel test.

## Work Rules

- P1 is a pure infrastructure fix (behavior-preserving when nothing is canceled) — golden/benchmarks
  must show zero change, same discipline as A5.
- P2 got explicit design sign-off before implementation, per the project's plan-before-code rule —
  it was a UX-visible change, not a pure internal fix.
- P3's Group A deletions and Group B migrations are each independently golden/benchmark-verified,
  one call site (or small batch) at a time — not one large diff. Group B can now start; P2 has landed
  (see the Ordering note above).
- No GitHub issue for any of this (unreleased 3.7-cycle behavior) — fix directly.
- Once P1-P3 land: add a changelog entry, update WS5's A5 section and WS7's status line to point
  here instead of duplicating content (both already do).
