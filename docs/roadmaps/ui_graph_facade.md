# Graph as Façade / Coordinator Roadmap (Skeleton)

## Goal
Make `Graph` a thin coordinator: orchestrate services, hold references, expose stable API to UI.

## Current Reality
- UI calls Graph methods that directly implement algorithms and mutate internal scratch state.

## Target Direction
UI → Graph (façade) → services/engines

## Milestones
- F1: Define “façade API” (what UI truly needs)
- F2: Route distance/centrality through extracted engines (already underway)
- F3: Route other analytics through services incrementally
- F4: Reduce direct UI → internal container access

## Work Rules
- Do not break UI behavior; replace internals behind stable API.
