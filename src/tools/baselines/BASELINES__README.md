# Baselines

This folder contains committed golden outputs for `socnetv-cli`.

Baselines are used to detect regressions while refactoring core algorithms.

If a baseline changes, it must be justified in a commit message.

Baselines are schema-versioned and kernel-specific.

---

# 1. Schema Versions

Each algorithm family owns a dedicated schema version.

| Kernel         | Schema Version | Folder                              |
|---------------|----------------|--------------------------------------|
| distance      | v1             | `src/tools/baselines/`               |
| reachability  | v2             | `src/tools/baselines/reachability/`  |
| walks_matrix  | v3             | `src/tools/baselines/walks/`         |

Schemas are never modified retroactively.

New algorithm families must use a new schema version.

---

# 2. Naming Convention

## Distance Kernel (schema v1)

```

<DATASET>__FT<filetype>__C<0|1>_W<0|1>_IW<0|1>_DI<0|1>.json

```

Where:

- `FT`  → file type (`-f` argument)
- `C`   → computeCentralities
- `W`   → considerWeights
- `IW`  → inverseWeights
- `DI`  → dropIsolates

Example:

```

SmallWorld_N10_E12__FT1__C1_W0_IW1_DI0.json

```

---

## Reachability Kernel (schema v2)

```

<DATASET>__REACH__V2.json

```

Example:

```

StokmanZiegler_Netherlands__REACH__V2.json

```

---

## Walks Kernel (schema v3)

```

<DATASET>__WALKS_K<length>__V3.json

```

Example:

```

TinyPath_N3_E2__WALKS_K2__V3.json
DunbarGelada_H22a__WALKS_K6__V3.json

```

---

# 3. Golden Checklist (Pre-Release)

Before tagging a release:

1. Build `socnetv-cli`
2. Run all golden comparisons
3. Ensure **no mismatches**

Recommended:

```

scripts/run_golden_compares.sh

```

If any case reports a mismatch:

- Do NOT regenerate baselines blindly.
- Investigate.
- Only update baseline if the change is intentional and documented.

---

# 4. How to Add a New Baseline

## Step 1 — Generate JSON

### Distance (v1)

```bash
./build/socnetv-cli \
  -i <dataset> \
  -f <filetype> \
  -c <0|1> -w <0|1> -x <0|1> -k <0|1> \
  --dump-json src/tools/baselines/<NAME>.json
```

### Reachability (v2)

```bash
./build/socnetv-cli \
  --kernel reachability \
  -i <dataset> -f <filetype> \
  -c 0 -w <0|1> -x <0|1> -k <0|1> \
  --dump-json src/tools/baselines/reachability/<NAME>.json
```

### Walks (v3)

```bash
./build/socnetv-cli \
  --kernel walks_matrix \
  --walks-length <K> \
  -i <dataset> -f <filetype> \
  -c 0 -w <0|1> -x <0|1> -k <0|1> \
  --dump-json src/tools/baselines/walks/<NAME>.json
```

---

## Step 2 — Review

* Check graph-level metrics
* Check matrix dimensions
* Confirm deterministic vertex ordering
* Confirm numeric plausibility

For walks:

* Small test networks (e.g., TinyPath_N3_E2) are strongly recommended
* Validate known combinatorial results manually

---

## Step 3 — Commit

* Add JSON file
* Mention dataset + flags in commit message
* If replacing an existing baseline, explain why

Baselines must correspond to datasets in:

```
src/data/
```

---

# 5. What Is Verified

## Distance Kernel (v1)

### Graph-Level Metrics

* nodes (N)
* links_sna
* ties_graph
* directed / weighted
* average geodesic distance
* diameter
* disconnected_pairs
* connected

### Per-Node Metrics (when C=1)

For each vertex (ascending deterministic id):

* CC / SCC
* BC / SBC
* SC / SSC
* EC / SEC
* PC / SPC
* distance_sum
* eccentricity

Floating-point values are serialized as strings to avoid JSON
double-format inconsistencies.

---

## Reachability Kernel (v2)

* nodes
* matrix (0/1)
* reachable_pairs
* reachable_density

Diagonal convention:

```
R(i,i) = 1
```

---

## Walks Kernel (v3)

* nodes
* matrix (integer counts)
* walks.length
* walks.total_walks

Walk semantics:

```
XM = A^K
XM(i,j) = number of walks of exact length K from i to j
```

Note:

A long-standing bug in `walksBetween()` was fixed.
Previous baselines (pre-v3) may have reflected adjacency values instead of true A^K results.

---

# 6. Exit Codes & CI Integration

`socnetv-cli --compare-json` exits with:

* `0` → match
* non-zero → mismatch or runtime error

This makes it suitable for:

* CI pipelines
* pre-merge validation
* pre-release checklist

The runner script fails fast on the first mismatch.

---

# 7. Existing Baselines

## Distance (v1)

Located directly under:

```
src/tools/baselines/
```

Includes:

* Pajek datasets
* UCINET datasets

---

## Reachability (v2)

Located under:

```
src/tools/baselines/reachability/
```

---

## Walks (v3)

Located under:

```
src/tools/baselines/walks/
```

Includes:

* Small validation networks (e.g., TinyPath_N3_E2)
* Medium datasets
* Large datasets

---

# 8. Notes

* `LINKS_SNA` reflects loader semantics.
* `TIES_GRAPH` reflects canonical Graph model.
* Baselines must be generated from identical datasets.
* Deterministic ordering is mandatory.
* Never update baselines silently.
* Schema structures must remain immutable once committed.

