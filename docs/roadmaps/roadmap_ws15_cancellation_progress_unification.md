# App Responsiveness Contract (WS15)

Dispatch, Cancellation, Busy-Guard & Parallelization

## Goal

Make "the app is responsive" a checkable property instead of a recurring, never-quite-finished
fix. `CHANGELOG.md:816-821` records #52 as a "comprehensive fix across all computation paths" for
cancel/progress in v3.4 — real work, wired into centrality, prestige, reachability, walks, matrix,
report, layout, clique, subgraph, all the random generators — and it's still what silently regressed
under `runGraphOperationAsync`'s queued-connection bug (P1, below) until this cycle. A second
"comprehensive fix" that turns out just as narrow would repeat that exact mistake. So: define
responsiveness as four independent, per-operation properties, and audit every long-running `Graph::`
operation against all four, rather than fixing whatever's currently visible and calling it done.

**The contract — four properties, checked independently per operation:**

1. **Non-blocking dispatch** — wrapped in `runGraphOperationAsync` (or equivalent), so the GUI
   thread's event loop stays free during the operation.
2. **Working cancellation** — a `cancelCheck` wired at genuinely fine-grained points (not a coarse
   phase boundary that never fires in practice), using the atomic-flag + `Qt::DirectConnection`
   mechanism (P1).
3. **Busy-guard coverage** — menu/toolbar/canvas disabled for the duration, via `setAppBusy()` (P2).
   Automatic once (1) is true; not independently implementable.
4. **Internally parallelized where the algorithm structure allows it** — `QtConcurrent`, following
   `DistanceEngine`'s APSP precedent, for the actual compute-heavy inner loop. Not every algorithm
   qualifies (e.g. LU decomposition is inherently sequential column-by-column); the audit's job is
   to say which do and how, not to assume all should be. Betweenness centrality (Brandes) is
   **already** effectively parallelized today, since it consumes APSP's output directly — not a gap
   to fix, a fact to record correctly in the audit so it isn't mis-flagged.

An operation can satisfy any subset of these independently and that can be a legitimate, honest
state (e.g. `Matrix::inverse()` today: (1) ✅ (2) ✅ (3) ✅ (4) ❌ — not parallelized, and that's fine
to say plainly rather than pretend otherwise).

## Status

🚧 In progress. P1 (property 2) and P2 (property 3) ✅ done and live-verified. P3 (property 1 —
retire the linear dialog, close the dispatch gap): Phase 1 (Group C, 18 call sites) ✅ done and
live-verified 2026-08-06; Phases 2-5 not started. P4 (parallelization audit across all long-running
`Graph::` operations, not just the `progressCreate()`-calling subset P3 covers) audit ✅ done (found
3 more zero-protection operations, folded into P3's Group C; full per-operation property 2/4 status
recorded) — implementation not started, not yet decided which findings to act on. Split off
from WS5 (A5's live cancellation test) and WS7 (the progress-dialog-duplication finding), then
reorganized around the 4-property contract (2026-08-05) once it became clear "dispatch" and "compute
parallelism" were being treated as one bundled fix when they're actually orthogonal. No GitHub issue
filed — everything here is unreleased-cycle (3.7) behavior, not a regression against a shipped
version, per standing project convention.

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

### P3 — Retire the linear progress-dialog system + close the "no protection at all" gap

Two progress-reporting systems currently coexist in `MainWindow`:

- **Linear** — `Graph::progressCreate()`/`progressUpdate()`/`progressFinish()` →
  `signalProgressBoxCreate`/`Update`/`Kill` → `MainWindow::slotProgressBoxCreate()`/
  `slotProgressBoxDestroy()` (`mainwindow.cpp:5282-5286`). A real `QProgressDialog` with a numeric
  0..N range, gated on `appSettings["showProgressBar"]` (defaults `"true"`, enabled out of the box).
- **Indeterminate** — `MainWindow::runGraphOperationAsync()` (`mainwindow.cpp:15726`). Its own local
  `QProgressDialog` in `min=max=0` ("busy") mode, created per wrapped operation, plus P2's
  `setAppBusy()` guard. Established faster and is the one to standardize on.

A third, undocumented category was found while scoping this (see Group C below): several
filter operations have **neither** system — no dialog, no `setAppBusy()` guard, nothing.

**Verified call-site audit** (full re-audit via a dedicated research pass, superseding the earlier
~13/~20 estimate — mapped every `progressCreate()` call site to its enclosing `Graph::` method, then
every `MainWindow` caller of that method, then whether each caller path is wrapped in
`runGraphOperationAsync`):

**Group A — double-dialog, unambiguous, safe to deduplicate directly (14 methods).** Every call
path into these is wrapped, so deleting their internal `progressCreate()`/`progressUpdate()`/
`progressFinish()` calls is pure redundancy removal, no behavior change:
`writeCentralityBetweenness/Eccentricity/Eigenvector/Information/Power/Stress` (`graph_reports.cpp`),
`writeEccentricity`, `writeMatrixSimilarityMatching` (also has its own bug, see Finding 3),
`layoutForceDirectedSpringEmbedder/FruchtermanReingold/KamadaKawai` (`graph_layouts_force.cpp`),
`layoutByProminenceIndex` (`graph_layouts_basic.cpp:354`, 4 `MainWindow` entry points, all wrapped).
Also effectively in this group despite having no *direct* `MainWindow` caller: `writeMatrix()`'s
nested `graphMatrixShortestPathsCreate`/`createMatrixReachability` calls — every one of the 9
`writeMatrix()` call sites in `MainWindow` is wrapped.

**Progress (2026-08-07), Phase 2 done:** ✅ All of Group A —
`writeCentralityBetweenness/Eccentricity/Eigenvector/Information/Power/Stress`, `writeEccentricity`,
`writeMatrixSimilarityMatching` (Finding 3's double-fire bug fixed as a direct side effect — both
its own and `createMatrixSimilarityMatching()`'s `progressCreate()` triads removed),
`layoutForceDirectedSpringEmbedder/FruchtermanReingold/KamadaKawai`, `layoutByProminenceIndex`,
`graphMatrixShortestPathsCreate`, `createMatrixReachability`. The latter two also had a mid-loop
`progressCanceled()` cancel-check whose branch called `progressFinish()` before returning — the
`progressFinish()` was removed (it paired with the now-deleted `progressCreate()`), but the
`progressCanceled()` check and early return themselves were kept, since they're the actual
cancellation behavior, not dialog bookkeeping. Each method verified individually:
`run_golden_compares.sh` clean after every method; full `run_golden_compares.sh` +
`run_benchmarks.sh` + `run_golden_io_roundtrip.sh` pass clean at the end.

**Non-obvious per-method care needed, recorded so the same mistake isn't made twice:** several of
these methods have an *earlier*, unrelated `progressCanceled()`/`progressFinish()` pair before
their own `progressCreate()` — cleaning up a dialog opened by an ancestor call (e.g.
`graphDistancesGeodesic()`), not this method's own. Only the triad that actually pairs with *this*
method's own `progressCreate()` gets removed; the ancestor-cleanup calls stay untouched.
`layoutForceDirectedKamadaKawai`'s loop counter (`progressCounter`) is dual-purpose - also its
iteration-count guard, not just a display value - so the variable and its increment stay; only the
`progressUpdate()` call itself was removed.

**Infrastructure fix found necessary before any Group A removal could be safe:**
`resetProgressCanceled()` was previously called from exactly two places in the whole codebase -
`Graph::progressCreate()` and `DistanceEngine`'s progress sink - and nowhere in
`MainWindow::runGraphOperationAsync()` itself. Removing Group A's `progressCreate()` calls would
therefore have removed the *only* reset point for any operation whose chain has no other one -
meaning a single earlier cancel would silently no-op every subsequent wrapped operation's
`cancelCheck()` forever after. This was a latent risk in **Phase 1's Group C wrapping too**, not
just a Phase 2 concern. Fixed centrally: `runGraphOperationAsync()` now calls
`activeGraph->resetProgressCanceled()` itself, first thing, before the busy dialog is even created
- covers every current and future wrapped operation, not just Group A's.

**Group A-tangled — same double-dialog problem, but wrapping is inconsistent per call path, not
per method (7 methods, needs its own design pass, not mechanical deletion).** `prestigeDegree`,
`prestigeProximity`, `prestigePageRank`, `centralityInformation`, `centralityEigenvector`,
`centralityDegree`, `centralityClosenessIR` are each reached from 3 places — their own `write*`
report (wrapping varies by method), `verticesCreateSubgraph()` (always unwrapped), and
`layoutByProminenceIndex()` (always wrapped) — so a per-method fix isn't enough; see Finding 1.
`createMatrixAdjacency` is its own, worse case (Finding 2). `writeMatrixWalks` has two `MainWindow`
entry points with *different* wrapping for the same nested calls (Finding 5).

**Group B — linear-only, migrate onto `runGraphOperationAsync` (21 methods, consistently
unwrapped).** `writeReciprocity`, `writeCentralityDegree/ClosenessInfluenceRange`,
`writePrestigeDegree/PageRank/Proximity`, `writeClusteringCoefficient`, `writeTriadCensus`,
`writeCliqueCensus`, `writeClusteringHierarchical`, `writeMatrixAdjacency/AdjacencyPlot`,
`verticesCreateSubgraph` (4 `MainWindow` entry points, all unwrapped),
`randomNetErdosCreate/ScaleFreeCreate/SmallWorldCreate/RegularCreate/RingLatticeCreate/LatticeCreate`
(the real dialog-driven slot only — `randomNetErdosCreate`'s other two "callers" are the
`--interactive-script` `erdos` benchmark harness, which deliberately bypasses
`runGraphOperationAsync` for timing precision and is out of scope here), `layoutRandom/
RadialRandom/EgoRadial`. None of these go through `runGraphOperationAsync` today — called directly,
synchronously, from `MainWindow` slots. The linear dialog is currently their *only* progress
feedback, **and** they still block the GUI thread during computation — the same class of problem
#254 found and `runGraphOperationAsync` was built to fix, just never applied to this list.
**Must wait until P2 has landed** (✅ it has) — migrating these before the busy guard existed would
have exposed them to P2's race; safe to start now.

**Group C — no protection at all ✅ Done (18 call sites total, see below).**
`MainWindow::slotFilterNodesDialogByCentrality/BySelection/ByEgoNetwork/ByAttribute`,
`slotFilterByQueryBuilder`, `slotFilterNodesRestoreAll`, `slotEditFilterNodesIsolates`,
`slotEditFilterEdgesByWeightDialog/Reset/Unilateral` (all `mainwindow.cpp`, ~12130-12480) call
straight into `activeGraph->vertexFilterBy*()`/`edgeFilterBy*()` with **no** `progressCreate()`,
**no** `runGraphOperationAsync`, **no** `setAppBusy()` guard. Confirmed live:
`vertexFilterByEgoNetwork()` on a N=2,000/E=20,000 network took **minutes**, with only macOS's own
"unresponsive app" beachball as feedback (not a Qt wait cursor — Qt shows none here at all).
Added during the 3.5/3.6 filtering feature work, after `runGraphOperationAsync` existed, but never
wired into it. Same GUI-thread-blocking problem as Group B, just never even got the linear dialog's
partial mitigation.

**Group C additions, found by P4's audit (2026-08-05) — same "zero protection" shape, different
files, not reachable from a `progressCreate()`-based search:**
- `Graph::addRelationSymmetricStrongTies` (`graph_relations.cpp:303`) — via
  `MainWindow::slotEditEdgeSymmetrizeStrongTies()`. No dispatch wrap, no progress dialog, no cancel
  check anywhere in the method.
- `Graph::relationAddCocitation` (`graph_relations.cpp:389`) — via
  `MainWindow::slotEditEdgeSymmetrizeCocitation()`. No dispatch wrap; its only cancel check is
  borrowed from a nested `createMatrixAdjacency()` call — its own O(N²) edge-creation loop has none.
- `Graph::subgraphExtract`/`subgraphExtractFromSelection` (`graph_subgraphs.cpp:41,164`) — via
  `MainWindow::slotEditSubgraphExtract()`/`slotEditSubgraphExtractFromSelection()`. No dispatch wrap,
  no progress feedback, no cancel check. Likely lower urgency in practice (usually runs on a small
  selected/visible subset), but unbounded on "extract all visible nodes" for a large network.

**Group C additions, found live during implementation (2026-08-06) — 4 more, invisible to both the
`progressCreate()`-based P3 audit and the P4 agent's method-name-based sweep, since none of these
are named `MainWindow::slotXxx()` methods:**
- `FilterBarWidget::chipCloseRequested` handler (`mainwindow.cpp:5446`) — anonymous lambda
  registered inline in `initView()`, calls `activeGraph->vertexFilterRemoveAt()` directly.
- `FilterBarWidget::clearAllRequested` handler (`mainwindow.cpp:5464`) — same shape, a `while`
  loop over `activeGraph->vertexFilterRestoreAll()`. This is the "Clear all" button specifically —
  confirmed live: no indeterminate dialog appeared, only the OS busy cursor, exactly matching
  Group C's original symptom.
- `MainWindow::slotEditNodeSetPropertyForSelection()`/`slotEditEdgeSetPropertyForSelection()`
  (`mainwindow.cpp:10392,10442`) — the outer functions are named slots (so visible to a
  method-name search), but the actual `activeGraph->` mutation happens inside an anonymous
  `DialogBulkEdit::userChoices` lambda nested inside them — invisible to a search that only reads
  function signatures, not bodies.

**Lesson recorded for any future audit of this shape**: search by call-site pattern
(`activeGraph->someMethod(...)` not inside `runGraphOperationAsync`/`QMetaObject::invokeMethod`),
not by enclosing function name — anonymous `connect(..., this, [this](...){...})` lambdas are a
real, recurring blind spot for name-based sweeps. A final broad sweep of this kind was run
after Phase 1's implementation and found nothing further.

**Live verification (2026-08-06):** dispatch/busy-guard mechanism confirmed working correctly —
menu/toolbar/canvas grey out, indeterminate dialog shows, app stays interactive-but-blocked for the
whole duration, re-enables on completion. One real surprise: an `--interactive-script` test
(`filter_ego` on the actual `2000actors-40000edges.graphml` file) measured the `Graph::`-side
computation itself at 50-140ms — but live GUI testing of the same operation (real node, real file)
took **7-8 real seconds**, with nodes disappearing quickly and edges lingering visibly before
clearing. The gap is per-item Qt signal emission/canvas-repaint cost (`setVertexVisibility`/
`signalSetEdgeVisibility`, one per node/edge) happening *after* the timed `Graph::` call returns —
not captured by a script-side timer, real live-GUI-only cost. This is expected and *fine* for P3's
own scope: the point was staying responsive with real feedback during that whole 7-8s, which it
did. *Why* it's still slow is a property-4 question for a future pass, not a P3 regression.

**Two pre-existing bugs found during live verification, unrelated to WS15** (confirmed present in
3.6, before any of this cycle's changes): Statistics Panel's edge count is wrong both during and
after node filtering (Focus on Node / Focus on Selection). Filed as
[#270](https://github.com/socnetv/app/issues/270), milestone 3.7, assigned to the user — not fixed
here, since it reproduces identically on the untouched, fully-synchronous `clearAllRequested` path
too, ruling out any connection to this cycle's dispatch changes.

**Findings from the audit** (numbered for reference elsewhere):

| # | Finding |
|---|---|
| 1 | `prestigeDegree/Proximity/PageRank`, `centralityInformation/Eigenvector/Degree/ClosenessIR` are each reached from 3 call paths (own report / `verticesCreateSubgraph` / `layoutByProminenceIndex`) with different wrapping per path — needs per-call-path handling, not a per-method flag. |
| 2 | `createMatrixAdjacency` fans out to ~15 callers across 4 files (`graph_reports.cpp`, `graph_centrality.cpp`, `graph_relations.cpp`, `graph_reachability_walks.cpp`) with mixed wrapping and no `updateProgress`-style gate. Largest blast radius of any single method here — needs its own isolated pass. |
| 3 | `writeMatrixSimilarityMatching` double-fires the linear dialog: `createMatrixSimilarityMatching`'s own `progressCreate()` (`graph_similarity_matrices.cpp:65`) fires, then its own `progressCreate(1, pMsg)` (`graph_reports.cpp:5083`) fires again — one click on "Compute Similarity Matrix" shows 1 busy dialog + 2 sequential linear dialogs. Real bug, not just redundancy. |
| 4 | `Graph::vertexinfluenceRange()`/`vertexinfluenceDomain()` (`graph_reachability_walks.cpp:250,312`) have **zero callers anywhere in `src/`** — dead code, not merely nested-only. Confirm with user whether intentionally reserved before touching. |
| 5 | `writeMatrixWalks` has two `MainWindow` entry points (`slotAnalyzeWalksLength` unwrapped, `slotAnalyzeWalksTotal` wrapped) reaching the same nested `graphWalksMatrixCreate()` progress calls — same method, different wrapping depending on which UI action triggered it. |

**Sequencing/phasing, agreed 2026-08-05:**

1. **Group C first ✅ Done** (18 call sites total: 10 original filter slots + 3 P4-discovered + 4
   found live during implementation, listed above) — highest user-visible pain today (multi-minute
   freeze, zero feedback), and structurally the same fix shape as Group B (wrap in
   `runGraphOperationAsync`). Live-verified 2026-08-06, `run_golden_compares.sh`/`run_benchmarks.sh`
   clean.
2. **Group A ✅ Done** — pure deletions, no dependency on anything, lowest risk. All 14 methods
   done, verified, see progress note above. Live-verified 2026-08-07,
   `run_golden_compares.sh`/`run_benchmarks.sh`/`run_golden_io_roundtrip.sh` clean.
3. **Group A-tangled** (Findings 1, 2, 5) — each needs its own small design decision per call path,
   not mechanical deletion. Do after the easy wins are banked.
4. **Group B** — migrate one method at a time onto `runGraphOperationAsync`, golden/benchmark-verified
   per site.
5. **Side items**: fix Finding 3's double-fire bug; decide with user whether Finding 4's dead code
   gets deleted; once Group B is done, the linear system
   (`slotProgressBoxCreate`/`slotProgressBoxDestroy`/`signalProgressBox*`/`progressCreate`/
   `progressUpdate`/`progressFinish`) has no remaining callers and can be deleted outright.

Phase 1 (Group C) ✅ done, live-verified. Phases 2-5 not started.

### P4 — Parallelization audit (property 4)

**Scope, wider than P3's:** P3's Group A/A-tangled/B/C tables cover every `progressCreate()` call
site — but that's the set of operations someone already thought to wire *some* progress feedback
into, not the full set of long-running `Graph::` operations. Filters (Group C) proved that set is
incomplete. P4 needs its own audit pass across `src/graph/`'s algorithm slices (`centrality/`,
`clustering/`, `distances/`, `reachability/`, `cohesion/`, `similarity/`, `layouts/`, `generators/`,
`matrices/`, `reporting/`, `filters/`), not reused from P3's list, covering:

- Every operation's status against **all four** contract properties (not just parallelization in
  isolation — dispatch/cancel/guard may also be missing for operations P3's `progressCreate()`-based
  survey never saw).
- For property 4 specifically: real algorithmic judgment per operation, not a grep pass — does the
  algorithm have independent per-source/per-node/per-partition work (parallelizable, following
  `DistanceEngine`'s APSP precedent) or an inherently sequential dependency chain (not a good
  candidate, e.g. LU decomposition's column-by-column pivoting)? Record known-already-parallel cases
  correctly (Brandes' betweenness centrality consumes APSP's output directly — already effectively
  parallelized, not a gap) rather than mis-flagging them as unaddressed.
- Concrete suggestions per operation that's a real parallelization candidate, not just a yes/no flag.

**Audit done (2026-08-05), implementation not started.** Legend: P2 = **fine** (checked inside the
hot loop, genuinely per-iteration), **coarse** (only at phase boundaries — a mid-loop cancel does
nothing), **none**. P4 = parallelized already / good candidate + why / poor candidate + why. P3
inherits P1 exactly (busy-guard is automatic once dispatch is wrapped), not tabulated separately.

**`src/graph/centrality/`:**

| Method | P1 | P2 | P4 |
|---|---|---|---|
| `centralityInformation` | A-tangled | Mixed: coarse boundary checks; WM-build loop (O(n²)) has none; the dominant O(n³) `inverse()` call is fine-grained (P1) | Inverse itself: poor (sequential LU). WM-build loop: good candidate, low payoff (dwarfed by O(n³) inverse) |
| `centralityEigenvector` | A-tangled | Coarse boundaries; `powerIteration()` call itself fine-grained (P1) | Outer convergence loop sequential, but each iteration's mat-vec sweep is Jacobi-style parallelizable (good candidate) |
| `centralityDegree` | A-tangled | **Coarse only** — before the O(N²) loop, never inside it | **Good candidate, clear win** — per-vertex independent, same shape as APSP |
| `centralityClosenessIR` | A-tangled | Fine (per outer iteration) | Good candidate — reads only cached `apspDistance()`, per-vertex independent |
| `prestigeDegree`/`prestigeProximity` | A-tangled | Fine | Good candidate — per-vertex, read-only |
| `prestigePageRank` | A-tangled | **Worst gap in this file**: coarse before/after only — the entire convergence `while` loop (`graph_prestige.cpp:427-542`) has zero checks inside it | Outer loop sequential (each iteration needs previous), but the per-vertex update within one iteration is Jacobi-style — good candidate |

**`src/graph/distances/`, `src/graph/matrices/`:**

| Method | P1 | P2 | P4 |
|---|---|---|---|
| `graphDistancesGeodesic` (→ `DistanceEngine`) | delegates | Already parallel (`QtConcurrent::blockingMap` over sources); worker threads deliberately skip cancel checks (documented, perf-motivated) | **Already done** — this is the APSP precedent itself. Betweenness/Brandes rides along in the same `blockingMap` pass — confirmed already effectively parallelized, not a gap |
| `graphMatrixShortestPathsCreate`/`createMatrixReachability` | A (nested) | Fine | Good candidate — O(N²) row-fill reading cached APSP output, per-row independent |
| `graphMatrixDistanceGeodesicCreate` | A (nested) | **None** (documented as intentional — "fast enough" comment, but zero-awareness for very large N) | Good candidate, same shape |
| `createMatrixAdjacency` | A-tangled (Finding 2, ~15 callers) | Fine | Good candidate — per-row, reads only `edgeExists()` |
| `createMatrixAdjacencyInverse` | (nested) | Coarse boundaries; `inverse()` itself fine-grained (P1) | Poor (sequential LU) |

**`src/graph/reachability/`:** `graphWalksMatrixCreate` — A (nested, inconsistent per Finding 5); fine for the `length==0` power-loop, **none** for the `length>0` single `AM.pow()` call. `Matrix::pow`'s individual multiplies are row/column-independent (good candidate per-multiply), but the power chain itself is sequential (poor candidate for the chain).

**`src/graph/clustering/`:**

| Method | P1 | P2 | P4 |
|---|---|---|---|
| `clusteringCoefficient` | B | Fine | **Good candidate** — pure per-vertex, no shared-state writes |
| `graphTriadCensus` | B | **Coarse only on outer loop** — O(N³) nested loops (the dominant cost) have zero checks; one outer iteration alone is O(N²) uninterruptible | **Good candidate** — outer loop independent, needs a per-thread histogram + reduction merge |
| `graphClusteringHierarchical` | B | Fine (per merge step) | **Poor candidate** — inherently sequential agglomerative merging, same shape as LU decomposition |

**`src/graph/cohesion/`:** `graphCliques` (Bron-Kerbosch, via `writeCliqueCensus`) — B; cancel checked only at recursion depth 1, **deliberately** (comment explicitly says this avoids flooding the event loop on deep recursion — an accepted trade-off, not an oversight). Parallelization: branches at any level are logically independent, but `graphCliqueAdd()` mutates shared state (`m_cliques`, `CLQM`) — task-based parallelization is possible in principle but needs real thread-safety work first, not a drop-in `blockingMap`.

**`src/graph/layouts/`:**

| Method | P1 | P2 | P4 |
|---|---|---|---|
| `layoutForceDirectedSpringEmbedder`/`FruchtermanReingold` | A | Fine | Good candidate for the O(N²) force-accumulation loop, but the symmetric two-sided `disp()` write (`:189-190`) needs restructuring to one-sided per-pair computation to be race-free under `blockingMap` |
| `layoutForceDirectedKamadaKawai` | A | Fine | Outer particle-selection loop: poor (sequential, picks one max-Delta particle at a time). Per-particle `Delta_m` computation: good candidate (independent, reducible) |
| `layoutByProminenceIndex` | A | **None** in its own O(N) placement loop (only one coarse check before it) | Good candidate but low payoff (cheap relative to the centrality computation that precedes it) |
| `layoutRandom`/`layoutRadialRandom`/`layoutEgoRadial`/`layoutCircular` | B | **None** — `progressUpdate()` called throughout, `progressCanceled()` never checked in any of them | Good candidate structurally (per-vertex placement), but emitting Qt signals from worker threads needs queued delivery back to the GUI thread — real complication, not drop-in |

**`src/graph/generators/`:** all 6 `randomNet*Create` methods are Group B, mostly fine-grained P2 (a couple of sub-loops — `randomNetRegularCreate`'s edge-randomization `while`, `randomNetLatticeCreate`'s edge-computation loop — have none). P4 is consistently the same shape across all of them: the *decision* of which edges to create can often be computed independently (parallelizable), but the *application* (`edgeCreate()`/`edgeExists()` mutating shared graph state) must stay serial — "parallel decide, serial apply" restructuring, not a drop-in `blockingMap`. Preferential-attachment growth (`randomNetScaleFreeCreate`) is a genuine exception: **poor candidate even for the decision phase**, since each new node's attachment probability depends on the cumulative degree distribution built by every previously-added node.

**`src/graph/reporting/`** (thin wrapper layer — most of the real compute is in the tables above): two sub-findings worth flagging on their own — `writeMatrixAdjacency`/`writeMatrixAdjacencyPlot` (`graph_reports.cpp:6282,6425`) call `progressUpdate()` every row but **never call `progressCanceled()` at all** — the dialog visibly moves but Cancel is a silent no-op, a worse bug than "no dialog," since it actively misleads the user. `writeReciprocity`'s actual compute (`graphReciprocity()`, `graph_structure_metrics.cpp:203`) runs entirely **before** the progress dialog is even created.

**`src/graph/storage/`:** `verticesCreateSubgraph` (all 4 modes) — B, fine-grained P2, but P4 is a poor candidate: each iteration mutates shared adjacency state via `edgeCreate()`, same "parallel decide, serial apply" caveat as the generators.

**Newly discovered, zero protection on every front** (already folded into P3's Group C above):
`Graph::addRelationSymmetricStrongTies`, `Graph::relationAddCocitation`, `Graph::subgraphExtract`/`subgraphExtractFromSelection`. None call `progressCreate()`/`progressCanceled()` at all — a step below Group B, same shape as the filters.

**Prioritized summary:**

- **Best parallelization candidates** (clear win, APSP-shaped, per-vertex/per-row independent,
  read-mostly): `centralityDegree`, `graphTriadCensus`, `clusteringCoefficient`, the O(N²)
  matrix-fill loops that follow `graphDistancesGeodesic()` (`graphMatrixShortestPathsCreate`,
  `graphMatrixDistanceGeodesicCreate`, `createMatrixReachability`, `createMatrixAdjacency`),
  `centralityClosenessIR`/`prestigeDegree`/`prestigeProximity` (already fine-grained-cancel, smaller
  payoff since they're not the bottleneck). `createMatrixSimilarityMatching`/`Matrix::distancesMatrix()`/
  `pearsonCorrelationCoefficients()` are also good structural fits but need `Matrix` API changes
  first (a `cancelCheck` parameter, same pattern as `inverse()`/`powerIteration()`).
- **Worst cancellation gaps** (long-running + coarse-only or zero mid-loop checks):
  `prestigePageRank`'s convergence loop (zero checks inside a potentially many-iteration `while`,
  despite being in the "already handled" A-tangled group), `writeMatrixAdjacency`/
  `writeMatrixAdjacencyPlot` (dialog moves, Cancel is a no-op — actively misleading), `graphTriadCensus`'s
  O(N³) inner loops, `createMatrixSimilarityMatching` (whole build is one opaque uncancellable step),
  `randomNetRegularCreate`'s unbounded edge-randomization retry loop, `graphReciprocity()` (compute
  finishes before the dialog even exists).
- **Zero protection on every front**: the 3 P4-discovered additions to Group C above (folded into
  P3's Phase 1), plus the already-known 10 filter slots.

**Not yet decided:** whether/when to act on P4's findings — this audit is a map, not a commitment to
parallelize everything found. Cheap, clear-win candidates (property 4) are natural follow-ups once
P3's dispatch/cancel work lands, but each would still need its own golden/benchmark evidence (same
discipline as WS5 A2.0/A3) before being called a real improvement, not just a plausible one.

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
- P4's audit is research first, implementation second — findings get written up and reviewed before
  any parallelization work is proposed, same discipline as P3's Group A/B/C survey.
- No GitHub issue for any of this (unreleased 3.7-cycle behavior) — fix directly.
- Once P1-P4 land: add a changelog entry, update WS5's A5 section and WS7's status line to point
  here instead of duplicating content (both already do).
