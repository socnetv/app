# Algorithm Additions (WS11)

## Status

**Just created — not started.** Collects a set of longstanding "implement algorithm X" feature
requests that don't fit any existing workstream: WS5 (Matrices Modernization) is explicitly scoped
to matrix *storage/performance*, not new algorithms; WS9 (Graph Exploration) is a completed
workstream. This is a different kind of work from either — pure numerical/graph-theory
implementation on top of already-stable infrastructure, not architecture.

## Scope

New analysis algorithms requested against the existing `src/graph/` algorithm-slice architecture
(see `CLAUDE.md`'s "Strict boundary inside `src/graph/`" — QtCore only, no UI construction in the
slice itself; rendering/reporting lives in `src/graph/ui/` and `src/graph/reporting/`). Grouped
below by which existing slice directory each would live in.

## Centrality — `src/graph/centrality/`

- **#10 — Katz Centrality.** Walk-based centrality with an attenuation factor `α` penalizing longer
  paths (`α^d` per path of length `d`). Needs a convergence/validity check on `α` relative to the
  adjacency matrix's largest eigenvalue.
- **#39 — Bonacich Power Centrality** `BPC(α, β)`. Not to be confused with the already-implemented
  Power Centrality (PC, a generalized degree measure by Gil and Schmidt) — different measure,
  same-sounding name; worth a clearly distinct label in the UI to avoid confusion.
- **#134 — Alternate Centrality Measures Meta-List.** Not a single algorithm — a running collection
  of centrality-measure ideas (cross-references #10, #39, #108, and external references like
  `centiserve`/`netrankr`/`CINNA`). Treat as a backlog-within-a-backlog: triage individual measures
  out of it into their own issues as they're actually prioritized, rather than implementing "the
  meta-list" as one deliverable.

## Cohesion — `src/graph/cohesion/`

- **#3 — Cohesive subgroups identification.** n-cliques, n-clans/n-clubs, k-plexes, matrix
  permutation approaches. Four distinct methods bundled in one issue — likely worth splitting into
  separate issues once one is actually scoped, since they're independent algorithms with different
  complexity profiles (clique-family enumeration is combinatorially expensive at scale).
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

## Clustering — `src/graph/clustering/`

- **#5 — More clustering/community-detection algorithms**: Girvan-Newman, Clauset-Newman-Moore,
  Wakita-Tsurumi. SocNetV currently only has HCA (Hierarchical Clustering Analysis). **Cross-reference:**
  #258 ("[Future] Color nodes by detected community — Louvain/modularity", tracked as WS9 debt) is
  the same underlying capability (community detection) from the layout/coloring-integration angle
  rather than the analysis-output angle — worth scoping together rather than landing two independent
  community-detection implementations.

## Similarity / Structural Equivalence — `src/graph/similarity/`

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
