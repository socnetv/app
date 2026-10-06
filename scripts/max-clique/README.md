# scripts/max-clique/

Benchmark tooling for SocNetV's clique code. Today it times the **maximal-clique census**
(`Graph::graphCliques()`, Bron–Kerbosch with Tomita pivoting) and checks that two builds find
exactly the same cliques. It is also the intended home for the benchmarks of the planned
maximum-clique routine — see "Maximum clique and size-bounded clique search" in
[`docs/roadmaps/roadmap_ws11_algorithm_additions.md`](../../docs/roadmaps/roadmap_ws11_algorithm_additions.md).

This is a manual A/B tool. It is **not** run by CI and has no committed performance baseline; the
correctness guard for the census is the golden `clustering` and `matrix` baselines
(`scripts/run_golden_compares.sh`).

## Contents

| File | Purpose |
|---|---|
| `bench_cliques.sh` | Times the census on 14 networks (median of several runs) and saves each run's census JSON. |
| `cmp.py` | Compares two labelled runs: time per network and whether the census is identical. Exit status 1 if any census differs. |
| `gen.py` | Regenerates the 10 seeded dense random test networks. |
| `nets/ER_N<n>_p<pct>.net` | The generated networks (Pajek `.net`): `G(n, p)` with `n` vertices and tie probability `p`, fixed seed, so regenerating reproduces them byte for byte. About 130 KB in total. |

## What is measured

`bench_cliques.sh` runs the `clustering` kernel of `socnetv-cli` and reads its `CLIQUES_MS`
line, which is the time spent in `graphCliques()` alone (the kernel's `COMPUTE_MS` also includes
the clustering coefficient, triad census and hierarchical clustering). Builds older than commit
`34a89e6a` do not print `CLIQUES_MS` and cannot be used.

Networks:

| Group | Networks |
|---|---|
| Dense random (generated, `nets/`) | `ER_N40_p50`, `N60_p50`, `N80_p50`, `N100_p50`, `N150_p50`, `N60_p70`, `N80_p70`, `N100_p70`, `N40_p85`, `N60_p85` |
| Real, shipped (`src/data/`) | `Zachary_Karate_Club.dl`, `Stokman_Ziegler_Corporate_Interlocks_Netherlands.dl` |
| Real, large (`$LARGE_NETS_DIR`) | `Random_ER_Undir_N300_E1735.graphml`, `Random_ER_Undir_N1000_E19879.graphml` |

Dense random graphs are the hard cases: the census cost grows with the number of maximal cliques,
not with the number of nodes. The two shipped real networks finish in under a millisecond and are
there as a sanity check.

## Running

Requires a built `socnetv-cli` (`-DBUILD_CLI=ON`), `python3`, and `perl` (used as a watchdog,
because macOS has no `timeout`).

```bash
# Time the current build; results go to ${TMPDIR:-/tmp}/socnetv-clique-bench
./scripts/max-clique/bench_cliques.sh ./build/socnetv-cli mybuild

# Choose the output directory
./scripts/max-clique/bench_cliques.sh ./build/socnetv-cli mybuild /tmp/clique-bench

# Fewer repetitions for a quick look (default 5; the median is reported)
RUNS=1 ./scripts/max-clique/bench_cliques.sh ./build/socnetv-cli quick /tmp/clique-bench

# Large real networks live elsewhere
LARGE_NETS_DIR=/path/to/nets/large ./scripts/max-clique/bench_cliques.sh ./build/socnetv-cli mybuild
```

A full pass with 5 runs takes a few minutes in a Debug build (the `N100_p70` network alone is about
16 s per run). Cases whose file is missing are skipped with a message.

### Comparing two builds

Keep a copy of the "before" binary, change the code, rebuild, then run both:

```bash
cp build/socnetv-cli /tmp/socnetv-cli.before
# ... edit, rebuild ...
./scripts/max-clique/bench_cliques.sh /tmp/socnetv-cli.before before /tmp/clique-bench
./scripts/max-clique/bench_cliques.sh ./build/socnetv-cli     after  /tmp/clique-bench
./scripts/max-clique/cmp.py /tmp/clique-bench before after
```

To benchmark an older commit, build it in a separate worktree and build directory rather than
switching branches in place.

Output of `cmp.py` (excerpt):

```
network                                                      before     after   delta   cliques  census-identical
ER_N150_p50.net                                                3585      3410   -4.9%     91899  YES
ER_N100_p70.net                                               15899     15656   -1.5%    380059  YES
Random_ER_Undir_N1000_E19879.graphml                            521       348  -33.2%     14103  YES
```

### Regenerating the networks

```bash
./scripts/max-clique/gen.py                 # rewrites scripts/max-clique/nets/
./scripts/max-clique/gen.py /tmp/other-nets # elsewhere
```

Edit `CONFIGS` in `gen.py` to add a size/density; keep the seed fixed so existing files do not change.

### Environment variables

| Variable | Default | Meaning |
|---|---|---|
| `RUNS` | `5` | Repetitions per network; the median is reported. |
| `LARGE_NETS_DIR` | `$HOME/socnetv/library/nets/large` | Directory with the two large real networks. |
| `TMPDIR` | system default | Base for the default output directory. |

## Reading the results

Output files, per label, in the output directory:

- `results_<label>.tsv` — one row per network: name, median `CLIQUES_MS`, total number of maximal
  cliques, and the raw per-run times in parentheses.
- `dump_<label>_<network>.json` — the clustering kernel's full JSON for the last run, used by
  `cmp.py` to compare the census.

How to interpret `cmp.py`:

- **`census-identical` must be `YES` everywhere.** It compares the whole `cliques` block (count per
  size, maximum size, total). A `NO` is a correctness regression, whatever the timings say.
- **Run-to-run noise is about ±5%.** Treat differences under roughly 10% as unchanged.
- **Time follows the clique count, not the node count.** In a Debug build the cost is roughly 40 µs
  per maximal clique on the dense networks (about 20 µs on the sparse ones), because the per-clique
  work — recording the clique and updating the co-membership matrix — dominates. A network with
  380,000 cliques takes about 16 s; one with 90,000 takes about 3.5 s.
- A value of `0` means under 1 ms.

For a trustworthy comparison:

- Use the **same build type** (Debug or Release) for both runs.
- Do not build or run anything heavy while it runs; it measures wall-clock time.
- Run both builds on the same machine in one session. If a small difference matters, alternate the
  two binaries on one network instead of comparing two separate runs.

## Reference numbers

Measured for #310 (the clique-census state/cancellation fixes), Debug build on macOS, median of 5
runs: commit `34a89e6a` (before) against `7895d667` (after). The census was identical on all 14
networks.

| Network | Maximal cliques | Before (ms) | After (ms) |
|---|---:|---:|---:|
| `ER_N150_p50` | 91,899 | 3585 | 3410 |
| `ER_N100_p70` | 380,059 | 15899 | 15656 |
| `ER_N60_p85` | 190,641 | 7785 | 7838 |
| `Random_ER_Undir_N300_E1735` | 1,383 | 25 | 17 |
| `Random_ER_Undir_N1000_E19879` | 14,103 | 521 | 348 |

The dense networks are within noise. The roughly 33% gain on the two large sparse networks was
confirmed by alternating the two binaries on the 1000-node network; its cause was not isolated.
These numbers are a reference for this machine, not a baseline to enforce.
