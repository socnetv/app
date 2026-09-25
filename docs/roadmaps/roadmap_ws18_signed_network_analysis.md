# Signed Network Analysis / Structural Balance (WS18)

## Goal

First-class support for signed networks — graphs where edge sign (not just magnitude) is
meaningful: friend/enemy, trust/distrust, alliance/conflict. Covers the full chain from "don't
silently corrupt results on negative weights" through dedicated signed-graph algorithms:
Bellman-Ford-based distances, purpose-built signed centrality measures, and Heider/Cartwright-
Harary structural balance analysis on triads.

## Status

Tracked by #284. **P0, P1, P2 all complete.** P1: 2026-09-19 (#277). P0: 2026-09-25 (#285) — DL/
Adjacency parsers silently dropped negative-weight edges, now fixed with golden coverage in all
7 supported formats. P2: engine work 2026-09-22 plus GUI menu wiring across the four affected
Graph-distances actions 2026-09-25 — see P2 below for the full account, including a real caching
bug found and fixed along the way. P3-P4 not started — scoped only.

**Unrelated fix found and landed along the way (#283):** while designing P2's Bellman-Ford engine
path, cross-checking `dijkstraSSSP()`'s behavior against an independent library surfaced a real BC
bug in the existing (unsigned) Dijkstra path — unrelated to signed networks, pre-dating this
workstream. Fixed separately; see `f6076bc7`. Noted here only because it was found during WS18
work, not because it's in scope for this workstream.

## Background

SocNetV already treats negative edge weights as legitimate, intentional input — Settings has a
dedicated negative-edge display color and negative edges render dashed on the canvas. But almost
nothing downstream actually understands *sign* as distinct from *magnitude*:

- **`DistanceEngine::dijkstraSSSP()`** (`src/engine/distance_engine.cpp:1196`) is textbook
  Dijkstra — mathematically undefined for negative weights (the "popped = finalized" invariant
  breaks). No guard anywhere. Every distance-derived measure (CC, BC, SC, EC, IRCC, PC) silently
  returns finite-but-wrong numbers on a negative-weight graph. This is #277.
- **One existing foothold:** `centralityBonacich()`
  (`src/graph/centrality/graph_centrality_bonacich.cpp`) already allows negative `β`
  (Bonacich & Lloyd 2004's extension of Bonacich Power Centrality to negative ties) — per-node
  scores can come out negative, unlike every other measure in the app. This is the only place in
  the codebase that treats sign as semantically meaningful rather than just "a weight that happens
  to be negative."
- **A partial, unsigned foothold:** `graphTriadCensus()`
  (`src/graph/clustering/graph_triad_census.cpp:42`) already computes the full 16-type
  Davis-Holland-Leinhardt MAN triad classification in parallel, with per-type doc comments. This is
  the *shape* half of what structural balance needs (triad enumeration + classification
  infrastructure already exists and is well-tested) — it does not currently look at edge sign at
  all, since MAN classification is about (mutual/asymmetric/null) dyad structure, not sign.
  Balance classification is an orthogonal layer on top, not a rewrite of this function.
- **No existing infrastructure for:** Bellman-Ford or any negative-weight-safe shortest-path
  algorithm; negative-cycle detection; any signed-specific centrality measure (PN centrality is not
  implemented); any structural-balance/triad-sign analysis.

## Relationship to #277

#277's guard (reject/warn on negative weights before Dijkstra runs) is **P1** of this workstream,
not a separate piece of work — it was scoped narrowly on its own issue because it was the
immediate correctness fix needed regardless of whether signed-network support was ever built out
further. Everything past P1 is new, larger, and was explicitly deferred pending prioritization —
see `roadmap_ws11_algorithm_additions.md`'s #277 entry.

## Phases

Ordered by dependency. P0 and P1 are independent of each other (P0 is an input-layer fix, P1 is
a computation-layer guard) and both are prerequisites for everything after them — P1 is already
complete; P0 is filed but not started. P2 depends on P1 only in the sense that P1's guard
becomes obsolete for graphs where P2's engine is selected — the two aren't sequenced by data
dependency otherwise. P3 depends on P2 for any measure defined via signed shortest paths (most of
them are), but PN centrality specifically is not distance-based and could land before P2. P4
(structural balance) is independent of P2/P3 — it operates on triad sign patterns directly, not on
distances.

### P0 — Parser support for negative edge weights (#285) ✔ complete

DL and Adjacency (one-mode) format parsers gated edge creation on `edgeWeight > 0`
(`parser_dl.cpp`'s fullmatrix reader, both the diagonal and normal-edge branches;
`parser_adjacency.cpp`'s one-mode branch — the two-mode/bipartite branch was unaffected, it
already gates on `cell != "0"`), silently dropping any negative-weight cell on import. Found
while building the golden-coverage kernel for this workstream (`kernel_signed_v10`, WS6.1):
a hand-built negative-cycle `.dl` fixture loaded with an edge missing, no error. Confirmed
every other parser's edge-creation call site was already unaffected: Pajek, GraphML, GML,
EdgeList, and DOT all create an edge unconditionally once a weight is parsed, no positivity
gate.

**Fixed 2026-09-25**: both gates changed to `edgeWeight != 0`, matching the other five formats.
`Signed_Dir_N4_NoCycle` now has golden coverage in all 7 supported formats (added
`.adj`/`.dl`/`.dot`/`.gml`/`.wlst`, alongside the pre-existing `.paj`/`.graphml`), each
independently cross-checked to produce identical Bellman-Ford potentials before being committed
as a baseline — `.dl`/`.adj` specifically exercise the two fixed parser branches. Also closed an
adjacent gap: `run_golden_io_roundtrip.sh` had zero coverage of a negative weight surviving a
save/reload cycle (a different failure mode than the load-time drop above — e.g. a format writer
mishandling the sign on export) — added the same network in all 7 formats there too, each
verified to carry the negative weight through a real round-trip, not just the initial load.

### P1 — Guard existing distance-based measures against negative weights (#277) ✔ complete

Shipped 2026-09-19 across three commits (c8666fd6, 5123064a, 38f1d6c1). Detects a negative edge
weight once per computation inside `DistanceEngine::initRun()` (merged into the existing
max-weight-scan loop — one O(N²) pass, not a second one — and checked before
`graph.setConnectedCached(true)` runs, so refusal never leaves connectivity state looking
legitimately computed), reported back via `Graph::negativeWeightsDetected()`, a queryable flag
mirroring the existing `m_progressCanceled`/`progressCanceled()` pattern exactly (same
cross-thread-atomic treatment, same reset-at-start-of-computation shape).

All 15 real `considerWeights`-forwarding call sites of `Graph::graphDistancesGeodesic()` are
guarded — 11 mirror an existing `progressCanceled()`-refusal precedent at their own call site
(`writeCentralityCloseness()`/`writeCentralityBetweenness()`/`writeCentralityStress()`/
`writeCentralityEccentricity()`/`writeCentralityPower()`/`writeEccentricity()` in
`graph_reports.cpp`; `centralityClosenessIR()`; `prestigeProximity()`;
`layoutByProminenceIndex()`; `vertexFindByIndexScore()`;
`graphMatrixShortestPathsCreate()`/`graphMatrixDistanceGeodesicCreate()` in
`graph_matrix_distances.cpp`). The remaining 4 (`graph_distance_facade.cpp`) had no existing
precedent and needed their own design: `graphDistanceGeodesic()`/`graphGeodesicDistanceDistribution()`
now return `RAND_MAX`/an empty `QMap` on refusal (matching `apspDistance()`'s own "nothing computed"
sentinel — without this, a *prior* successful computation's stale `m_apspDist` entry would have been
returned instead of a clear refusal signal); `graphDiameter()`/`graphDistanceGeodesicAverage()` have
no safe sentinel (0 is a legitimate result), so the fix lives in their sole MainWindow callers
(`slotAnalyzeDiameter()`/`slotAnalyzeDistanceAverage()` in `mainwindow_analyze_distance.cpp`), which
now capture `negativeWeightsDetected()` the same way they already captured `*isWeighted`/`*isConnected`
and show a refusal status message instead of a misleading numeric result.

Verified live via `--interactive-script` against a negative-weight fixture and an otherwise-identical
positive-weight control (two new script commands, `diameter`/`average-distance`, added since no
existing command reached those two MainWindow slots). `./scripts/run_golden_compares.sh` clean
throughout. Matrix-power measures (EVC, Katz, Bonacich, PRP) are not Dijkstra-based and were
correctly left untouched — Bonacich already handles negative values by design (see Background).

### P2 — Negative-weight-safe shortest paths (Johnson's algorithm) ✔ complete

**Algorithm choice settled: Johnson's, not naive per-source Bellman-Ford.** A single global
Bellman-Ford pass from a virtual source computes a potential `h(v)` per vertex; every edge is
reweighted as `w'(u,v) = w(u,v) + h(u) - h(v)` (guaranteed non-negative, preserves the shortest-path
set and ties exactly); the existing `dijkstraSSSP()` then runs **unmodified** on the reweighted
graph, once per source, inside the existing parallel loop; results are un-reweighted afterward
(`d(s,v) = d'(s,v) - h(s) + h(v)`). Negative-cycle detection falls out of the same Bellman-Ford pass
for free (the standard "does round V still improve anything" check). This was chosen over naive
Bellman-Ford for its asymptotics (`O(V·(E+V log V))` total vs. `O(V²·E)`) and because it leaves
`dijkstraSSSP()`'s own logic untouched — no separate relaxation-order/Stack-ordering concern the way
naive Bellman-Ford would have needed (Dijkstra's own pop order already satisfies Brandes' back-
propagation requirement once every weight it sees is non-negative).

**Design decisions confirmed:**
- Negative-cycle handling: **refuse the whole computation**, not partial results — a shortest path
  is undefined in a graph with a reachable negative cycle, and there's no principled way to give
  BC/CC/etc. a defined answer for just the affected pairs.
- P1's existing guard: **stays as the default**, refuse-by-default behavior. Johnson's is opt-in via
  an explicit flag on the relevant `Graph` entry points, not a silent auto-switch whenever a
  negative weight is detected — callers ask for negative-weight-safe distances, they don't get
  silently upgraded to a different algorithm.
- Reweighted edge costs are **not cached anywhere** (no parallel edge-weight structure): potentials
  are threaded into `dijkstraSSSP()` as a read-only `const QVector<qreal> &`, and `w'` is computed
  inline at the existing weight-read site. Rejected a precomputed reweighted-edge cache — it would
  save one subtraction per edge examination at the cost of a second, must-stay-in-sync source of
  truth for edge weight, not worth it.
- Tie-detection floating-point risk (composing two independently-rounded `h(u)`, `h(v)` into every
  edge before the `dist_w == cur_dist_w` comparison) is **already closed**: `dijkstraSSSP()`'s tie
  check was switched from exact `qreal` equality to `distancesNearlyEqual()` (a relative-tolerance
  comparison) ahead of this phase, specifically to remove this as an open risk before Johnson's
  reweighting starts composing extra floating-point terms into every edge weight.

**Status: ✔ complete** — engine work (2026-09-22, `e63a1f30`), GUI menu wiring (2026-09-25).
- ✔ `DistanceEngine::bellmanFordPotentials()` — the reweighting/negative-cycle-detection pass,
  golden-tested standalone via `--kernel signed` (schema v10).
- ✔ Potentials threaded into `dijkstraSSSP()`/`runAllSources()`: `dijkstraSSSP()` takes an optional
  `const QVector<qreal> &potentials` and reweights each edge inline (`w' = w + h(u) - h(v)`);
  `runAllSources()` un-reweights `tls.pss.dist[]` right after each source's run, before the
  `m_apspDist` write-back and CC/PC accumulation (both need the true, not reweighted, value).
- ✔ Opt-in entry point: `Graph::graphDistancesGeodesicSigned()` (separate method, not a parameter
  on `graphDistancesGeodesic()` — none of that function's ~20 existing callers are touched) plus
  `DistanceEngine::compute()`'s new `negativeWeightSafe` parameter (default `false`). Refuses
  distinctly via `Graph::negativeCycleDetected()` on a genuine negative cycle.
  `--kernel signed`/`--interactive-script`'s `bellman-ford` command both call it; the six signed
  golden baselines cover BC/CC/distances from the real reweighted path, not just standalone
  potentials.
- Found and fixed a real pre-existing bug while exercising this end-to-end: `dijkstraSSSP()`'s
  strict-improvement branch required `dist_w > 0`, silently dropping any relaxation landing on
  exactly 0 — unreachable under plain Dijkstra (zero-weight edges are skipped earlier in the same
  loop) but reachable once Johnson's reweighting can legitimately produce an exact 0. Fixed to
  `dist_w >= 0`.
- ✔ GUI menu wiring (2026-09-25): the four Analyze → Cohesion... → Graph distances actions that
  can hit a negative-weight refusal (Distance, Average Distance, Geodesic Distances Matrix,
  Diameter - the last was the original pilot) now all offer the same "use the negative-weight-safe
  algorithm instead?" upgrade on refusal, calling the new `graphDistanceGeodesicSigned()` /
  `graphDistanceGeodesicAverageSigned()` / `writeMatrix(..., allowNegativeWeights=true)` wrappers.
  Manually verified end-to-end via the GUI on `src/data/Signed_Dir_N4_NoCycle.paj`.
- Found and fixed a real caching bug while doing this GUI wiring: `DistanceEngine::compute()`'s
  `calculatedDistances`/`calculatedCentralities` early-return cache didn't record *which* mode
  (plain vs. negative-weight-safe) produced the cached result - switching modes on the same graph
  (e.g. upgrading to the signed path for one action, then requesting an ordinary computation for
  another) silently reused the wrong mode's stale result instead of recomputing, permanently
  masking the refusal dialog for every subsequent ordinary-mode call. Fixed via a new
  `m_lastComputeWasNegativeWeightSafe` flag that invalidates the cache on a mode mismatch.
  Independently reproduced via the GUI before the fix, and confirmed fixed after, by chaining a
  signed Distance call into an ordinary Average Distance call on the same loaded graph.
- Renamed the engine-level `negativeWeightSafe` parameter to `allowNegativeWeights` throughout
  (`DistanceEngine::compute()`/`initRun()`, `Graph::writeMatrix()`,
  `Graph::graphMatrixDistanceGeodesicCreate()`) - clearer about what the flag actually does (opts
  in to negative weights being allowed, not a claim that the whole call is somehow "safe").
- **Open question, not decided here**: on a signed graph, an individual geodesic distance can
  itself be negative (Johnson's algorithm's whole point is that shortest paths are still
  well-defined then) - confirmed on `Signed_Dir_N4_NoCycle.paj`, where B→C's shortest distance is
  -2. Average Distance's `d = 2.16667` is the mathematically correct mean of the six reachable
  pairs' (possibly-negative) distances, but whether "average distance" is even the right framing
  for a signed graph, or needs different wording/caveats, is undecided - applies equally to
  Distance/Diameter/Average Distance/the Distances Matrix, not just this GUI wiring pass. Revisit
  during P3/P4 design.

### P3 — Signed-specific centrality measures

Scope: **signed degree centrality** (#300) and **PN centrality** (#301, Everett & Borgatti 2014). A
third measure covered by prior art in this space, signed eigenvector centrality, is explicitly
deferred — it has real algorithmic complexity (dominant-eigenvalue existence/uniqueness checks)
beyond what's needed here, and no roadmap slot yet. Each gets its own issue/PR per this doc's Work
Rules; signed degree first (small, no open design questions), then PN centrality (bigger —
convergence bound and directed-graph semantics both need resolving along the way).

**Formula (PN centrality)** — confirmed directly from an established outside package's own
source implementation (not a secondary summary this time). Three real differences from what was
assumed earlier, all now settled:

1. **Strictly binary, no weighted mode at all.** `P = (A > 0)` and `N = (A < 0)` (as 0/1 masks) -
   tie *magnitude* is discarded unconditionally, even if the input adjacency matrix is weighted.
   The reference implementation additionally requires every edge's sign to be exactly `-1` or `1`,
   so the whole model is ±1-only. No `considerWeights` toggle for PN - it doesn't apply.
2. **Three distinct formulas depending on directedness/mode**, not one formula reused via a
   transpose:
   ```
   mode = "all" (undirected):  PN = rowSums( (I - βA)⁻¹ )
   mode = "out" (directed):    PN = rowSums( (I - β²·A·Aᵀ)⁻¹ · (I + β·A) )
   mode = "in"  (directed):    PN = rowSums( (I - β²·Aᵀ·A)⁻¹ · (I + β·Aᵀ) )
   ```
   where `A = P - 2N` (built from the binary P/N above), `β = 1/(2n-2)` for `all`/the linear term
   in `in`/`out`, and `β² = 1/(4(n-1)²)` for the quadratic term inside `in`/`out`'s inverse.
   the `all` mode is rejected on a directed graph and vice versa (an undirected graph is forced to
   `all`) - not a free combination.
3. Final step is a row-sum of the fully-solved matrix - same operation as `PN = (...)⁻¹ · 1`
   (row-sum against the all-ones vector), just expressed differently; matches the row-sum pattern
   `graph_centrality_katz.cpp` already uses for `invM`.

Structurally the `all`-mode formula is still the same closed-form "geometric series of walks"
identity Katz centrality uses (`C_Katz = ((I - alpha*A^T)^-1 - I) * 1`) - same `(I - x·M)⁻¹ · 1`
shape, fixed `β` instead of a user-tunable `α`, no `- I` term. The `in`/`out` directed forms are a
different, more involved closed form specific to this measure, not a simple Katz analogue.

**Scope for this pass**: binary only (matches the confirmed formula exactly), all three modes
(`all`/`out`/`in`) implemented now rather than undirected-only-then-defer, since the formulas for
all three are now confirmed rather than guesswork.

#### Checklist

- [ ] **`Matrix` gains a public signed-A-matrix build method** — new method on `Matrix` (not
      ad-hoc code in the centrality slice) to build `A = P - 2N` directly from a signed adjacency
      matrix in one pass (binary P/N per the confirmed formula, not materialized as separate
      matrices - the per-cell rule folds directly into one pass: `+1` for a positive tie, `-2` for
      a negative tie, `0` for none). Needed by PN centrality (which genuinely does matrix-level
      work). Signed degree does **not** use this: `centralityDegree()`'s own existing pattern is
      direct `edgeExists()` iteration parallelized via `QtConcurrent::blockingMap` (WS15 P4), never
      `Matrix`/`AM` - signed degree follows that same precedent instead, so building a `Matrix`
      P/N-derived matrix just to sum rows would be a detour from how that measure's family is
      actually implemented elsewhere in the codebase.
- [x] **Signed degree centrality engine** — new `src/graph/centrality/graph_centrality_signed_degree.cpp`,
      `Graph::centralitySignedDegree(...)`. Four variants (pos / neg / ratio / net) stored
      simultaneously per vertex (one edge scan fills all four - cheap, and a signed-network report
      naturally wants all four side by side, not one re-run per variant). **Out-degree only for
      this first pass** - `centralityDegree()`/DC itself is out-degree-only, with in-degree as an
      entirely separate measure (`prestigeDegree()`/DP); signed in-degree is deliberately deferred
      as its own later follow-on rather than silently doubling this step's scope to 8 stored
      values. Direct edge iteration filtered by sign, same parallelization shape as
      `centralityDegree()` above (`QtConcurrent::blockingMap`, per-vertex independent writes).
- [x] **Signed degree CLI wiring** — `kernel_prominence_v4.cpp` computes it unconditionally
      alongside DC (no gating flag needed, unlike Katz/Bonacich - no user-supplied parameter with
      no meaningful default) and dumps `signedDegreePos`/`Neg`/`Ratio`/`Net` per node. New
      `Signed_Dir_N4_NoCycle` prominence baseline gives real negative-split coverage (every other
      prominence fixture is all-positive, so `pos==DC`/`neg==0` there - degenerate, not a
      meaningful test of the split itself).
- [x] **Signed degree GUI wiring** — new `Graph::writeCentralitySignedDegree()` (minimal
      report: just the pos/neg/ratio/net score table, no distribution chart/sum/mean/variance,
      matching the engine's own no-aggregate-stats scope), `MainWindow::slotAnalyzeCentralitySignedDegree()`,
      a menu action under Analyze > Centrality and Prestige indices with a full Meaning/When to
      use/Weights/Compare to What's This, and a new `IndexType::SIGNED_DEGREE` entry so it's also
      selectable in the Prominence toolbox combo. Explicitly **not** wired into
      `layoutByProminenceIndex()`/`vertexFindByIndexScore()`/`vertexFilterByCentrality()` (all
      three assume one scalar + a standardized/max value per index, which this measure doesn't
      have) - excluded from the "Visualize by prominence index" and Node Find dropdowns entirely
      (same `removeAll()` pattern already used for Clustering Coefficient), and
      `isCentralityIndexComputed()`'s existing `default: return false` keeps the Filter-by-
      Centrality dialog's copy permanently (and correctly) disabled. All verified live via manual
      GUI testing on `Signed_Dir_N4_NoCycle`, not just build success.
- [x] **Signed degree WS12 interactive-script command** — `report-centrality-degree-signed
      [weights] [dropisolates] [csv]`, same two-step-dispatch pattern as `report-centrality-degree`.
      Named after an established outside package's own `degree_signed()` function name (per WS12's
      naming-parity direction), reordered to keep this codebase's own
      `report-centrality-*` prefix. Verified headlessly against `Signed_Dir_N4_NoCycle` (CSV
      output): values match the manually-verified GUI report exactly.

**Signed degree centrality (#300) is now fully wired** — engine, CLI kernel, GUI (menu + toolbox
combo), reporting, and WS12 script command all done and verified. Next: PN centrality (#301).
- [ ] **PN centrality** — new `src/graph/centrality/graph_centrality_pn.cpp`,
      `Graph::centralityPN(...)`. Build `A = P - 2N` (binary) via the new `Matrix` method, fixed
      `β = 1/(2n-2)`, then all three modes per the confirmed formulas above:
      undirected (`solve(I-βA)`), directed out (`solve(I-β²AAᵀ)·(I+βA)`), directed in
      (`solve(I-β²AᵀA)·(I+βAᵀ)`) - row-sum of the solved matrix in every case. Mode selection:
      undirected graphs are forced to the `all` formula; directed graphs choose `in`/`out` (`all`
      isn't valid on a directed graph, matching the reference implementation's own hard error on
      that combination).
- [ ] **Convergence/singularity guard for PN** — now a matter of implementation, not open design:
      the reference formula is a plain matrix inversion with no separate convergence check before
      it - `Matrix::inverse()`'s own existing singularity detection (already used by Katz) is
      sufficient; report "not defined: singular" on failure, same as Katz's own fallback.
- [x] **Directed-graph semantics for PN** — resolved by the confirmed formula above: not a guess
      or an extension, `in`/`out` are real, distinct, independently-confirmed formulas.
- [ ] **Wiring**, same 8-touchpoint shape Katz used, for both measures: `Graph` façade method,
      `GraphVertex` storage, `MainWindow` dialog/menu action
      (`src/mainwindow/analyze/mainwindow_analyze_centrality.cpp`), CLI kernel
      (`kernel_prominence_v4.cpp`), reporting (`graph_reports.cpp`), `graph_centrality.cpp`
      dispatch, layout-by-prominence (`graph_layouts_basic.cpp` — PN's values can be negative,
      same open framing question already flagged for Distance/Diameter under P2 applies here too).
- [ ] **GUI "What's This"/tooltip text for PN**, not just the doc-comment Meaning section — plain-
      language framing to carry over: *"it is worse to receive a negative tie from someone who is
      highly popular (receives a lot of positive ties) than from someone who is marginalized;
      receiving a negative tie from someone who is universally disliked might even be interpreted
      as a positive indicator in structural dynamics."* This is the intuition for why PN weights a
      negative tie by the sender's own positive standing, not just a flat penalty - worth landing
      in the dialog's What's This help and/or its tooltip, not just this roadmap doc or the source
      comment.
- [ ] **New `--interactive-script` command(s)** (WS12) for both measures, following WS12's Command
      naming direction (name/shape after the equivalent operation in an established SNA scripting
      ecosystem where one clearly exists) — every new algorithm added from here on needs this, not
      just a GUI menu action and a CLI kernel flag.

**Found, deliberately deferred, not part of this checklist**: all 16 existing centrality/prestige
`QAction`s' `setWhatsThis()`/`setStatusTip()` text predates the richer Meaning/When to
use/Weights/Compare to/Math doc-comment convention now used in the source (confirmed concretely:
Katz's source doc comment is much richer than its current What's This, which is short and has none
of that shape). Signed degree's own new What's This should be written well from the start, but
upgrading the other 15 pre-existing measures' GUI text to match is a separate, sizeable audit -
worth its own tracked item once P3 lands, not a mid-task detour here.
- [ ] **Doc comments** following the fixed Meaning/When to use/Weights/Compare to/Math shape (PN's
      Compare-to section names Katz centrality explicitly, given the corroborated resemblance
      above).
- [ ] **Independent verification** for both measures against ground truth computed outside SocNetV
      (see `docs/roadmaps/roadmap_ws6_testing_ci_regression.md`'s WS6.8/WS6.9 discipline — hand
      derivation plus a second independent method, not self-consistency), on a small fixture with
      hand/independently-computable `P`, `N`, `A`, and the closed-form result.
- [ ] New golden baselines (`prominence` or `signed` kernel family, whichever fits once the wiring
      is in place); `./scripts/run_golden_compares.sh` clean before each commit.
- [x] **Prerequisite groundwork, done ahead of the centrality wiring itself**: PN's convergence
      guard needs `Matrix::spectralRadiusExact()` (Perron-Frobenius, extracted from
      `Graph::estimateSpectralRadius()`), `Matrix::spectralRadiusBound()` (Gerschgorin, works on
      any matrix including signed ones), and `Graph::hasNegativeWeight()`/
      `Matrix::hasNegativeEntry()` to dispatch between them - all four added and golden-verified,
      including a new `matrix`-kernel `spectral_radius` scalar block
      (`has_negative_entry`/`bound`/`exact`) with dedicated coverage of the signed case via the
      `Signed_Dir_N4_NoCycle` fixture (the only `matrix`-kernel baseline with a negative weight,
      so `has_negative_entry: true` and the omitted `exact` field are both actually
      regression-tested, not just the non-negative path every other fixture exercises).

### P4 — Structural balance analysis (Heider / Cartwright-Harary)

- Classify each triad as **balanced** or **unbalanced** by the product-of-signs rule (a triad is
  balanced iff the product of its three edge signs is positive — equivalently, 0 or 2 negative
  edges is balanced, 1 or 3 is not). Builds on `graphTriadCensus()`'s existing enumeration/MAN
  classification infrastructure (`src/graph/clustering/graph_triad_census.cpp`) by adding a sign
  dimension alongside the existing MAN dimension — needs design attention on how the two
  classifications compose (a triad is classified by both dyad-structure type *and* balance
  status), not a replacement of the existing function.
- Network-level **balance ratio** (fraction of balanced triads) as a single summary statistic,
  following the same aggregate-from-per-triad-classification pattern the existing triad census
  already uses for its type-frequency table.
- Directed-graph handling needs explicit design: Cartwright-Harary balance theory is classically
  defined for undirected signed graphs; SocNetV's triad census already handles directed MAN types,
  so extending balance to directed graphs (rather than restricting this phase to undirected graphs
  only) needs its own scoping pass, not an assumption either way here.
- **Clusterizable / two-faction test** (structural balance's strong theorem: a fully-balanced
  signed graph is exactly two mutually-hostile internally-friendly factions) as a possible stretch
  goal once the base classification lands — lower priority, not part of the initial deliverable.

## Explicitly out of scope (for now)

- Any UI feature (canvas coloring by balance status, dedicated signed-network layout) — this
  workstream is compute-first, matching how WS11's other measures shipped (algorithm slice first,
  UI wiring as a small follow-on once the measure exists and has golden coverage).
- Weighted/graded balance measures beyond the classic ±1 sign model (e.g. degree-of-imbalance
  metrics beyond the simple balance ratio) — not ruled out permanently, just not in the initial
  scope.

## Work Rules

Same discipline as every other workstream:

- `./scripts/run_golden_compares.sh` clean before any commit; new golden baselines for each new
  computation, including at least one hand-verified signed fixture with an analytically-known
  answer per phase (mirroring how #281's Esfahanian-Hakimi work and the Katz/Bonacich additions
  were each verified against independently-computed ground truth, not just self-consistency).
- New algorithm slices stay QtCore-only per the `src/graph/` boundary rule; UI/reporting hooks (if
  and when P4's stretch UI work is prioritized) live in `src/graph/ui/` and `src/graph/reporting/`.
- Each phase gets its own issue once actually scoped — don't implement a whole phase as one PR;
  P2-P4 in particular each bundle several design decisions that need to be settled (and likely
  checked in with the user, per this repo's mid-task consult rule) before code, not during.
- Centrality/clustering doc comments follow the fixed **Meaning / When to use / Compare to / Math**
  shape from `docs/README_DEVELOPER_NOTES.md`.
