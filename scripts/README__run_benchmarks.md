# Benchmarks

`run_benchmarks.sh` runs **socnetv-cli micro-benchmarks** and compares median compute times against stored baselines.

Benchmarks validate:

- Algorithmic performance stability
- Absence of accidental slowdowns
- Structural refactor safety (e.g. CLI modularization)

Benchmarks **do not validate correctness** — correctness is enforced by golden JSON comparisons (`run_golden_compares.sh`).

---

# What Is Being Benchmarked

The benchmark harness executes:

```

--kernel distance

```

Only the **distance/centrality kernel (schema v1)** is benchmarked.

Other kernels (reachability, walks_matrix, etc.) are validated for correctness only, not performance.

---

# Baseline Selection (Machine-Aware)

Priority order for choosing the expected baseline file:

1. `BENCH_BASELINE=/path/to/perf_expected.env`  
   (explicit file path)

2. `BENCH_BASELINE_SET=<name>`  
   → `scripts/perf_baselines/<name>/perf_expected.env`

3. Auto set:  
   `<OS>-<ARCH>-<HOST>`  
   → `scripts/perf_baselines/<auto>/perf_expected.env` (if present)

4. Legacy fallback:  
   `PERF_EXPECTED_FILE` (defaults to `scripts/perf_expected.env`)

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

---

# Strict Regression Detection

To treat performance regressions as hard failures:

```bash
./scripts/run_benchmarks.sh --strict
```

A case fails if:

```
median_actual > median_expected × 1.10
```

The tolerance is +10%.

---

# Build Type / Binary Selection

The script does **not** build the project.
It executes an existing `socnetv-cli` binary.

Override binary path explicitly:

```bash
SOCNETV_CLI=/path/to/socnetv-cli ./scripts/run_benchmarks.sh
```

`BENCH_BUILD_TYPE=Debug|Release` is only a hint used for default binary path discovery.

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

To revert to legacy single-baseline behavior:

* Revert `scripts/run_benchmarks.sh`
* Remove `scripts/perf_baselines/`

The legacy baseline file:

```
scripts/perf_expected.env
```

remains supported.

