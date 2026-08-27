# scripts/

Index of everything in this directory. Detailed docs are linked where they exist.

---

## Regression & Benchmarking (actively used)

| Script | Purpose |
|---|---|
| `run_golden_compares.sh` | Correctness regression: runs all 8 CLI kernels against committed JSON baselines (`src/tools/baselines/`, see [`BASELINES__README.md`](../src/tools/baselines/BASELINES__README.md)). |
| `run_benchmarks.sh` | Performance regression: distance/prominence/io baseline-enforced timing, plus optional clustering timing probes. See [`README__run_benchmarks.md`](README__run_benchmarks.md) for the full reference. |
| `run_golden_io_roundtrip.sh` | Runs the `io_roundtrip` kernel for every baseline under `src/tools/baselines/io_roundtrip/`. `--update` regenerates them in place instead of comparing. |
| `run_io_roundtrip_shipped_datasets.sh` | Runs `io_roundtrip` across every file in `src/data/` (filetype inferred from extension) as a load smoke-test — no `--compare-json`, no committed baseline. |
| `run_matrix_storage_bench.sh` | WS5 A2.0/A3 storage micro-benchmark driver (`QHash` vs `Matrix`, or `Matrix`-vs-`Matrix` across code versions) for the `matrix-storage-bench` tool (`BUILD_MATRIX_BENCH` CMake option). |
| `run_render_perf_bench.sh` | WS10/WS6.6 canvas rendering-performance regression kernel (#240) — drives the real GUI binary headless via `--interactive-script` (`fixtures/render_perf_script.txt`), `QT_QPA_PLATFORM=offscreen`. |
| `lib/find_socnetv_cli.sh`, `lib/find_socnetv_gui.sh` | Shared binary-discovery helpers sourced by the scripts above. |
| `perf_baselines/<platform>/perf_expected.env` | Committed performance baselines, one directory per platform (`macos-arm64`, `macos-m5`, `linux-x86_64`). **Only re-recorded against a clean tagged release, never `develop` HEAD** — see [`README__run_benchmarks.md`](README__run_benchmarks.md#baseline-philosophy). |
| `fixtures/render_perf_script.txt` | `--interactive-script` fixture used by `run_render_perf_bench.sh`. |

## Packaging / Release

| Script | Purpose |
|---|---|
| `entitlements.plist` | macOS code-signing entitlements (app sandbox, etc.) for the DMG build. |
| `innosetup.iss` | Windows installer script (Inno Setup), used by `build-release.yml`. |
| `flathub/` | Flatpak manifest + desktop file + icon for the Flathub build. |
| `update_translations.sh` | Regenerates Qt `.ts`/`.qm` translation files. |

## Legacy (not used by current CI)

`travis_before.sh`, `travis_install_deps.sh`, `travis_make_build_linux.sh`,
`travis_make_build_macos.sh`, `travis_upload_packages.sh` — from the project's earlier Travis CI
setup. Not referenced by any current `.github/workflows/*.yml`; left in place, not maintained.

---

See also: [`docs/README_DEVELOPER_NOTES.md`](../docs/README_DEVELOPER_NOTES.md) for the regression
harness's place in the overall architecture, and
[`docs/SOCNETV_CLI_REGRESSION_TOOL.md`](../docs/SOCNETV_CLI_REGRESSION_TOOL.md) for the full
`socnetv-cli` command reference.
