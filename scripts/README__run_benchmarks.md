# Benchmarks

`run_benchmarks.sh` runs socnetv-cli micro-benchmarks and compares medians against stored baselines.

## Baseline selection (machine-aware)

Priority order for choosing the expected baseline file:

1. `BENCH_BASELINE=/path/to/perf_expected.env` (explicit file path)
2. `BENCH_BASELINE_SET=<name>` → `scripts/perf_baselines/<name>/perf_expected.env`
3. Auto set: `<OS>-<ARCH>-<HOST>` → `scripts/perf_baselines/<auto>/perf_expected.env` (if present)
4. Legacy fallback: `PERF_EXPECTED_FILE` (defaults to `scripts/perf_expected.env`)

The script prints an INFO line to stderr showing which baseline file is in use.

## Recording a new baseline set

Record a baseline for the current machine (auto set name):

```bash
./scripts/run_benchmarks.sh --record
````

Record into an explicit set name:

```bash
BENCH_BASELINE_SET=linux-ryzen ./scripts/run_benchmarks.sh --record
```

This writes:

```
scripts/perf_baselines/<set>/perf_expected.env
```

## Running with strict regression detection

```bash
./scripts/run_benchmarks.sh --strict
```

## Build type / binary selection

The script does not build; it executes `socnetv-cli`.

You can select a different binary via:

```bash
SOCNETV_CLI=/path/to/socnetv-cli ./scripts/run_benchmarks.sh
```

`BENCH_BUILD_TYPE=Debug|Release` is a hint used only for default binary path discovery.

## Rollback

To revert to legacy single-baseline behavior:

* revert `scripts/run_benchmarks.sh`
* remove `scripts/perf_baselines/`

The legacy baseline file `scripts/perf_expected.env` is preserved.

