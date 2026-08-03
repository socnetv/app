# Baselines

This folder contains committed golden outputs for `socnetv-cli`.

Baselines are used to detect regressions while refactoring core algorithms.

If a baseline changes, it must be justified in a commit message.

Baselines are schema-versioned and kernel-specific.

---

# 1. Schema Versions

Each algorithm family owns a dedicated schema version.

| Kernel        | Schema | Folder |
|---------------|--------|--------|
| distance      | v1     | `src/tools/baselines/` |
| reachability  | v2     | `src/tools/baselines/reachability/` |
| walks_matrix  | v3     | `src/tools/baselines/walks/` |
| prominence    | v4     | `src/tools/baselines/prominence/` |
| io_roundtrip  | v5     | `src/tools/baselines/io_roundtrip/` |
| clustering    | v6     | `src/tools/baselines/clustering/` |
| connectivity  | v7     | `src/tools/baselines/connectivity/` |
| matrix        | v8     | `src/tools/baselines/matrix/` |

distance (v1) has no subfolder — it was the only kernel when the tool was first built, so nothing
needed disambiguating yet. Every kernel added afterward got its own subfolder; distance's existing
baselines were left where they were rather than moved for no functional reason.

Schemas are never modified retroactively.

New algorithm families must use a new schema version.

---

# 2. Naming Convention

## Distance Kernel (schema v1)

```
<DATASET>__FT<filetype>__C<0|1>_W<0|1>_IW<0|1>_DI<0|1>.json
```

* `FT`  → file type (`-f` argument)
* `C`   → computeCentralities
* `W`   → considerWeights
* `IW`  → inverseWeights
* `DI`  → dropIsolates

Example: `SmallWorld_N10_E12__FT1__C1_W0_IW1_DI0.json`

---

## Reachability Kernel (schema v2)

```
<DATASET>__REACH__V2.json
```

Example: `StokmanZiegler_Netherlands__REACH__V2.json`

---

## Walks Kernel (schema v3)

```
<DATASET>__WALKS_K<length>__V3.json
```

Example: `TinyPath_N3_E2__WALKS_K2__V3.json`

---

## Prominence Kernel (schema v4)

```
<DATASET>__PROM__V4__FT<filetype>__W<0|1>_IW<0|1>_DI<0|1>.json
```

Note: `C` is not encoded — prominence always computes the full index set.

Example: `Krackhardt_Kite_N10__PROM__V4__FT2__W0_IW1_DI0.json`

---

## IO Roundtrip Kernel (schema v5)

```
<DATASET>__FT<filetype>.json
```

Example: `TinyGraphML_Weighted_Dir_N3__FT1.json`

---

## Clustering Kernel (schema v6)

```
<DATASET>__CLUST__V6__FT<filetype>__W<0|1>_IW<0|1>_DI<0|1>.json
```

Example: `Krackhardt_Kite_N10__CLUST__V6__FT2__W0_IW1_DI0.json`

---

## Connectivity Kernel (schema v7)

```
<DATASET>__CONN__V7__FT<filetype>.json
```

No weight/centrality flags — connectivity is topology-only.

Example: `TinyDisconnected_Undir_N6_E4__CONN__V7__FT2.json`

---

## Matrix Kernel (schema v8)

```
<DATASET>__MATRIX__V8__FT<filetype>__W<0|1>_IW<0|1>_DI<0|1>.json
```

Note: `C` is not encoded — matrix always computes every category.

Example: `TinyPath_N3_E2__MATRIX__V8__FT2__W0_IW1_DI0.json`

---

# 3. Golden Checklist (Pre-Release)

Before tagging a release:

1. Build `socnetv-cli`
2. Run all golden comparisons: `scripts/run_golden_compares.sh`
3. Ensure **no mismatches**

If any case reports a mismatch:

* Do NOT regenerate baselines blindly.
* Investigate.
* Only update baseline if the change is intentional and documented.

---

# 4. How to Add a New Baseline

## Distance (v1)

```bash
./build/socnetv-cli \
  -i <dataset> -f <filetype> \
  -c <0|1> -w <0|1> -x <0|1> -k <0|1> \
  --dump-json src/tools/baselines/<NAME>.json
```

## Reachability (v2)

```bash
./build/socnetv-cli --kernel reachability \
  -i <dataset> -f <filetype> \
  -c 0 -w <0|1> -x <0|1> -k <0|1> \
  --dump-json src/tools/baselines/reachability/<NAME>.json
```

## Walks (v3)

```bash
./build/socnetv-cli --kernel walks_matrix \
  --walks-length <K> \
  -i <dataset> -f <filetype> \
  --dump-json src/tools/baselines/walks/<NAME>.json
```

## Prominence (v4)

```bash
./build/socnetv-cli --kernel prominence \
  -i <dataset> -f <filetype> \
  -w <0|1> -x <0|1> -k <0|1> \
  --dump-json src/tools/baselines/prominence/<NAME>.json
```

## IO Roundtrip (v5)

```bash
./build/socnetv-cli --kernel io_roundtrip \
  -i <dataset> -f <filetype> \
  --dump-json src/tools/baselines/io_roundtrip/<NAME>.json
```

## Clustering (v6)

```bash
./build/socnetv-cli --kernel clustering \
  -i <dataset> -f <filetype> \
  -w <0|1> -x <0|1> -k <0|1> \
  --dump-json src/tools/baselines/clustering/<NAME>.json
```

## Connectivity (v7)

```bash
./build/socnetv-cli --kernel connectivity \
  -i <dataset> -f <filetype> \
  --dump-json src/tools/baselines/connectivity/<NAME>.json
```

## Matrix (v8)

```bash
./build/socnetv-cli --kernel matrix \
  -i <dataset> -f <filetype> \
  -c 0 -w <0|1> -x <0|1> -k <0|1> \
  --dump-json src/tools/baselines/matrix/<NAME>.json
```

---

# 5. What Is Verified

## Distance Kernel (v1)

Graph-level: nodes, links_sna, ties_graph, directed, weighted, average geodesic distance, diameter, disconnected_pairs, connected

Per-node (when C=1): CC/SCC, BC/SBC, SC/SSC, EC/SEC, PC/SPC, distance_sum, eccentricity

Floating-point values are serialized as strings.

---

## Reachability Kernel (v2)

nodes, matrix (0/1), reachable_pairs, reachable_density

Diagonal convention: R(i,i) = 1

---

## Walks Kernel (v3)

nodes, matrix (integer counts), walks.length, walks.total_walks

Walk semantics: XM(i,j) = number of walks of exact length K from i to j

---

## Prominence Kernel (v4)

Graph-level: nodes, links_sna, ties_graph, directed, weighted

Per-node centrality: DC/SDC, CC/SCC, IRCC/SIRCC, BC/SBC, SC/SSC, PC/SPC, IC/SIC, EVC/SEVC, eccentricity

Per-node prestige: DP/SDP, PP/SPP, PRP/SPRP

All floating-point values are serialized as strings.

---

## IO Roundtrip Kernel (v5)

Graph-level: nodes, relations, directed, symmetric, weighted, ties_graph, links_sna

Roundtrip: performed vs skipped export, ROUNDTRIP_EQUIV, per-relation signature bundle for multi-relational datasets

---

## Clustering Kernel (v6)

Graph-level: nodes, links_sna, ties_graph, directed, weighted

Metrics: averageCLC, nodesWithCLC

Per-node: CLC, hasCLC

Triad census (16 MAN classes): 003, 012, 102, 021D, 021U, 021C, 111D, 111U, 030T, 030C, 201, 120D, 120U, 120C, 210, 300

Cliques: counts by size, max_clique_size, total_cliques

---

## Connectivity Kernel (v7)

Graph-level: component_count, connected (bool), type ("weak" for directed / "connected" for undirected)

Per-node: component_id (1-based integer)

Connectivity semantics:

* Undirected: standard BFS — connected if 1 component
* Directed: BFS treating all arcs as undirected (weak connectivity) — connected if 1 weak component

---

## Matrix Kernel (v8)

Raw contents of every `Matrix`-producing `Graph` operation: adjacency, adjacency inverse,
distances, similarity, reachability, walks, total walks (skipped above `kTotalWalksSkipThreshold`
nodes — see `kernel_matrix_v8.cpp`), clique co-membership.

Networks above `kFullGridSizeLimit` (20 nodes) are dumped as a compact summary (row/col sums,
trace, five sampled cells) instead of the full N×N grid — a full-grid dump is impractical past a
few dozen nodes (WS5 A2.0 measured ~461 MB for one `Matrix` at N=7,343).

Exists specifically because every other kernel only ever checks downstream results (centrality
scores, distance values, clique counts), never a `Matrix`'s actual contents — see
`roadmap_ws6_testing_ci_regression.md`'s WS6.7 section for the full design rationale.

---

# 6. Exit Codes & CI Integration

`socnetv-cli --compare-json` exits with:

* `0` → match
* non-zero → mismatch or runtime error

`scripts/run_golden_compares.sh` aggregates failures and exits non-zero if any mismatch occurs.

---

# 7. Existing Baselines

| Kernel        | Directory |
|---------------|-----------|
| distance v1   | `src/tools/baselines/` |
| reachability v2 | `src/tools/baselines/reachability/` |
| walks v3      | `src/tools/baselines/walks/` |
| prominence v4 | `src/tools/baselines/prominence/` |
| io_roundtrip v5 | `src/tools/baselines/io_roundtrip/` |
| clustering v6 | `src/tools/baselines/clustering/` |
| connectivity v7 | `src/tools/baselines/connectivity/` |
| matrix v8     | `src/tools/baselines/matrix/` |

---

# 8. Notes

* `LINKS_SNA` reflects loader semantics.
* `TIES_GRAPH` reflects canonical Graph model.
* Baselines must be generated from identical datasets in `src/data/`.
* Deterministic ordering is mandatory.
* Never update baselines silently.
* Schema structures must remain immutable once committed.
