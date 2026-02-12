# Distance & Geodesic Engine Refactor Roadmap

This roadmap documents the ongoing architectural refactor of
`Graph::graphDistancesGeodesic()` into a testable, maintainable engine
while preserving exact behavior and results.

---

## Goals

- Reduce Graph-centric complexity
- Improve maintainability / evolvability
- Enable headless verification and unit tests
- Preserve performance (no regressions vs v3.2)
- Preserve UI behavior during refactor

### Non-negotiables
- Preserve functionality and numeric results (bit-for-bit where possible).
- Preserve current debug output and progress UI signaling behavior unless explicitly changed.
- Keep refactors incremental: compile + run + compare results at every step.

---

## CURRENT STATUS (WHAT’S DONE)

### Phase A — Extraction Foundation ✅
- Introduced `DistanceEngine` as the owner of the geodesic-distance computation previously living in `Graph::graphDistancesGeodesic()`.
- `DistanceEngine` has direct access to `Graph` internals via
  `friend class DistanceEngine;` (explicit transitional design choice).

---

### Phase B — Verified Behavioral Parity ✅
- Verified with Zachary’s Karate Club dataset:
  - identical distances and connectivity handling
  - identical centrality / prestige indices
  - identical results vs SocNetV 3.2 release

---

### Phase C — Scratch State Separation ✅

#### C1 ✅
- Broke computation into three internal methods:
  - `initRun(...)`
  - `runAllSources(...)`
  - `finalize(...)`
- Preserved logic and debug output while reducing `compute()` size.

#### C2.3 ✅
- Introduced explicit scratch structs to eliminate giant parameter lists.
- Scratch structures are now the **single source of truth** for transient algorithm state.

---

## Phase D — Engine Decoupling & Testability

Phase D transitions from “code movement” to **structural safety and testability**
without changing UI behavior.

---

### ✅ D.1 Progress / UI Decoupling (COMPLETED)

**What was done**
- Introduced `IDistanceProgressSink`.
- Implemented:
  - `GraphDistanceProgressSink` (Qt/UI-backed)
  - `NullDistanceProgressSink` (headless / test usage)
- Replaced all direct `emit graph.*` calls with sink calls.

**Result**
- UI behavior unchanged
- DistanceEngine no longer depends directly on Qt
- Headless execution is now possible
- Performance preserved

---

### ✅ D.2 Reduce unnecessary Graph internals access (COMPLETED)

**What was done**
DistanceEngine no longer mutates Graph internals directly.
Instead, intent-revealing Graph APIs encapsulate transient SSSP / Brandes state.

#### 1. Stack encapsulation (SSSP / Brandes)
```cpp
void ssspStackClear();
bool ssspStackEmpty() const;
int  ssspStackTop() const;
void ssspStackPop();
int  ssspStackSize() const;
````

#### 2. Nth-order neighborhood (Power Centrality)

```cpp
void ssspNthOrderClear();
H_f_i sizeOfNthOrderNeighborhood;
H_f_i::const_iterator ssspNthOrderBegin() const;
H_f_i::const_iterator ssspNthOrderEnd() const;
```

#### 3. Component size accumulator

```cpp
void ssspComponentReset(int value = 1);
void ssspComponentAdd(int delta);
int  ssspComponentSize() const;
```

#### 4. Connectivity bookkeeping

```cpp
void notConnectedPairsClear();
void notConnectedPairsInsert(int from, int to);
int  notConnectedPairsSize() const;
```

**Result**

* No algorithmic changes
* Reduced coupling
* Clear semantic boundaries
* Safer future refactors

---

### ✅ D.3 — Golden Regression Harness (COMPLETED — multi-format, graph + per-node)

Phase D.3 establishes a deterministic, format-agnostic regression harness
that protects the DistanceEngine refactor against semantic and numeric drift.

This phase elevates the headless CLI into a **full safety net**.

---

#### What Was Implemented

✔ Headless CLI execution path
✔ Deterministic JSON output
✔ Strict comparison mode (CI-ready)
✔ Multi-format baseline coverage

DistanceEngine is now verifiable independently of UI and MainWindow.

---

#### JSON Regression Schema (v1)

##### Dataset & Run Metadata

* file path / name
* file type
* run flags:

  * computeCentralities
  * considerWeights
  * inverseWeights
  * dropIsolates

##### Graph-Level Metrics

* `nodes` (N)
* `LINKS_SNA` (loader semantics)
* `TIES_GRAPH` (Graph::edgesEnabled canonical ties/arcs)
* directed / weighted flags
* average geodesic distance
* diameter
* disconnected_pairs
* connected

##### Per-Node Vectors (deterministic order by id)

* CC / SCC
* BC / SBC
* SC / SSC
* EC / SEC
* PC / SPC
* distance_sum
* eccentricity

---

#### Determinism Guarantees

* Vertex order strictly sorted by id
* Floating-point values serialized as strings
* NaN values handled explicitly
* Field-by-field JSON comparison
* Non-zero exit on mismatch (CI-safe)

This ensures even subtle algorithmic drift (e.g., stack ordering,
Brandes accumulation changes, loader semantics differences) is detected.

---

#### Baseline Coverage

##### GraphML

* Erdos–Rényi N=10
* Small-world N=10

##### UCINET (.dl)

* Stokman_Ziegler_Corporate_Interlocks_Netherlands.dl

  * weighted / unweighted runs
  * directed semantics verified
  * disconnected_pairs validated

##### Pajek (.paj)

* Dunbar Gelada baboon colony (H22a)

  * undirected valued edges
  * weighted + unweighted runs
  * connectedness verified

---

#### Why This Matters

GraphML behaved correctly for ties.

Historically:

* Pajek and DL imports had subtle tie-count differences
* Weighted flags were sometimes ambiguous
* Loader semantics varied across formats

D.3 guarantees:

* Loader correctness
* Canonical tie semantics
* Connectivity bookkeeping
* Full per-node centrality correctness
* Multi-format parity

DistanceEngine refactors are now protected across:

* directed vs undirected
* weighted vs unweighted
* connected vs disconnected
* sparse vs dense
* multi-format imports

---


### ✅ D.4 — Document engine boundary


#### DistanceEngine owns

* Algorithm flow: `initRun`, `runAllSources`, `finalize`
* Scratch lifetime + invariants
* Use of Graph traversal primitives / accessors only
* Progress reporting **only** via `IDistanceProgressSink`

#### Graph owns

* Storage and access to:

  * vertices/edges (enabled/disabled)
  * per-vertex metric storage (CC/BC/…)
  * connectivity bookkeeping container (notConnectedPairs…)
* “Narrow” algorithm support primitives (already done: stack, nth-order, component counter)
* Cached results exposure (your new `*Cached()` accessors are perfect here)

#### UI owns

* Creating the sink used by DistanceEngine (`GraphDistanceProgressSink`)
* Translating progress to Qt widgets/signals
* Nothing algorithmic


--- 

## CURRENT SHAPE (REFERENCE)

### Engine API

```cpp
DistanceEngine::compute(
    computeCentralities,
    considerWeights,
    inverseWeights,
    dropIsolates
);
```

### Internal structure

* `initRun(...)`
* `runAllSources(...)`
* `finalize(...)`

### Scratch State

* `DistanceScratch` holds:

  * iterators
  * counters
  * N/E snapshots
  * per-run scalars
  * connectivity tracking

---

## NEXT STEPS (WHAT WE DO NEXT)


## D.5 — Physical extraction from `graph.cpp` (NEXT)

Here’s the safest extraction sequence that won’t change behavior:

### Step 1: Create files, don’t change logic

* Add:

  * `src/engine/distance_engine.h`
  * `src/engine/distance_engine.cpp`
* Move the **DistanceEngine class** and implementation from `graph.cpp` into these.
* Keep **exact same code**, just relocated.

### Step 2: Keep access unchanged (transitional)

* Keep `friend class DistanceEngine;` in `Graph`
* Keep any “Graph scratch structs” where they are for now (or move with DistanceEngine if they’re private to it).
* Use forward declarations to avoid include explosions:

  * `class Graph;`
  * `class IDistanceProgressSink;`
* Only include `graph.h` inside `distance_engine.cpp`, not in the header (unless absolutely required).

### Step 3: Wire it back

In `graph.cpp`, replace the inlined engine body with:

* `#include "engine/distance_engine.h"`
* `DistanceEngine engine(*this, sink); engine.compute(...);` (whatever your current call shape is)

### Step 4: Build + run golden compares

* `socnetv-cli --compare-json ...` on *all* baselines
* Ensure **no diffs** in JSON

### Step 5: Only then do cleanup

After it’s safely compiled and baselines pass:

* reduce includes
* tidy headers
* (optionally) split scratch structs if they’re better colocated

---

### D.6 — Micro-benchmarking

* Same datasets, same toggles:

  * weighted / unweighted
  * inverse weights
  * isolates on/off
  * directed vs undirected

**Deliverable**

* Timing output (ms, N, E)
* Performance regression guardrail vs v3.2

---


## Phase E — Regression Guardrails

* Karate Club
* Synthetic graphs (line, star, disconnected, isolates, N=1/2)
* Weighted + inverse-weighted variants

Verify:

* distance summaries
* centrality vectors
* connectivity bookkeeping

---

## Phase F — Optional Improvements (Post-safety-net)

Only after regression coverage exists:

* Narrow or remove `friend` access
* Split centrality algorithms further
* Tighten types (`int` → `qsizetype` etc.)

---

## WORK RULES

* No semantic changes during refactor phases
* One small step per commit
* Always compare against known datasets
* Debug output stays unless explicitly cleaned up later
