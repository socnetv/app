# Domain Model Split Roadmap (Skeleton)

## Goal
Introduce a domain model that is independent from UI concerns and can be tested headlessly.

## Current Reality
- `Graph` mixes storage, algorithm state, caches, and UI signaling.
- `GraphVertex` acts as both node storage and analysis result cache.

## Target Direction
- Separate “model” (nodes/edges/relations) from “services/algorithms”.
- Keep `Graph` as façade during transition.

## Milestones
- M1: Identify minimal model surface required by algorithms
- M2: Introduce `GraphModel` (adapter over existing Graph internals initially)
- M3: Move pure data containers out of UI/Qt dependencies where possible
- M4: Gradually relocate caches into explicit cache objects

## Work Rules
- Prefer adapters/wrappers first, not data migrations.
