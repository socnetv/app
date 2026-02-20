# IO / Parser Refactor Roadmap (WS4)

## Goal

Reduce tight coupling between `Parser` (Qt signals / threads) and core graph state,
while preserving exact parsing behavior and performance.

This work prepares the ground for WS3 (domain model split).

---

## Current Reality

* `Parser` emits Qt signals.
* `Graph` receives those signals and mutates internal state.
* Parsing, threading, and graph mutation are tightly intertwined.
* CLI headless loading exists but still follows the same internal wiring.

---

## Target Direction

Introduce a clearer IO boundary:

```
Parser / IO
      ↓
Explicit build API
      ↓
Graph (façade)
```

Constraints:

* Parsing logic remains unchanged.
* Qt signal-based pipeline remains operational during transition.
* A deterministic, non-UI entry point must exist.

---

## Structural Rules (Mandatory)

During WS4:

* Parser may depend on QtCore (QString, QFile, etc.).
* Parser must not construct UI widgets.
* Parser must not reach into Graph internals directly.
* Graph must not depend on Parser implementation details.
* All graph mutation must flow through an explicit API (introduced in P3).

No semantic parsing changes are allowed unless explicitly approved.

---

## Milestones

---

### P1 — Document Parser Contract

**Objective:** make the implicit signal-based contract explicit.

Deliverables:

* List all public Parser entry points.
* List all emitted signals.
* For each signal:

  * payload
  * emission timing (per-file / per-node / per-edge / finalization)
  * receiving slot(s) in `Graph`
* Document threading assumptions (who owns Parser, which thread emits).

Definition of Done:

* A documented “Parser → Graph contract” section exists in this file.
* No behavior changes.
* Build + golden comparisons pass.

---

### P2 — Headless Parse-Only Mode

**Objective:** enable deterministic parsing without UI involvement.

Deliverables:

* Entry point that parses a file without UI signal assumptions.
* No dependency on `MainWindow`.
* Works under CLI harness.

Definition of Done:

* CLI can parse datasets and produce deterministic graph state.
* Golden metric comparisons unchanged.
* Performance within guardrails.

---

### P3 — Introduce Explicit Builder / Transaction API

**Objective:** replace implicit signal fan-out with controlled graph mutation.

Deliverables:

* Define a minimal “GraphBuild” API.
* Parser calls explicit mutation methods instead of signal fan-out.
* Qt signal pipeline temporarily coexists if needed.

Definition of Done:

* Parsing no longer relies on signal chaining for state mutation.
* UI load flow still functions.
* Golden + benchmarks pass.

---

### P4 — Golden Parse Tests

**Objective:** guarantee parsing determinism.

Deliverables:

* Deterministic structural snapshot (e.g., node/edge count + checksum).
* Golden tests for:

  * GraphML
  * One additional commonly used format.
* Stored under baseline tooling.

Definition of Done:

* Parsing differences become detectable via regression tooling.
* CI-ready snapshot comparison possible.

---

## Regression Discipline

For every milestone:

* Golden metric comparisons must pass.
* Performance benchmarks must remain within tolerance.
* Parsing semantics must remain identical.

Parsing changes are considered high-risk and must be validated with known datasets.

---

## Sequencing Note

WS4 should be completed before WS3 (domain model split).

Clarifying IO boundaries first prevents domain split from inheriting Qt signal entanglement.

---
