# Baselines

This folder contains committed golden outputs for `socnetv-cli`.

Baselines are used to detect regressions while refactoring core algorithms.

If a baseline changes, it must be justified in a commit message.

---

# 1. Naming Convention

Recommended format:

```
<DATASET>__FT<filetype>__C<0|1>_W<0|1>_IW<0|1>_DI<0|1>.json
```

Where:

* `FT`  → file type (`-f` argument)
* `C`   → computeCentralities
* `W`   → considerWeights
* `IW`  → inverseWeights
* `DI`  → dropIsolates

Example:

```
SmallWorld_N10_E12__FT1__C1_W0_IW1_DI0.json
```

---

# 2. Golden Checklist (Pre-Release)

Before tagging a release:

1. Build `socnetv-cli`
2. Run all golden comparisons
3. Ensure **no mismatches**

Recommended command:

```
src/tools/run_golden_compares.sh
```

Manual example:

```bash
./build/socnetv-cli \
  -i src/data/SmallWorld_N10_E12.graphml \
  -f 1 \
  -c 1 -w 0 -x 1 -k 0 \
  --compare-json src/tools/baselines/SmallWorld_N10_E12__FT1__C1_W0_IW1_DI0.json
```

If any case reports a mismatch:

* Do NOT regenerate baselines blindly.
* Investigate.
* Only update baseline if the change is intentional and documented.

---

# 3. How to Add a New Baseline

Step 1 — Generate JSON:

```bash
./build/socnetv-cli \
  -i <dataset> \
  -f <filetype> \
  -c <0|1> -w <0|1> -x <0|1> -k <0|1> \
  --dump-json src/tools/baselines/<NAME>.json
```

Step 2 — Review:

* Check graph-level metrics
* Check per-node values
* Confirm deterministic ordering

Step 3 — Commit:

* Add JSON file
* Mention dataset + flags in commit message

Baselines must always correspond to datasets in:

```
src/data/
```

---

# 4. What Is Verified

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

# 5. Exit Codes & CI Integration

`socnetv-cli --compare-json` exits with:

* `0` → match
* non-zero → mismatch or runtime error

This makes it suitable for:

* CI pipelines
* pre-merge validation
* pre-release checklist

The runner script fails fast on the first mismatch.

---

# 6. Existing Baselines

## Pajek Baselines

### Topology (ignore weights in kernel)

```bash
./build/socnetv-cli \
  -i src/data/Stephenson_Zelen_Dunbar_Dunbar_Gelada_baboon_colony_H22a_IC.paj \
  -f 2 \
  -c 1 -w 0 -x 1 -k 0 \
  --compare-json src/tools/baselines/DunbarGelada_H22a__FT2__C1_W0_IW1_DI0.json
```

### Weighted + Inverse Weights

```bash
./build/socnetv-cli \
  -i src/data/Stephenson_Zelen_Dunbar_Dunbar_Gelada_baboon_colony_H22a_IC.paj \
  -f 2 \
  -c 1 -w 1 -x 1 -k 0 \
  --compare-json src/tools/baselines/DunbarGelada_H22a__FT2__C1_W1_IW1_DI0.json
```

---

## UCINET Baselines

### Binary (Non-Weighted)

```bash
./build/socnetv-cli \
  -i src/data/Stokman_Ziegler_Corporate_Interlocks_Netherlands.dl \
  -f 5 \
  -c 1 -w 0 -x 1 -k 0 \
  --compare-json src/tools/baselines/StokmanZiegler_Netherlands__FT5__C1_W0_IW1_DI0.json
```

### Weighted

```bash
./build/socnetv-cli \
  -i src/data/Stokman_Ziegler_Corporate_Interlocks_Netherlands.dl \
  -f 5 \
  -c 1 -w 1 -x 1 -k 0 \
  --compare-json src/tools/baselines/StokmanZiegler_Netherlands__FT5__C1_W1_IW1_DI0.json
```

---

# 7. Notes

* `LINKS` are SNA-style directed ties.
* `EDGES` and `ARCS` are derived for convenience.
* Baselines must be generated using identical datasets.
* Never update baselines silently.
