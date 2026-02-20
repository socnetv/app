# Matrices Modernization Roadmap (Skeleton)

## Goal
Isolate matrix creation and computations into coherent types and services.

## Current Reality
- Matrix-related logic is scattered and sometimes intertwined with Graph/UI.

## Target Direction
- Clear matrix types (adjacency, laplacian, etc.)
- Deterministic constructors
- Headless tests

## Milestones
- A1: Inventory matrix-related classes and their current callers
- A2: Extract construction code paths
- A3: Add golden outputs for small graphs
