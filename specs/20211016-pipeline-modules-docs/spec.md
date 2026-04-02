# Feature Specification: Pipeline, Module and Buffer Documentation

**Feature Branch**: `20211016-pipeline-modules-docs`  
**Created**: 2026-04-02  
**Status**: Draft  
**Input**: User description: "Przygotuj dokumentacje na podstawie aktualnego kodu projektu. Zwróć szczególną uwage na pipeline, moduły i bufory. W jaki sposób moduły są dołączane do pipeline i jak później wygląda przetwarzanie danych w pipeline."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Developer Understands the Complete Data Flow Through a Pipeline (Priority: P1)

As a firmware developer, I need documentation that explains the complete data processing flow within a SOF pipeline — from the moment a pipeline is scheduled to run, through the graph traversal that visits each component, to the actual reading and writing of audio samples between buffers — so that I can understand how audio data moves through the system and debug processing issues.

**Why this priority**: Understanding data flow is the single most important piece of knowledge for anyone working on SOF audio modules. Every bug fix, performance optimization, or new module implementation requires understanding how `pipeline_copy()` walks the graph, in which order components execute, and how buffers transfer data between them.

**Independent Test**: Can be validated by having a developer read the documentation and correctly trace the processing order of a 4-component playback pipeline (Host → Volume → EQ → DAI), including which component executes first, which direction the graph is walked, and how buffers mediate data transfer at each step.

**Acceptance Scenarios**:

1. **Given** a developer reading the documentation, **When** they look for how a playback pipeline processes data, **Then** they find a clear explanation that the graph is walked upstream (from DAI toward Host), that deeper components execute first, and that each component's `copy()` reads from source buffers and writes to sink buffers.
2. **Given** a developer reading the documentation, **When** they look for how a capture pipeline processes data, **Then** they find that the graph is walked downstream (from DAI/DMIC toward Host) and components execute in source-to-sink order.
3. **Given** a developer debugging a data flow issue, **When** they consult the documentation about buffer state during processing, **Then** they find an explanation of how `avail` and `free` counters in the circular buffer track data, how `comp_get_copy_limits()` calculates the number of frames to process, and how `produce`/`consume` calls advance the read/write pointers.

---

### User Story 2 - Developer Understands How Modules Are Attached to a Pipeline (Priority: P1)

As a firmware developer, I need documentation that explains the complete lifecycle of attaching a module to a pipeline — from IPC-driven creation, through buffer allocation and connection, to the graph finalization step — so that I can understand how the pipeline graph is built at runtime from topology definitions.

**Why this priority**: Module attachment defines the pipeline graph topology. Without understanding how `INIT_MODULE`, `BIND`, and `pipeline_complete()` work together, a developer cannot reason about pipeline structure, debug connection issues, or understand why a module receives certain buffer configurations.

**Independent Test**: Can be validated by a developer reading the documentation and correctly describing the sequence of operations that creates a pipeline with two modules and one buffer between them, including which IPC commands are issued, what data structures are allocated, and how the linked lists connect them.

**Acceptance Scenarios**:

1. **Given** a developer reading the documentation, **When** they look for the module creation flow, **Then** they find that `INIT_MODULE` IPC triggers `comp_new_ipc4()`, which locates the matching driver by UUID, allocates a `comp_dev` and (for module_adapter) a `processing_module`, calls the module's `init()`, and adds the component to the pipeline's component list.
2. **Given** a developer reading the documentation, **When** they look for how two modules are connected, **Then** they find that the `BIND` IPC allocates a `comp_buffer` sized at `2 * MAX(source_obs, sink_ibs)`, calls `pipeline_connect()` twice (source→buffer and buffer→sink), and notifies both modules via `comp_bind()`.
3. **Given** a developer reading the documentation, **When** they look for what `pipeline_complete()` does, **Then** they find that it walks the graph downstream from the source component, sets each component's `pipeline` pointer, identifies the `source_comp` and `sink_comp` endpoints, and transitions the pipeline to READY state.

---

### User Story 3 - Developer Understands Buffer Architecture and the Source/Sink Abstraction (Priority: P2)

As a firmware developer, I need documentation that explains the buffer system in detail — the `comp_buffer` structure with its circular `audio_stream`, the modern source/sink API abstraction, and how the ring buffer enables cross-domain (LL↔DP) communication — so that I can correctly implement modules that read and write audio data.

**Why this priority**: Buffers are the fundamental data transport mechanism between modules. The coexistence of the legacy `audio_stream` direct-access pattern and the modern `sof_source`/`sof_sink` API, plus the hybrid ring buffer for DP modules, creates complexity that must be documented for developers to choose the right approach.

**Independent Test**: Can be validated by a developer reading the documentation and correctly explaining the difference between accessing a buffer via `audio_stream` read/write pointers versus using the source/sink API, and when each approach is appropriate.

**Acceptance Scenarios**:

1. **Given** a developer reading the documentation, **When** they look for how `comp_buffer` connects two components, **Then** they find that each buffer has a `source` component pointer (producer) and a `sink` component pointer (consumer), and is doubly linked into the source's `bsink_list` and the sink's `bsource_list`.
2. **Given** a developer reading about the source/sink API, **When** they look for how a modern module reads input data, **Then** they find that it uses `source_get_data_frames_available()` to check available data, `source_get_data()` to obtain a read pointer, processes the data, and calls `source_release_data()` to advance the read position.
3. **Given** a developer reading about DP↔LL bridging, **When** they look for how a DP module communicates with an LL pipeline, **Then** they find that a lockless `ring_buffer` is created as a secondary buffer, the DP thread reads/writes via the source/sink API on the ring buffer, and the LL scheduler synchronizes data between the ring buffer and the `audio_stream` via `module_adapter_copy_ring_buffers()`.

---

### User Story 4 - Developer Understands Pipeline Scheduling (Priority: P2)

As a firmware developer, I need documentation explaining how pipelines are scheduled for execution — the LL (Low Latency) timer/DMA-driven scheduling model and the DP (Data Processing) per-component thread model — so that I can understand when and how often components process data and how the two scheduling domains interact.

**Why this priority**: Scheduling determines real-time behavior. Developers need to understand the difference between LL scheduling (one task per pipeline, periodic timer/DMA driven) and DP scheduling (one thread per component, period derived from buffer sizes) to correctly design modules and debug timing issues.

**Independent Test**: Can be validated by a developer reading the documentation and correctly describing which type of scheduling a given module uses based on its `proc_domain` configuration, and how a pipeline containing both LL and DP components is scheduled.

**Acceptance Scenarios**:

1. **Given** a developer reading the documentation, **When** they look for LL scheduling, **Then** they find that each pipeline has a `pipe_task` scheduled by the LL scheduler (timer or DMA driven), and `pipeline_task()` calls `pipeline_copy()` to walk the graph and invoke `comp_copy()` on each component within a single scheduling period.
2. **Given** a developer reading the documentation, **When** they look for DP scheduling, **Then** they find that each DP component gets its own preemptible thread, its period is calculated from OBS and sample rate (aligned to LL cycle boundaries), and the thread runs `module_process_sink_src()` independently from the LL task.
3. **Given** a developer reading about hybrid pipelines, **When** they look for how LL and DP components coexist, **Then** they find that the ring buffer acts as the asynchronous boundary: the LL task synchronizes data between the `audio_stream` and the ring buffer, while the DP thread accesses the ring buffer independently.

---

### User Story 5 - Developer Understands the Module Adapter Pattern (Priority: P3)

As a firmware or third-party module developer, I need documentation explaining the module adapter pattern — how the `module_interface` is wrapped by `module_adapter` to present standard `comp_ops` to the pipeline engine — so that I can implement new audio processing modules using the preferred modern interface.

**Why this priority**: Nearly all modern SOF components use `SOF_COMP_MODULE_ADAPTER`. Understanding the adapter pattern is essential for module authors, but it builds on the foundational knowledge from the higher-priority stories about pipelines and buffers.

**Independent Test**: Can be validated by a module developer reading the documentation and correctly identifying which `module_interface` methods they need to implement, how the adapter routes `comp_copy()` calls to their module's `process()` method, and what the three processing modes (source/sink, audio_stream, raw_data) mean.

**Acceptance Scenarios**:

1. **Given** a module author reading the documentation, **When** they look for how to register a new module, **Then** they find a description of the `DECLARE_MODULE_ADAPTER` pattern that creates a `comp_driver` with `module_adapter_*` ops and links their `module_interface`.
2. **Given** a module author reading about processing modes, **When** they look for which mode to use, **Then** they find that the `process()` method (source/sink mode) is the preferred modern approach, while `process_audio_stream()` and `process_raw_data()` are deprecated alternatives maintained for backward compatibility.
3. **Given** a module author reading about the copy dispatch, **When** they look for how their `process()` method is invoked, **Then** they find that `module_adapter_copy()` checks the processing mode and dispatches to the appropriate handler, which then calls the module's interface method with the bound source and sink arrays.

---

### Edge Cases

- What happens when a pipeline graph has a component with no source buffers (DAI/DMIC capture endpoint)? The graph walker starts from this component and walks downstream; it has no input buffer to read from and instead reads from hardware via DMA.
- What happens when a component is connected to multiple sink buffers (fan-out, e.g., a mixer output going to multiple paths)? The graph walker iterates all buffers in the component's `bsink_list` and recurses into each path, using the `walking` flag to prevent cycles.
- What happens when a buffer between two components is empty during `pipeline_copy()`? The component's `comp_copy()` calls `comp_get_copy_limits()` which returns 0 frames available, so the component processes 0 frames (no-op) and returns success.
- What happens when the ring buffer for DP↔LL communication overflows or underflows? The ring buffer uses modular arithmetic on offsets; a write that would exceed capacity returns an error, and the LL synchronization layer handles the underrun by reporting it to the pipeline's XRUN handling mechanism.
- How does the pipeline handle an XRUN (buffer underrun/overrun during streaming)? `pipeline_task()` calls `pipeline_xrun_check()` before processing; if detected, it posts an XRUN notification IPC to the host and may attempt recovery based on the `xrun_action` policy.
- What happens when `pipeline_connect()` is called with an already-connected buffer? The buffer's `source` or `sink` pointer is overwritten; the caller (IPC handler) is responsible for ensuring correct ordering.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Documentation MUST describe the pipeline lifecycle from creation through completion, including the state machine transitions (INIT → READY → PRE_ACTIVE → ACTIVE → PAUSED) and the IPC commands that trigger each transition.
- **FR-002**: Documentation MUST explain the graph traversal mechanism (`pipeline_for_each_comp`) that is the foundation for all pipeline operations — copy, trigger, prepare, params, reset, and complete — including the walk direction, callback pattern, and cycle-prevention via the `walking` flag.
- **FR-003**: Documentation MUST describe how modules are instantiated via IPC (`INIT_MODULE`), including driver lookup by UUID, `comp_dev` allocation, module adapter initialization, and DP task thread creation for DP-domain modules.
- **FR-004**: Documentation MUST explain how modules are connected via IPC (`BIND`), including buffer size calculation (`2 * MAX(source_obs, sink_ibs)` for LL, `2 * ibs` for DP), `comp_buffer` allocation, `pipeline_connect()` linking, and `comp_bind()` notification.
- **FR-005**: Documentation MUST describe the `comp_buffer` structure in detail: its relationship to `sof_audio_buffer`, the `audio_stream` circular buffer with read/write pointers and avail/free counters, the `source`/`sink` component pointers, and the doubly-linked list integration into component buffer lists.
- **FR-006**: Documentation MUST explain the source/sink API (`sof_source`, `sof_sink`) as the modern abstraction for data access, including the key operations (get available, get/release data, get free, get/commit buffer) and when modules should use this API versus direct `audio_stream` access.
- **FR-007**: Documentation MUST describe the ring buffer mechanism for cross-domain DP↔LL communication: its lockless design, sizing at `2 * MAX(IBS, OBS)`, attachment as a secondary buffer, and the synchronization performed by `module_adapter_copy_ring_buffers()`.
- **FR-008**: Documentation MUST explain the data processing flow in `pipeline_copy()`: the direction-dependent walk (upstream for playback, downstream for capture), the recursive execution order, and what each component's `comp_copy()` does (read from sources, process, write to sinks, update produce/consume).
- **FR-009**: Documentation MUST describe both scheduling models: LL scheduling (one `pipe_task` per pipeline, `pipeline_task()` entry point, timer/DMA triggered) and DP scheduling (one thread per component, period derived from OBS/sample rate, aligned to LL boundaries).
- **FR-010**: Documentation MUST explain the module adapter pattern: how `DECLARE_MODULE_ADAPTER` registers a module, how `module_adapter_copy()` dispatches to the correct processing mode, and the three processing modes (source/sink, audio_stream, raw_data) with their deprecation status.
- **FR-011**: Documentation MUST describe `pipeline_complete()` as the graph finalization step: how it walks the graph, sets component `pipeline` pointers, identifies source/sink endpoints, and transitions the pipeline to READY state.
- **FR-012**: Documentation MUST include flow descriptions for at least 3 key operations: the end-to-end pipeline creation sequence (IPC commands), the data processing flow during a single pipeline copy period, and the module binding/buffer connection sequence.
- **FR-013**: Documentation MUST list all source files comprising the pipeline, module adapter, and buffer subsystems with their roles and relationships.
- **FR-014**: Documentation MUST be written using Doxygen-compatible comments for inline code documentation and Markdown for architectural documents, consistent with SOF project conventions defined in AGENTS.md.
- **FR-015**: Documentation MUST describe the `comp_ops` virtual method table and how the pipeline engine invokes module operations (create, params, prepare, copy, trigger, reset, free, bind, unbind) through this interface.
- **FR-016**: Documentation MUST explain how stream parameters (format, sample rate, channels) are propagated through the pipeline graph via `pipeline_params()` and how each component validates and adapts parameters for its downstream connections.

### Key Entities

- **Pipeline** (`struct pipeline`): A directed graph of connected audio components that processes one stream. Contains scheduling metadata (period, priority, time domain), state machine, endpoint component pointers (source_comp, sink_comp), and the scheduler task. Identified by a unique pipeline_id.
- **Component** (`struct comp_dev`): An individual audio processing unit within a pipeline. Has a state machine, a reference to its driver (containing the operations vtable), lists of source and sink buffers, input/output buffer size requirements (IBS/OBS), and a processing domain (LL or DP). Identified by a unique component ID.
- **Buffer** (`struct comp_buffer`): The data transport unit connecting exactly one source component to one sink component. Contains a circular audio stream with read/write pointers, linked-list entries for the source component's sink list and the sink component's source list, and an optional secondary ring buffer for cross-domain communication.
- **Module Interface** (`struct module_interface`): The API contract for modern audio modules. Defines processing callbacks (process, init, prepare, reset, free, bind, unbind, trigger) and configuration methods. Wrapped by the module adapter to present standard `comp_ops` to the pipeline engine.
- **Module Adapter** (`struct processing_module`): The intermediary between the pipeline engine and a module implementation. Holds the bound source/sink arrays, tracks the processing mode, and dispatches `comp_copy()` calls to the correct module interface method.
- **Audio Stream** (`struct audio_stream`): The circular buffer implementation within a `comp_buffer`. Manages read pointer, write pointer, available bytes, free bytes, and stream parameters (format, rate, channels).
- **Ring Buffer**: A lockless producer/consumer circular buffer used for asynchronous data transfer between LL-scheduled and DP-scheduled components. Sized at `2 * MAX(IBS, OBS)` and attached as a secondary buffer to `comp_buffer`.
- **Pipeline Walk Context** (`struct pipeline_walk_context`): The callback-carrying structure used by `pipeline_for_each_comp()` to traverse the pipeline graph. Contains component callback, buffer callback, walk direction, and cycle-detection state.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A developer unfamiliar with SOF pipelines can correctly trace the processing order of a 4-component pipeline after reading only the documentation (validated by peer review).
- **SC-002**: Documentation covers the complete end-to-end pipeline lifecycle: all 5 IPC commands (CREATE_PIPELINE, INIT_MODULE, BIND, SET_PIPELINE_STATE, plus the implicit pipeline_complete) are described with their effects on data structures.
- **SC-003**: All 8 key entities (Pipeline, Component, Buffer, Module Interface, Module Adapter, Audio Stream, Ring Buffer, Walk Context) are documented with their roles, key attributes, and relationships.
- **SC-004**: At least 3 flow descriptions are provided: pipeline creation sequence, data processing flow during one copy period, and module binding/buffer connection.
- **SC-005**: Both scheduling models (LL and DP) and their interaction via ring buffers are explained clearly enough that a developer can determine which model a given module uses and why.
- **SC-006**: The documentation build completes without introducing new warnings or errors related to pipeline, buffer, or module adapter files.
- **SC-007**: All source files in the pipeline, module adapter, and buffer subsystems (at least 15 files across headers and implementations) are cataloged with their roles.

## Assumptions

- The primary audience is SOF firmware developers and third-party module authors with basic familiarity with embedded C, real-time scheduling concepts, and circular buffer principles.
- Documentation covers the `pipeline2.0` branch codebase, including the ring buffer cross-domain mechanism and the source/sink API; deprecated patterns (`process_audio_stream`, `process_raw_data`) are mentioned for context but not the primary focus.
- The existing `ARCHITECTURE.md` at the repository root provides a high-level overview; this documentation expands on the pipeline/module/buffer subsystems in depth without duplicating the high-level content.
- Both IPC3 and IPC4 paths exist in the codebase; documentation focuses on the IPC4 flow as the current standard, with IPC3 mentioned only where differences are significant.
- The scope covers the data path (pipeline processing, buffer management, module invocation) but not the control path in depth (e.g., detailed IPC message encoding, topology parser, host driver internals).
- Documentation will follow SOF conventions: Doxygen for inline code documentation, Markdown for architectural documents, as specified in AGENTS.md.
