# SocNetV Tools

This directory contains headless utilities used for refactoring safety nets.

## socnetv-cli

`socnetv-cli` loads a dataset headlessly (no GUI), runs distance metrics,
and can dump/compare deterministic JSON golden outputs.

### Build

The CLI is built as part of the normal build when `BUILD_CLI` is enabled.

### Basic run

```bash
./socnetv-cli -i src/data/SmallWorld_N10_E12.graphml -f 1
````

`-f` is the `FileType` enum integer (GRAPHML=1, PAJEK=2, UCINET=5, TWOMODE=9, ...).

### Output fields

* `LINKS_SNA`:
  SNA links/ties as reported at load time (A->B counts as 1 link).
  For undirected networks, some formats/storage rules may result in 2 links per edge.

* `TIES_GRAPH`:
  Canonical tie count derived from the model (`Graph::edgesEnabled()`).
  This equals edges for undirected graphs and arcs for directed graphs.

### Golden JSON

Dump a baseline:

```bash
./socnetv-cli -i src/data/SmallWorld_N10_E12.graphml -f 1 \
  --dump-json src/tools/baselines/SmallWorld_N10_E12__C1_W0_IW1_DI0.json
```

Compare:

```bash
./socnetv-cli -i src/data/SmallWorld_N10_E12.graphml -f 1 \
  --compare-json src/tools/baselines/SmallWorld_N10_E12__C1_W0_IW1_DI0.json
```

Exit codes:

* 0: match / success
* 1: mismatch (compare) or load failed
* 2: invalid arguments or JSON read/write error
