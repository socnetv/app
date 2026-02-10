# Testing / CI / Regression Roadmap (Skeleton)

## Goal
Prevent silent regressions during modernization.

## Current Reality
- Manual comparisons exist; headless CLI now prints metrics.

## Target Direction
- Golden outputs committed in-repo
- A deterministic comparison tool
- CI job that runs core datasets

## Milestones
- T1: Define output schemas (metrics + per-node vectors)
- T2: Add golden baselines for a small suite of datasets
- T3: Add comparison mode (fail on mismatch)
- T4: Integrate into CI (GitHub Actions)

## Work Rules
- Keep outputs stable (version schemas when changing format).
