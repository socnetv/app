# Benchmarks

`run_benchmarks.sh` runs **socnetv-cli micro-benchmarks** and compares benchmark timings against stored performance baselines where applicable.

Benchmarks validate:

- Algorithmic performance stability
- Absence of accidental slowdowns
- Structural refactor safety (e.g. CLI modularization)

Benchmarks **do not validate correctness** — correctness is enforced by golden JSON comparisons (`run_golden_compares.sh`).

---

# What Is Being Benchmarked

The benchmark harness supports three benchmark families.

By default (`--type all`), it runs only the **baseline-enforced** families:

- Distance / Centrality
- IO Roundtrip

The third family, **Optional Clustering Timing**, is developer-oriented and runs only when explicitly selected with `--type clustering`.

## Distance / Centrality (schema v1)

Benchmarks the `--kernel distance` (DistanceEngine + centrality computation).

Shipped cases (always run, CI-reproducible):

- `EIES48_T1_C1_W1` — Freeman EIES 48 actors, time 1, weighted, centralities ON
- `EIES48_T2_C1_W1` — Freeman EIES 48 actors, time 2, weighted, centralities ON
- `BA500_M3_C1_W0` — Barabási–Albert N=500, m=3, centralities ON
- `BA500_M3_C0_W0` — Barabási–Albert N=500, m=3, distances only

Large-net cases (run only if `~/socnetv/library/nets/large/` exists — local developer machines only, not CI):

- `DIST_GRAPHML_1000N_10000A_C0_W0` — 1000 actors, 10000 arcs, distances only (`--bench 2`)
- `DIST_GRAPHML_1000N_10000A_C1_W0` — 1000 actors, 10000 arcs, centralities ON (`--bench 2`)

> **Note:** Distance computation cost is primarily a function of E (edges), not N (vertices).
> The DistanceEngine runs BFS/Dijkstra from each source: O(N × (N + E)).
> For dense graphs, E dominates. This is why a 1000-node/10000-arc graph
> takes ~100× longer than a 500-node/1219-arc graph.

## IO Roundtrip (schema v5)

Benchmarks `--kernel io_roundtrip` load timing for large datasets.
Measures `LOAD_MS` (file load time only, not save/reload).

Shipped cases (always run, CI-reproducible):

- `IO_PAJ_BA500` — Barabási–Albert N=500, m=3, Pajek format

Large-net cases (run only if `~/socnetv/library/nets/large/` exists):

- `IO_GRAPHML_2000N_40000E` — 2000 actors, 40000 edges, GraphML
- `IO_GRAPHML_1000N_10000A` — 1000 actors, 10000 arcs, GraphML

Other kernels (reachability, walks_matrix, prominence) are validated for correctness only, not performance.

## Optional Clustering Timing (schema v6, informational)

Benchmarks `--kernel clustering` wall-clock timing for selected datasets.

Important:

- clustering timing does **not** use `--bench`
- timings are single-run, wall-clock measurements
- results are **informational only**
- they are **not enforced** against performance baselines
- they run **only** when explicitly selected with `--type clustering`

Run them with:

```bash
./scripts/run_benchmarks.sh --type clustering
```

Current shipped clustering timing probes:

* `CLUST_KITE_N10` — Krackhardt Kite N=10
* `CLUST_SAMPSON_N18` — Sampson Monks N=18

Rationale:

* The clustering kernel does not currently support deterministic multi-run benchmarking (`--bench`).
* Clique enumeration and triad census are still useful to time locally.
* This gives trend visibility without introducing noisy CI failures.

---

# Benchmark Type Selection

Use `--type` to select which benchmark family to run:

* `--type distance` → run only distance / centrality benchmarks
* `--type io` → run only IO timing benchmarks
* `--type clustering` → run only optional clustering timing probes
* `--type all` → run the baseline-enforced benchmark families only (`distance` + `io`) (default)

Examples:

```bash
./scripts/run_benchmarks.sh --type distance
./scripts/run_benchmarks.sh --type io
./scripts/run_benchmarks.sh --type clustering
./scripts/run_benchmarks.sh --type all
```

---

# Baseline Selection (Machine-Aware)

Priority order for choosing the expected baseline file:

1. `BENCH_BASELINE=/path/to/perf_expected.env`
   (explicit file path)

2. `BENCH_BASELINE_SET=<name>`
   → `scripts/perf_baselines/<name>/perf_expected.env`

3. Auto set: `<OS>-<ARCH>`
   → `scripts/perf_baselines/<auto>/perf_expected.env` (if present)

The script prints an INFO line showing:

```
BENCH_BASELINE_SET
EXPECTED_FILE
BUILD_TYPE
RECORD
```

Baseline selection is therefore explicit and reproducible.

---

# Recording a New Baseline Set

Record a baseline for the current machine (auto set name):

```bash
./scripts/run_benchmarks.sh --record
```

Record into an explicit set name:

```bash
BENCH_BASELINE_SET=linux-ryzen ./scripts/run_benchmarks.sh --record
```

This writes:

```
scripts/perf_baselines/<set>/perf_expected.env
```

Note:

* Recording applies to the baseline-enforced benchmark families.
* Optional clustering timing probes are not recorded into performance baselines.

---

# Strict Regression Detection

To treat performance regressions as hard failures:

```bash
./scripts/run_benchmarks.sh --strict
```

A case fails if:

```
actual > expected × 1.10
```

The tolerance is +10%.

For distance cases, `actual` is the **median compute time** over N runs (`--bench N`).
For IO cases, `actual` is the **single load time** (`LOAD_MS`). IO baselines with
`expected == 0ms` are skipped automatically (too fast to measure reliably).

Strict regression enforcement applies to distance and IO benchmarks only. Optional clustering timing probes are informational and are not baseline-enforced.

IO timing regression enforcement is also available at the kernel level via
`socnetv-cli --strict` (applies `checkIoTimings()` inside `compareGoldenV5Io()`).

---

# Large Net Cases

Cases that use `~/socnetv/library/nets/large/` are **local-only**:

* They are skipped silently in CI environments where the directory does not exist.
* They are always guarded by `if [[ -d "$LARGE_NETS_DIR" ]]`.
* They should not be expected to be fast — 1000-node distance benchmarks
  take 28–47 seconds on a Debug build.
* Record baselines for these on your own machine; do not commit baselines
  recorded on machines with different hardware or build configurations.

---

# Build Type / Binary Selection

The script does **not** build the project.
It executes an existing `socnetv-cli` binary.

Override binary path explicitly:

```bash
SOCNETV_CLI=/path/to/socnetv-cli ./scripts/run_benchmarks.sh
```

`BENCH_BUILD_TYPE=Debug|Release` is only a hint used for default binary path discovery
via `scripts/lib/find_socnetv_cli.sh`.

Example:

```bash
BENCH_BUILD_TYPE=Release ./scripts/run_benchmarks.sh
```

---

# Debug vs Release

Benchmarks are sensitive to:

* Build type (Debug vs Release)
* Compiler flags
* LTO / inlining
* Logging configuration

Important:

* Debug builds must suppress `qDebug` / `qInfo` output during benchmarking.
* Logging overhead can distort compute times significantly.

Baseline sets should be recorded consistently under the same build type and environment.

---

# Baseline Philosophy

Baselines are treated as **stable performance contracts**.

They should only be re-recorded when:

* A deliberate algorithmic improvement is made
* A known performance bug is fixed
* The build environment materially changes

Baselines are not intended to be routinely overwritten.

---

# Rollback

To revert to a previous baseline:

* Restore the relevant `scripts/perf_baselines/<set>/perf_expected.env` from git history.
* Or delete the set directory and re-record with `--record`.

---

# Future Work

Clustering benchmarks may evolve to support deterministic multi-run timing
(bench mode) once `kernel_v6` exposes stable iteration semantics.