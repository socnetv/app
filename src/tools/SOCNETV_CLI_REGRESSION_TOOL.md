# SocNetV CLI Regression Tool

This is a headless regression tool used to verify
algorithmic correctness during the ongoing architectural refactor of
SocNetV.

The tool provides deterministic execution of the distance / centrality
kernel without loading any UI components.

It is specifically designed to protect the refactor of:

    Graph::graphDistancesGeodesic()
    → DistanceEngine

---

## Purpose

The CLI enables:

- Headless dataset loading (no MainWindow / GraphicsWidget)
- Deterministic execution of DistanceEngine
- Golden-output JSON generation
- Strict regression comparison against committed baselines
- CI integration (fail-fast on mismatch)

This ensures that refactors preserve:

- numeric results
- connectivity bookkeeping
- centrality vectors
- directed/weighted semantics

---

## Build

The tool is built as:

    socnetv-cli

It is compiled alongside the main application via CMake.

---

## Basic Usage

Load a dataset and print summary metrics:

```bash
./socnetv-cli \
  -i src/data/SmallWorld_N10_E12.graphml \
  -f 1
```

Common file types (see `global.h`):

* 1 → GRAPHML
* 2 → PAJEK
* 3 → ADJACENCY
* 5 → UCINET
* etc.

---

## Golden Output Dump

Generate a deterministic JSON baseline:

```bash
./socnetv-cli \
  -i src/data/SmallWorld_N10_E12.graphml \
  -f 1 \
  --dump-json src/tools/baselines/SmallWorld_N10_E12__C1_W0_IW1_DI0.json
```

The filename convention encodes run flags:

```
C1 = computeCentralities=1
W0 = considerWeights=0
IW1 = inverseWeights=1
DI0 = dropIsolates=0
```

---

## Golden Output Compare

Strict comparison against a baseline:

```bash
./socnetv-cli \
  -i src/data/SmallWorld_N10_E12.graphml \
  -f 1 \
  --compare-json src/tools/baselines/SmallWorld_N10_E12__C1_W0_IW1_DI0.json
```

On mismatch, the tool prints exact field differences and exits non-zero.

This makes it suitable for:

* CI pipelines
* pre-merge validation
* refactor checkpoints

---

## What Is Verified

### Graph-level metrics

* nodes
* links_sna (loader semantics)
* ties_graph (canonical Graph model)
* directed / weighted
* average geodesic distance
* diameter
* disconnected_pairs
* connected

### Per-node vectors (when centralities enabled)

For each vertex (deterministic ascending id order):

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

## Design Principles

* No UI involvement
* No graphics dependency
* Deterministic ordering
* Deterministic float formatting
* Zero semantic modification of Graph

The CLI is a safety harness, not a new analytics engine.

---

## Baselines

Baselines live in:

```
src/tools/baselines/
```

These are committed to the repository and represent
“known good” outputs from the refactor target.

If a baseline changes, it must be justified in a commit message.

---

## Non-Goals

* This tool does not replace the UI.
* This tool does not introduce new analytics.
* This tool does not optimize performance.

It exists purely to protect correctness during refactor.

