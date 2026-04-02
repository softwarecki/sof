# Feature Specification: DP Scheduling Documentation Expansion

**Feature Branch**: `20211017-dp-scheduling-docs-expansion`  
**Created**: 2026-04-02  
**Status**: Draft  
**Input**: User description: "Przeanalizuj ten plik dokumentacji wraz z plikami które on includuje i rozszerz swoją dokumentację aby obejmowała planowane funkcjonalności."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Complete DP Scheduling Reference for Firmware Developers (Priority: P1)

A firmware developer working on a new audio processing module needs to understand how DP scheduling works end-to-end — from task creation via IPC/topology, through deadline calculation, to runtime behavior. Currently, the documentation covers the theoretical EDF algorithm and deadline formulas with examples, but lacks integration context: how IPC commands create DP tasks, what component developers must declare (LPT, IBS, OBS), and how the scheduler interacts with Zephyr RTOS threads. The developer should be able to read the expanded documentation and implement a correctly configured DP module without needing to reverse-engineer source code.

**Why this priority**: This is the core value of the documentation expansion — making DP scheduling self-contained and actionable for the primary audience (firmware developers). Without this, developers must read source code to fill in gaps, which is error-prone and time-consuming.

**Independent Test**: A reviewer who has not worked on the DP scheduler should be able to read the documentation and correctly describe the full lifecycle of a DP task from creation to teardown, including all required parameters and their impact on scheduling.

**Acceptance Scenarios**:

1. **Given** the expanded documentation, **When** a developer reads the DP task lifecycle section, **Then** they can identify all mandatory parameters (LPT, IBS, OBS, period) and how each affects deadline calculations.
2. **Given** the expanded documentation, **When** a developer looks up how IPC commands map to DP task creation, **Then** they find a clear mapping from IPC4 topology commands to scheduler task parameters.
3. **Given** the expanded documentation, **When** a developer reads the component API section, **Then** they understand the `is_ready_to_process()` optional callback, buffer readiness criteria, and data release protocol.

---

### User Story 2 - Document WIP Scheduling Features (TwB, Idle Tasks, Fast Mode) (Priority: P2)

A system architect evaluating SOF scheduling capabilities needs to understand the full scheduling model including planned features currently marked as "work in progress": Tasks with Budget (TwB), Idle Priority Tasks, and Fast Mode. The current documentation references these features in `mpp_scheduling.rst` but provides incomplete or placeholder descriptions. The expanded documentation should clearly separate implemented features from planned features, describe the intended behavior of WIP features, and outline their interaction with existing DP scheduling.

**Why this priority**: The WIP features (TwB, Idle, Fast Mode) are referenced across multiple documents but lack sufficient detail to be understood or implemented. Documenting them prevents knowledge loss and enables future contributors to pick up the work.

**Independent Test**: A reader can distinguish which scheduling features are fully implemented vs. planned, and for each planned feature, understands the intended behavior, priority level, and interaction with DP tasks.

**Acceptance Scenarios**:

1. **Given** the expanded documentation, **When** a reader looks for TwB (Tasks with Budget), **Then** they find a description of budget allocation, renewal mechanism, priority behavior when budget is exhausted, and relationship to DP task preemption.
2. **Given** the expanded documentation, **When** a reader looks for Idle Priority Tasks, **Then** they find a description of when idle tasks execute, their relationship to Fast Mode, and their priority relative to DP and TwB.
3. **Given** the expanded documentation, **When** a reader looks for Fast Mode, **Then** they find a description of what triggers it, how it interacts with the History Buffer, and how it differs from normal DP processing.

---

### User Story 3 - Advanced Scheduling Scenarios and Failure Handling (Priority: P3)

An engineer debugging a real-time audio pipeline needs documentation covering failure scenarios, multi-stage chains (3+ DP stages), dynamic reconfiguration, and CPU overload conditions. The current documentation only shows 2-stage pipeline examples and does not address what happens when deadlines are missed, when pipelines are reconfigured during operation, or when CPU load exceeds capacity. The expanded documentation should provide guidance for diagnosing and handling these situations.

**Why this priority**: While the happy-path documentation (P1) and WIP features (P2) serve the majority of use cases, failure handling and advanced scenarios are essential for production debugging and system robustness.

**Independent Test**: An engineer reading the failure handling sections can describe the expected system behavior when a DP module misses its deadline, and can identify the tracing/logging points to diagnose the issue.

**Acceptance Scenarios**:

1. **Given** the expanded documentation, **When** a developer encounters a deadline miss, **Then** they find documented behavior: what the scheduler does, how to detect it via traces, and recovery strategies.
2. **Given** the expanded documentation, **When** an engineer needs to understand a 3+ stage DP chain, **Then** they find at least one worked example showing deadline propagation through more than 2 stages.
3. **Given** the expanded documentation, **When** a pipeline is dynamically reconfigured (modules added/removed), **Then** the documentation describes the impact on ongoing deadline calculations and task lifecycle.

---

### User Story 4 - Cross-Service Integration Documentation (Priority: P4)

A developer implementing a cross-core pipeline needs to understand how DP scheduling integrates with the Asynchronous Messaging Service (AMS), Library Manager (loadable modules), and Pipeline/Component Management IPC. The current documents reference these services with TODO links but never explain the integration points. The expanded documentation should describe how AMS events can trigger DP readiness, how dynamically loaded modules declare their scheduling parameters, and how pipeline management commands affect the scheduler.

**Why this priority**: Integration documentation prevents architectural misunderstandings. Currently, the TODO links create dead-ends that force developers to ask maintainers or read code.

**Independent Test**: All previously unresolved TODO link references (e.g., "Add link to Asynchronous Messaging Service detailed description") are replaced with actual content or cross-references, and the integration points are described.

**Acceptance Scenarios**:

1. **Given** the expanded documentation, **When** a developer looks for how AMS triggers DP task readiness, **Then** they find a description of the event flow from AMS notification to scheduler re-evaluation.
2. **Given** the expanded documentation, **When** a developer loads a module via Library Manager, **Then** the documentation explains how the loaded module's DP parameters (LPT, IBS, OBS) are registered with the scheduler.

---

### Edge Cases

- What happens when a DP module's declared LPT is significantly shorter than actual processing time (repeated deadline misses)?
- How does the scheduler handle a DP module that never becomes ready (stuck input buffer)?
- What happens when a pipeline has a circular dependency in deadline calculation chains?
- How are non-integer sample rates (e.g., 44.1 kHz) handled in deadline arithmetic to avoid cumulative rounding errors?
- What happens when buf2 data is released but the consumer has already been preempted mid-processing?
- How does pipeline teardown interact with a DP module currently in mid-processing (End of Stream handling)?

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Documentation MUST describe the complete DP task lifecycle from creation (via IPC/topology) through scheduling, execution, and teardown, including all required task parameters.
- **FR-002**: Documentation MUST formally specify the component developer API for DP modules, including `is_ready_to_process()`, buffer readiness criteria, and data release protocol.
- **FR-003**: Documentation MUST describe Tasks with Budget (TwB) scheduling — budget allocation, renewal, priority demotion when budget exhausted, and interaction with DP task preemption — clearly marked as planned/WIP.
- **FR-004**: Documentation MUST describe Idle Priority Tasks — execution conditions, relationship to Fast Mode, priority relative to DP and TwB — clearly marked as planned/WIP.
- **FR-005**: Documentation MUST describe Fast Mode — triggers, History Buffer interaction, differences from normal DP processing, and implementation status.
- **FR-006**: Documentation MUST include at least one worked example with a 3+ stage DP pipeline, showing deadline propagation across all stages.
- **FR-007**: Documentation MUST describe failure scenarios: deadline miss behavior, CPU overload handling, buffer overflow/underrun, and recovery strategies.
- **FR-008**: Documentation MUST describe dynamic pipeline reconfiguration impact on DP scheduling (module addition/removal during operation).
- **FR-009**: Documentation MUST resolve all existing TODO placeholders in `mpp_scheduling.rst` and `dp_scheduling.rst` by providing actual content or valid cross-references.
- **FR-010**: Documentation MUST describe the integration between AMS events and DP task readiness triggers.
- **FR-011**: Documentation MUST describe how dynamically loaded modules (via Library Manager) register their DP scheduling parameters.
- **FR-012**: Documentation MUST include guidance on handling non-integer sample rates (e.g., 44.1 kHz) in deadline calculations, including rounding rules and precision considerations.
- **FR-013**: Documentation MUST describe the Delayed Start mechanism formally: entry criteria, exit criteria, data hold duration formula, and interaction with EDF scheduling.
- **FR-014**: Documentation MUST describe the watchdog timer mechanism for DP tasks: enable/disable conditions, reset frequency, and failure recovery flows.
- **FR-015**: Documentation MUST include PlantUML diagrams for any new examples (3+ stage chains, failure scenarios, startup with delayed start state machine).
- **FR-016**: Each new or modified RST file MUST build without Sphinx/Doxygen warnings or errors.

### Key Entities

- **DP Task**: A preemptible processing unit with parameters (LPT, IBS, OBS, period, deadline). Scheduled by EDF algorithm. Belongs to a pipeline, runs on a specific core.
- **Buffer**: Data queue between modules. Has properties: current fill level, LFT (Latest Feeding Time). Connects a producer module to a consumer module.
- **Pipeline**: An ordered chain of modules (LL and DP) forming an audio processing path. Has lifecycle states (startup, running, stopping).
- **Deadline Calculation Chain**: An independent subgraph of DP modules connected by buffers, terminating at an LL module. Multiple chains can exist in a complex topology.
- **Tasks with Budget (TwB)** *(planned)*: Medium-priority tasks with pre-allocated CPU cycle budget per system tick. Budget renewal mechanism and priority drop behavior when exhausted.
- **Fast Mode** *(planned)*: A processing mode faster than real-time, triggered by components like History Buffer for Wake-on-Voice scenarios.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: 100% of TODO placeholders in existing `mpp_layer/*.rst` files are resolved (replaced with content or cross-references).
- **SC-002**: A firmware developer unfamiliar with the DP scheduler can read the documentation and correctly answer at least 90% of questions about task lifecycle, parameter configuration, and deadline calculation without consulting source code (validated via peer review).
- **SC-003**: All WIP features (TwB, Idle Tasks, Fast Mode) have dedicated documentation sections with clearly labeled implementation status.
- **SC-004**: At least one new worked example covers a 3+ stage DP pipeline (compared to current maximum of 2 stages).
- **SC-005**: Documentation builds cleanly with zero new Sphinx/Doxygen warnings or errors.
- **SC-006**: All cross-service integration points (AMS, Library Manager, Pipeline Management IPC) are documented with at least a summary and data flow description.

## Assumptions

- The existing dp_scheduling.rst mathematical formulas (LFT, deadline, LST, LPT) are correct and should be preserved as-is; the expansion adds context and examples around them, not corrections.
- The target audience is firmware developers and system architects already familiar with real-time scheduling concepts and SOF architecture at a high level.
- The PlantUML diagram format used in existing examples is the standard for new diagrams.
- WIP features (TwB, Idle Tasks, Fast Mode) will be documented based on their current design intent as described in `mpp_scheduling.rst`, not on implementation code (since they may not be fully implemented).
- The documentation expansion stays within the `mpp_layer/` directory structure and follows the existing RST (reStructuredText) format.
- IPC4 is the primary interface version for documenting DP task creation; IPC3 differences are out of scope unless explicitly noted.
- The existing 28 PlantUML diagram files do not need modification; new diagrams will be added as new files.
