# Algorithm Additions (WS11)

## Goal

Implement algorithm-family feature requests that don't fit any existing workstream: WS5 (Matrices
Modernization) is explicitly scoped to matrix *storage/performance*, not new algorithms; WS9
(Graph Exploration) is a completed workstream. WS11 is pure numerical/graph-theory implementation
on top of already-stable infrastructure, not architecture.

## Status

**Started.** Three items shipped so far — see What WS11 Delivered. Everything else in this doc
(Bonacich Power, cohesive subgroups, more clustering algorithms, structural equivalence) is
still untouched.

## Background

New analysis algorithms requested against the existing `src/graph/` algorithm-slice architecture —
algorithm slices stay QtCore only, no UI construction in the slice itself; rendering/reporting
lives in `src/graph/ui/` and `src/graph/reporting/`. Grouped below by which existing slice
directory each would live in.

## What WS11 Delivered

- **#7 — Network connectivity metric.** Local vertex connectivity κ(s,t) between an actor pair —
  the minimum number of other nodes whose removal disconnects them — via Menger's theorem: κ(s,t)
  equals the max number of internally vertex-disjoint s→t paths, computed as a max-flow problem on
  a vertex-split network (split each vertex into in/out nodes joined by a capacity-1 edge, so a
  unit of flow through a vertex "costs" one removal). Also covers global vertex connectivity
  κ(G) = min over all non-adjacent pairs of κ(s,t), with a cheap upper bound from the minimum
  vertex degree (Whitney's inequality) to prune most pairs before running max-flow on them. For
  directed networks, asks the user weak vs. strong (see #272) and applies that choice to both the
  local and global computation. Distinct from the existing edge-based cohesion measures.
- **#272 — Connectedness only checks weak connectivity for directed networks.** Not a new
  algorithm — a bug-fix/polish pass on the existing "Connectedness" feature, tracked here because
  it produces `Graph::graphStronglyConnectedComponents()` (Tarjan's SCC), which #7's "strong" mode
  depends on. Also fixes a dead `connectivityMenu` menu member, a result message pointing at a
  menu path that doesn't exist, and a missing help-text entry for "Connectedness" in the Cohesion
  toolbox dropdown.

Three follow-on visualizations surfaced by this work are noted below under What Remains Open
(Cohesion), not yet filed or scoped.

- **#10 — Katz Centrality.** New **Analyze → Centrality → Katz Centrality (KC)**, with full parity
  to the app's other 12 prominence indices: HTML/CSV report, all 4 Layout → By Prominence Index
  variants, Filter Nodes by Centrality, Find Node by index score, prominence distribution chart,
  and a `--katz-alpha` CLI flag under the existing `prominence` kernel with golden baselines in
  `src/tools/baselines/prominence/` (undirected/directed happy paths, an α-boundary rejection case,
  and Krackhardt Kite for non-trivial size). New `src/graph/centrality/graph_centrality_katz.cpp`
  (`Graph::centralityKatz(alpha, considerWeights, inverseWeights, dropIsolates)`), `IndexType::KATZ`,
  `GraphVertex::KC()/SKC()`, and `src/forms/dialogcentralitykatz.{h,cpp,ui}` (asks for `α`; no live
  `1/λ_max` bound shown, since that would require a synchronous `Graph` call from the GUI thread —
  relies on `centralityKatz()`'s post-hoc rejection instead). Since the shared layout-by-prominence
  dispatch has no parameter slot, the 4 layout variants reuse the `α` cached from the last Analyze
  run (`Graph::m_lastKatzAlpha`), with a `QMessageBox` telling the user to run Analyze first if Katz
  hasn't been computed yet this session. Landed `Matrix::powerIteration()`'s `λ_max` output param
  (reused by `centralityEigenvector()` too) as a shared prerequisite. Found and fixed two
  independent pre-existing arg-shift bugs (`centralityInformation`/`centralityEigenvector` missing
  `inverseWeights`) opportunistically while wiring this in, in the CLI kernel and in
  `layoutByProminenceIndex()` respectively — both noted in `CHANGELOG.md`.

## What Remains Open

### Centrality — `src/graph/centrality/`

- **#39 — Bonacich Power Centrality.** Same `(I - xM)⁻¹` shape as Katz (`BPC(α, β) =
  α(I - βR)⁻¹ · R · 1`, R = adjacency, possibly weighted), minus the `- I` term and with a second
  parameter `β` (the one multiplying the matrix) that — unlike Katz's `α` — is allowed to be
  negative, flipping whether being connected to well-connected others helps or hurts; `β` must
  still satisfy `|β| < 1/λ_max(A)`. Scoped as its own follow-up series after Katz landed and was
  verified working, reusing Katz's now-proven pattern rather than developing both blind in
  parallel: `graph_centrality_bonacich.cpp`, `writeCentralityBonacich`,
  `dialogcentralitybonacich.{h,cpp,ui}` (two spin boxes, `β`'s not clamped to positive),
  `slotAnalyzeCentralityBonacich()`, `IndexType::BPC = 15`, `GraphVertex::BPC()/SBPC()`,
  `m_lastBonacichAlpha`/`m_lastBonacichBeta` cache, same 4-layout-variant + filter + chart + CLI
  wiring Katz just got. **Naming collision to guard against**: `getProminenceIndexByName()` matches
  menu-action text by substring, and `"Power Centr"` (used for the existing Gil-Schmidt `PC`) would
  also match inside "**Bonacich Power** Centrality" — the `BPC` branch must check for `"Bonacich"`
  first. Label it "Bonacich Power Centrality (BPC)" distinctly from "Power Centrality (PC)" in every
  UI string and report header — same-sounding names, unrelated measures, and PC is older/already
  known to users.
- **#134 — Alternate Centrality Measures Meta-List.** Not a single algorithm — a running collection
  of centrality-measure ideas (cross-references #10, #39, #108, and external references like
  `centiserve`/`netrankr`/`CINNA`). Treat as a backlog-within-a-backlog: triage individual measures
  out of it into their own issues as they're actually prioritized, rather than implementing "the
  meta-list" as one deliverable.

### Cohesion — `src/graph/cohesion/`

- **#3 — Cohesive subgroups identification.** n-cliques, n-clans/n-clubs, k-plexes, matrix
  permutation approaches. Four distinct methods bundled in one issue — likely worth splitting into
  separate issues once one is actually scoped, since they're independent algorithms with different
  complexity profiles (clique-family enumeration is combinatorially expensive at scale).
- **(not yet filed) Color nodes by strongly connected component.** Extends the existing weak-only
  "Layout → Node Color by Connected Component" action to offer strong too. Tarjan's algorithm
  (`graphStronglyConnectedComponents()`) already computes per-vertex SCC membership as a side
  effect of finding the components — it's just discarded today (only the count is kept; see the
  function's own doc comment in `graph_distance_facade.cpp`). Needs an
  `m_vertexStrongComponentId` cache mirroring the existing weak one, plus UI wiring.
- **(not yet filed) Articulation points (cut vertices).** Nodes whose individual removal
  disconnects the graph — the classic, most intuitive answer to "which single nodes are critical
  for connectivity." Standard DFS/low-link algorithm, a close cousin of Tarjan's SCC (same
  underlying technique as bridge-finding) but a distinct computation — doesn't need vertex
  connectivity or max-flow at all, runs once over the whole graph. Not built yet.
- **(not yet filed) Minimum vertex cut for the weakest pair.** `graphConnectivity()` already runs
  max-flow internally to compute κ(G) — the actual separating node set is recoverable via standard
  max-flow/min-cut extraction (a BFS over the residual graph once the flow saturates), not a
  guess. Needs `graphConnectivity()` to remember which pair achieved the minimum (currently only
  the value is tracked), then re-run local flow for that pair with cut extraction. **Caveat, worth
  surfacing to the user in the UI**: minimum vertex cuts aren't always unique — a symmetric graph
  (e.g. the Petersen graph) can have several different node sets of the same minimum size
  separating the same pair, and the algorithm will deterministically return one of them, not
  necessarily "the" canonical one a human would pick.

### Clustering — `src/graph/clustering/`

- **#5 — More clustering/community-detection algorithms**: Girvan-Newman, Clauset-Newman-Moore,
  Wakita-Tsurumi. SocNetV currently only has HCA (Hierarchical Clustering Analysis). **Cross-reference:**
  #258 ("[Future] Color nodes by detected community — Louvain/modularity", tracked as WS9 debt) is
  the same underlying capability (community detection) from the layout/coloring-integration angle
  rather than the analysis-output angle — worth scoping together rather than landing two independent
  community-detection implementations.

### Similarity / Structural Equivalence — `src/graph/similarity/`

- **#181 — Structural equivalence analysis**: Multidimensional Scaling (MDS), blockmodelling,
  CONCOR (convergent correlations). Standard SNA structural-equivalence toolkit; SocNetV currently
  has pairwise similarity measures (matching, Pearson) but not these three.

## Work Rules

- Same discipline as every other workstream: `./scripts/run_golden_compares.sh` clean before any
  commit, new golden baselines added for each new computation.
- Each algorithm gets its own issue once actually scoped (several of the issues above bundle
  multiple distinct methods) — don't implement "the issue," implement one algorithm at a time.
- New algorithm slices stay QtCore-only per the `src/graph/` boundary rule; UI/reporting hooks live
  in `src/graph/ui/` and `src/graph/reporting/`, matching the existing pattern for every other
  analysis in the app.
- Every centrality/prestige doc comment pairs its technical `@brief` with a fixed shape —
  **Meaning**, **When to use**, **Compare to** (nearby/confusable measures), then **Math** — per
  `docs/README_DEVELOPER_NOTES.md`'s Doc-Comment Convention section. Already retrofitted onto every
  existing measure (DC, CC, IRCC, BC, SC, EC, PC, IC, EVC, DP, PRP, PP, CLC); Katz/Bonacich's own
  doc comments follow the same shape from the start.
