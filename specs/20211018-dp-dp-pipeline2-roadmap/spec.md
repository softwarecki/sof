# Feature Specification: DP-to-DP Connections and Pipeline 2.0 Roadmap

**Feature Branch**: `20211018-dp-dp-pipeline2-roadmap`  
**Created**: 2026-04-02  
**Status**: Draft  
**Input**: User description: "DP-to-DP connections and Pipeline 2.0 roadmap: scheduling fixes, buffer abstraction, module binding overhaul — based on handoff TODO list from departing colleague."

## Clarifications

### Session 2026-04-02

- Q: Can P2 (buffer factory / direct binding) development proceed in parallel with P1 (sink/source migration), or must P1 be fully complete first? → A: Parallel per-module — P2 can be developed and tested alongside P1 as individual modules are migrated; P2 is only "complete" when P1 is complete.
- Q: When a module is unbound while a downstream consumer is mid-processing from its exposed source, what should happen? → A: Operation not supported — unbind MUST fail and return an error if a consumer is currently mid-processing on the affected source/sink.
- Q: What buffer type should the buffer factory select for same-core DP-to-DP connections? → A: Ring buffer (cached) — use a standard cached ring buffer for same-core DP-to-DP; reserve shared (non-cached) ring buffer only for cross-core connections.
- Q: Is the "at least 30% less buffer memory" target (SC-003) a hard requirement or an aspirational estimate? → A: No specific percentage target — success is that double-buffering is eliminated for modules exposing internal storage, and memory usage does not increase.
- Q: Should DP-to-DP (P4) be treated as a speculative design exercise or a committed deliverable with mandatory test coverage? → A: Committed deliverable — P4 is fully implemented and tested as part of this roadmap, with the 3-stage DP chain test (SC-005, SC-006) as hard acceptance criteria.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Migrate All Modules to Exclusive Sink/Source API Usage (Priority: P1)

A firmware developer maintaining an audio processing module currently accesses data through a mix of legacy `comp_buffer` / `audio_stream` interfaces and the modern sink/source API. The project needs every module to interact with data exclusively through the sink/source API. This means no module may directly reference `comp_buffer`, `audio_stream` read/write pointers, or any buffer-specific structure. If a module needs a capability not yet available in the sink/source API, the API must be extended. Once all modules are migrated, the hybrid buffer workaround (secondary buffer attach/sync) and double buffering become unnecessary and can be removed, simplifying the data path and reducing memory consumption.

**Why this priority**: This is the foundational prerequisite for every other Pipeline 2.0 improvement. DP-to-DP connections, buffer factory, shared buffers, and direct module-to-module binding all depend on modules using only the abstract sink/source interface. Without this migration, none of the downstream work items are feasible.

**Independent Test**: Pick any single module (e.g., the gain module). After migration, verify it compiles and passes all existing tests while having zero references to `comp_buffer`, `audio_stream`, or any buffer-internal structure. The module should access data solely through `source_get_data()` / `source_release_data()` and `sink_get_buffer()` / `sink_commit_buffer()`.

**Acceptance Scenarios**:

1. **Given** a module currently using legacy audio_stream pointers, **When** it is migrated to sink/source API, **Then** it produces bit-identical output for the same input data and passes all existing tests.
2. **Given** all modules are migrated, **When** the hybrid buffer secondary attach/sync mechanism is removed, **Then** all pipelines (LL-to-LL, LL-to-DP, DP-to-LL) continue to function correctly.
3. **Given** the copier module, **When** it exposes its internal buffers via sink/source API, **Then** downstream modules read directly from the copier's internal storage without an extra copy step, eliminating double buffering.

---

### User Story 2 - Enable Direct Module-to-Module Binding Without Intermediate Buffer (Priority: P2)

A pipeline designer wants to connect two modules directly — without an explicit buffer object between them — when the producing module can expose a sink/source interface on its own internal storage. Today, every bind between modules requires creating an intermediate buffer (comp_buffer or audio_buffer). In the new model, a bind operation always connects two modules. When a buffer is needed between them (e.g., cross-domain DP↔LL, or when neither module provides internal storage), the bind operation creates one automatically through a buffer factory as an internal implementation detail. This removes the current rigid one-type buffer creation and enables the proposed buffer types (ring buffer, shared buffer) to be selected based on the connection requirements.

**Why this priority**: Direct module binding is the architectural enabler for Pipeline 2.0. It decouples the pipeline graph from buffer allocation, allows the buffer factory to optimize memory usage (especially shared buffers saving HPSRAM), and simplifies the pipeline traversal from recursive component walks to flat ordered lists.

**Independent Test**: Create a test pipeline where the copier module exposes its internal buffer via sink/source. Bind a downstream module to the copier module. Verify data flows correctly without any intermediate buffer object being allocated. Then create a second test where neither module exposes internal storage — verify the buffer factory creates the appropriate type (ring buffer for cross-domain, shared buffer for multi-core).

**Acceptance Scenarios**:

1. **Given** a module that exposes a source on its internal storage, **When** another module binds to it, **Then** the bind succeeds without creating an intermediate buffer, and data flows correctly.
2. **Given** two modules that do not expose internal storage, **When** they are bound, **Then** the buffer factory automatically creates a buffer of the appropriate type based on connection properties (same-core, cross-core, LL-DP bridging).
3. **Given** a bind operation between two modules, **When** the bind is executed, **Then** it succeeds regardless of whether either module exposes internal storage — the buffer factory transparently handles buffer creation when needed.
4. **Given** the mixin/mixout module, **When** it exposes its internal buffers via sink/source, **Then** it can be bound directly to upstream/downstream modules, eliminating the external buffer copy.

---

### User Story 3 - Replace Recursive Pipeline Traversal with Flat Ordered Lists (Priority: P3)

A system developer maintaining pipeline execution code currently traces a recursive graph-walk to determine module execution order. This recursive traversal is complex, error-prone, and makes it difficult to reason about execution order or insert ordering constraints. In the new model, each pipeline maintains a simple flat ordered list of modules to execute. For IPC4 this list is built during the bind operation. For IPC3 backward compatibility, the existing recursive traversal can remain but must produce the same flat list during bind, which is then used at runtime.

**Why this priority**: Flat execution lists simplify the scheduler, make debugging easier, and are a prerequisite for correct DP-to-DP deadline chain propagation (which needs to walk the module list to compute deadlines backward from the sink).

**Independent Test**: For an existing IPC4 pipeline topology, compare the module execution order produced by the flat list (built at bind time) with the order produced by the current recursive traversal. They must be identical. Then remove the recursive walk from the IPC4 runtime path and verify all tests pass.

**Acceptance Scenarios**:

1. **Given** an IPC4 pipeline, **When** modules are bound, **Then** a flat ordered execution list is built and stored on the pipeline, reflecting the correct processing order.
2. **Given** the flat execution list, **When** the pipeline processes data, **Then** modules are invoked in list order without any recursive graph traversal.
3. **Given** an IPC3 pipeline, **When** modules are bound using the legacy recursive approach, **Then** the recursive traversal produces the same flat ordered list, which is used at runtime.

---

### User Story 4 - Enable DP-to-DP Module Connections with Correct Deadline Propagation (Priority: P4)

A pipeline designer needs to chain multiple DP modules together (DP1 → DP2 → DP3 → ... → LL). Today, DP-to-DP connections are not implemented — the code path is unreachable and explicitly marked with a TODO noting cache incoherence issues. Two prerequisites must be met: (1) module scheduling parameters (period, LPT, readiness state) must be accessible cross-core from non-cached memory, and (2) the buffer abstraction (from P1/P2) must be in place. Once these are satisfied, the deadline calculation must propagate backward through the entire DP chain — not just look at the nearest buffer — using the LFT/LST/LPT formulas already documented in `dp_scheduling.rst`. The delayed start mechanism must also be extended to work until the *next* module in the chain becomes ready for the first time (currently it only works until the first processing completion, which is sufficient only when the next module is LL).

**Why this priority**: DP-to-DP is the capstone feature that depends on P1 (sink/source migration), P2 (buffer abstraction), and P3 (flat lists for chain traversal). The colleague's notes indicate there may not even be a use case yet, but the design with details exists in the documentation and the code has reserved TODOs. This is the logical final step.

**Independent Test**: Create a test pipeline with 3 DP modules chained: DP1(5ms) → DP2(10ms) → DP3(20ms) → LL(1ms). Verify that deadline calculations propagate correctly from LL back through DP3, DP2, DP1. Verify delayed start holds data until downstream DP becomes ready (not just until first processing completion). Verify the system works cross-core with module parameters accessed from non-cached memory.

**Acceptance Scenarios**:

1. **Given** a pipeline DP1 → DP2 → LL, **When** both DP modules are scheduled, **Then** DP1's deadline is calculated based on DP2's LST and buffer state (not just the nearest buffer fill level), matching the formula in `dp_scheduling.rst`.
2. **Given** a DP module in delayed start state, **When** the next module in the chain is also DP, **Then** the delayed start holds data until that next DP module becomes ready for the first time (not just until first processing cycle completion).
3. **Given** DP modules running on different cores, **When** the scheduler calculates deadlines, **Then** it reads module parameters (period, LPT, readiness) from non-cached memory aliases, ensuring cross-core coherence.
4. **Given** the correction formula for producer period < consumer period in DP-to-DP chains, **When** deadlines are calculated, **Then** the multi-cycle correction is applied correctly: `correction = LPT(producer) × ((consumer_period − data_in_buffer) / producer_period)`, clamped to zero if negative.

---

### User Story 5 - Add DP Scheduling Trace/Diagnostics System (Priority: P5)

A developer debugging a real-time audio pipeline needs visibility into DP scheduling decisions: which modules were deemed "ready", what deadlines were calculated, which module was selected by EDF, and whether any deadline was missed. Standard per-event logging is too slow for the 1ms LL tick rate. Instead, a lightweight accumulation-based tracing system should collect scheduling data (readiness, deadlines, selected task) over several milliseconds and emit a single consolidated trace line periodically (e.g., every 10ms or configurable interval), minimizing performance overhead while providing actionable diagnostics.

**Why this priority**: Tracing is critical for validating the correctness of all the above features (especially DP-to-DP deadline propagation and delayed start fixes), but it is not a blocker for any functional feature. It can be developed independently at any point.

**Independent Test**: Run a known pipeline with 2+ DP modules. Enable the DP scheduling trace. Verify that trace output shows, for each accumulation window: which DP modules were ready, their calculated deadlines, which was selected, and whether any deadline was missed. Verify the trace overhead does not cause deadline misses that don't occur without tracing enabled.

**Acceptance Scenarios**:

1. **Given** a running pipeline with DP modules, **When** DP scheduling tracing is enabled, **Then** the system periodically emits consolidated trace lines containing: module ID, readiness state, calculated deadline, and whether the module was selected for execution.
2. **Given** the tracing system, **When** a DP module misses its deadline, **Then** the trace output includes the missed deadline event with the module ID, expected deadline, and actual completion time.
3. **Given** the tracing system running at default accumulation interval, **When** compared to the same pipeline without tracing, **Then** no additional deadline misses occur that would indicate unacceptable trace overhead.

---

### Edge Cases

- What happens when a DP module that exposes internal storage via sink/source is unbound while a downstream module is mid-processing from that source? **Resolved**: The unbind operation MUST fail and return an error; unbinding while a consumer is mid-processing is not supported.
- How does the buffer factory decide between ring buffer and shared buffer when the connection is same-core DP-to-DP? **Resolved**: Same-core DP-to-DP uses a cached ring buffer; shared (non-cached) ring buffer is reserved for cross-core connections only.
- What happens if a bind operation is requested between two sinks (or two sources) — invalid topology detection?
- How does flat list ordering handle fan-out topologies (one module feeding multiple downstream modules)?
- What happens when a DP-to-DP pipeline has modules with identical deadlines — how does EDF break ties?
- How does delayed start interact with dynamic pipeline reconfiguration (module added/removed mid-stream)?
- What happens when the accumulation-based tracing buffer fills up before the next emit interval — oldest data dropped or emit forced?
- How are non-integer sample rates (44.1 kHz) handled in DP-to-DP deadline correction arithmetic?

## Requirements *(mandatory)*

### Functional Requirements

**Sink/Source API Migration (P1)**

- **FR-001**: Every audio processing module MUST access data exclusively through the sink/source API — no direct references to `comp_buffer`, `audio_stream`, or buffer-internal structures.
- **FR-002**: The sink/source API MUST be extended to cover any capability currently accessed through buffer-specific interfaces, so that modules do not need workarounds.
- **FR-003**: The copier module MUST expose its internal buffers via sink/source API, allowing downstream consumers to read directly from the copier's storage.
- **FR-004**: The mixin/mixout module MUST expose its internal buffers via sink/source API, allowing direct producer/consumer binding.
- **FR-005**: After all modules are migrated, the hybrid buffer mechanism (secondary buffer attach/sync) and associated double buffering MUST be removable without breaking any pipeline type (LL-LL, LL-DP, DP-LL).

**Module Binding and Buffer Factory (P2)**

- **FR-006**: The bind operation MUST connect two modules — binding is always module-to-module. Buffers, if needed, are created internally by the buffer factory as an implementation detail of the bind.
- **FR-007**: A bind between two modules MUST succeed without creating an intermediate buffer if the producing module exposes a source on its internal storage and the consuming module exposes a sink.
- **FR-008**: When a bind requires an intermediate buffer, a buffer factory MUST create a buffer of the appropriate type based on connection properties: cached ring buffer for same-core DP-to-DP, shared (non-cached) ring buffer for cross-core connections, ring buffer for LL-DP bridging.
- **FR-009**: The buffer factory MUST support at least the following buffer types: ring buffer (cross-domain async) and shared buffer (multi-core with non-cached memory). The current legacy buffer (comp_buffer/audio_stream) is not a target type and MUST be removed once all modules are migrated to sink/source API.
- **FR-010**: The shared buffer type MUST use non-cached memory to enable coherent cross-core access, providing HPSRAM savings compared to the current per-core cached buffer approach.

**Flat Execution Lists (P3)**

- **FR-011**: Each pipeline MUST maintain a flat ordered list of modules reflecting the correct processing execution order.
- **FR-012**: For IPC4 pipelines, the flat execution list MUST be built during the bind operation.
- **FR-013**: For IPC3 pipelines, the existing recursive traversal MAY remain but MUST produce the same flat execution list during bind, which is then used at runtime.
- **FR-014**: Runtime pipeline processing MUST iterate the flat list — no recursive graph traversal at processing time for IPC4.

**DP-to-DP Scheduling (P4)**

- **FR-015**: The system MUST support DP-to-DP module connections, allowing chains of multiple DP modules terminating at an LL module.
- **FR-016**: Module scheduling parameters (period, LPT, readiness state) MUST be accessible from any core via non-cached memory aliases, enabling cross-core deadline calculations.
- **FR-017**: Deadline calculation for DP modules MUST propagate backward through the entire DP chain using the documented LFT/LST/LPT formulas — not just the nearest buffer fill level.
- **FR-018**: The delayed start mechanism MUST hold data until the next module in the chain becomes ready for the first time (not just until the current module's first processing completion).
- **FR-019**: The multi-cycle correction for producer period < consumer period MUST be applied correctly in DP-to-DP chains: `correction = LPT(producer) × ((consumer_period − data_in_buffer) / producer_period)`, clamped to zero if negative.

**DP Scheduling Diagnostics (P5)**

- **FR-020**: The system MUST provide a lightweight tracing mechanism that accumulates DP scheduling decisions (readiness, deadlines, selected task) over a configurable interval and emits consolidated output periodically.
- **FR-021**: The tracing system MUST report deadline miss events including: module identifier, expected deadline, and actual completion time.
- **FR-022**: The tracing system's overhead MUST be low enough that enabling it does not cause deadline misses in pipelines that meet deadlines without tracing.

### Key Entities

- **Module (Processing Module)**: An audio processing unit that consumes data from sources and produces data to sinks. May optionally expose its internal storage as sink/source interfaces. Has scheduling parameters: period, LPT (Longest Processing Time), IBS (input buffer size), OBS (output buffer size).
- **Sink/Source API**: Abstract data access interface. A source provides data for reading; a sink accepts data for writing. Can be backed by a buffer or by a module's internal storage.
- **Buffer Factory**: A mechanism that creates the appropriate buffer type during bind operations based on connection characteristics (domain crossing, core assignment, sharing requirements).
- **Shared Buffer**: A buffer type using non-cached memory for coherent multi-core access. Reduces HPSRAM usage by avoiding per-core cached copies.
- **Bind Operation**: The act of connecting two modules in a pipeline. In Pipeline 2.0, binding is always module-to-module. If an intermediate buffer is needed (e.g., cross-domain, neither module exposes internal storage), the buffer factory creates it transparently as part of the bind.
- **Flat Execution List**: An ordered list of modules attached to a pipeline, built at bind time, used at runtime to drive processing without recursive graph traversal.
- **Deadline Calculation Chain**: A backward-propagating computation from the LL sink through all DP modules in the path, computing LFT, deadline, LST, and LPT for each stage.
- **Delayed Start**: A mechanism during pipeline startup that holds a module's output data until the downstream module becomes ready, preventing underruns and enabling correct EDF scheduling.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Zero modules reference `comp_buffer` or `audio_stream` directly — all data access goes through sink/source API (verified by static analysis or code review).
- **SC-002**: The hybrid buffer secondary attach/sync code path is removed from the codebase with no test regressions.
- **SC-003**: Pipelines with copier or mixin/mixout exposing internal buffers via direct binding do not allocate redundant intermediate buffers — total buffer memory usage does not increase compared to the current configuration, and double-buffering is eliminated where modules expose internal storage (measured on a representative pipeline).
- **SC-004**: All existing pipeline topologies (LL-LL, LL-DP, DP-LL) pass their test suites after the bind/buffer factory rework.
- **SC-005**: A 3-stage DP chain (DP1 → DP2 → DP3 → LL) runs without underruns or deadline misses at steady state on a representative platform.
- **SC-006**: Delayed start correctly holds data in a DP-to-DP chain until the downstream DP module becomes ready (verified by test with 2+ DP stages).
- **SC-007**: DP scheduling trace output is produced at the configured interval with no measurable increase in deadline misses when tracing is enabled.
- **SC-008**: IPC4 pipeline runtime processing uses flat list iteration — no recursive calls in the processing hot path (verified by code review or profiling).

## Assumptions

- The formulas in `dp_scheduling.rst` (LFT, LST, LPT, deadline, multi-cycle correction) are mathematically correct for DP-to-DP chains, though the existing code implementation only covers DP-to-LL. The code referenced in the departing colleague's TODO is a starting point but may need corrections.
- DP-to-DP connections are a committed deliverable. Although the departing colleague noted there may not be a production use case today, P4 is fully in scope with mandatory test coverage (SC-005, SC-006). The design exists in documentation and code TODOs are reserved.
- P1 (sink/source migration) and P2 (buffer factory / direct binding) may proceed in parallel on a per-module basis. P2 development and testing can start as soon as individual modules are migrated in P1. P2 is considered complete only after P1 is fully complete. P3 and P4 similarly benefit from incremental P1 progress.
- IPC3 backward compatibility is required but not a primary driver — the recursive traversal can remain for IPC3 as long as it produces the flat execution list.
- "Shared buffer" refers to a buffer using non-cached (coherent) memory for multi-core access, as already defined by the `BUFFER_USAGE_SHARED` flag and ring buffer implementation. The HPSRAM savings come from avoiding duplicate cached copies per core.
- The tracing system in P5 is a firmware-level diagnostic, not a host-side tool. It accumulates data in a small fixed-size buffer and emits via the existing trace infrastructure.
- The sink/source API as currently defined in `source_api.h` / `sink_api.h` is the target interface. Extensions needed during migration will be designed as they are discovered, not pre-specified.
- The buffer factory does not need to support runtime buffer type switching — the buffer type is determined once at bind time and remains fixed for the lifetime of the connection.
