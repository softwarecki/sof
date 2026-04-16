# Feature Specification: Sink/Source API Extension for Phase 1 Migration

**Feature Branch**: `20211020-sink-source-api-extension`  
**Created**: 2026-04-16  
**Status**: Draft  
**Input**: User description: "sink/source api extension. Zaplanuj implementacje pierwszej fazy polegającej na rozszerzeniu sink/source API o brakujące funkcjonalności. Przeanalizuj istniejące moduły, znajdź te które z nich wymagają przeróbki aby korzystały wyłącznie z sink/source api. Określ jakich funckji brakuje w sink/surce api."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Add the Missing Module-Facing Sink/Source Capabilities (Priority: P1)

A firmware developer migrating legacy SOF modules to the sink/source processing model needs the sink/source API to cover the remaining circular-buffer operations that legacy modules still perform through `audio_stream` helpers. The new API surface must let modules process wrapped fragments, compute how much data or free space exists before a wrap boundary, and perform the limited backward inspection still required by modules such as `volume` and `asrc`, without exposing `struct audio_stream` or `struct comp_buffer` in module-facing code.

**Why this priority**: This is the first blocking step in the wider Pipeline 2.0 roadmap. Without these missing capabilities, a large part of the runtime module set cannot be migrated away from legacy buffer access.

**Independent Test**: Verify that a representative simple module and a representative routing module can be expressed using only sink/source handles plus the new helper layer, without direct `audio_stream_*` or `comp_buffer` access in their processing logic.

**Acceptance Scenarios**:

1. **Given** a module that receives a wrapped source fragment, **When** it needs to process only the contiguous part before wrap, **Then** the sink/source extension provides enough information to do so without direct `audio_stream` access.
2. **Given** a module that scans backward inside an acquired fragment, **When** it performs the scan, **Then** it can do so through the sink/source extension without reintroducing `audio_stream_rewind*()` into module-facing code.
3. **Given** a module using only sink/source handles, **When** it processes a circular fragment, **Then** it does not need to inspect `comp_buffer->stream` directly.

---

### User Story 2 - Produce a Complete Legacy Module Inventory and Migration Map (Priority: P1)

A maintainer preparing the Phase 1 migration needs a precise inventory of all runtime modules that still depend on `.process_audio_stream` or `.process_raw_data`, together with a classification of which modules are blocked by true sink/source API gaps and which only require refactoring to existing sink/source getters and buffer acquisition calls.

**Why this priority**: The first implementation phase must avoid spending time on false blockers. The migration order depends on separating real API work from straightforward code conversion.

**Independent Test**: Audit the runtime modules in `src/audio/` and confirm that each legacy module is assigned to exactly one migration cluster with a documented blocker profile.

**Acceptance Scenarios**:

1. **Given** the runtime module tree, **When** the inventory is complete, **Then** all modules still using `.process_audio_stream` or `.process_raw_data` are listed with file paths and migration clusters.
2. **Given** a raw-data codec module, **When** its prepare-path dependencies are analyzed, **Then** the analysis distinguishes existing sink/source getters from real API gaps.
3. **Given** a routing or timing-sensitive module, **When** its migration blockers are documented, **Then** the blocker description references the exact legacy access patterns it depends on today.

---

### User Story 3 - Keep the First Phase Focused and Forward-Compatible (Priority: P2)

A system architect wants the first sink/source API extension phase to enable future work without absorbing later roadmap items such as direct binding, buffer factory behavior, or full copier redesign. The phase should add only the missing sink/source capabilities that unblock migration and one low-cost timing symmetry needed later by DP-to-DP scheduling.

**Why this priority**: Keeping this phase small and explicit reduces architectural churn and makes later P2/P4 work easier to stage.

**Independent Test**: Review the resulting plan and contracts and confirm that they cover the identified API gaps while explicitly excluding direct bind, shared-buffer policy, and copier internal-storage exposure.

**Acceptance Scenarios**:

1. **Given** the first-phase scope, **When** the plan is reviewed, **Then** direct bind and buffer-factory work remain out of scope.
2. **Given** future DP-to-DP scheduling work, **When** it needs timing symmetry from the source side, **Then** the first phase already provides a source-side LFT accessor or equivalent contract.
3. **Given** a legacy module that only needs metadata getters, **When** it is analyzed, **Then** the feature does not invent a new API where the existing sink/source API is already sufficient.

### Edge Cases

- How should backward inspection behave when the current read pointer is already at the logical start of the acquired fragment and the fragment wraps in the underlying circular buffer?
- How should helper functions report wrap-boundary sizes for frame formats whose sample/container size differs from valid bit depth?
- How should a routing module compute the safe processing chunk when one source wraps earlier than the others?
- How should timing-sensitive modules behave if they need source-side LFT information but the source implementation does not yet provide it?
- How should raw-data codecs access stream metadata during prepare without reaching through `comp_buffer->stream`?

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The feature MUST inventory all runtime modules in `src/audio/` that still use `.process_audio_stream` or `.process_raw_data`.
- **FR-002**: The feature MUST classify each inventoried module as either `refactor-only` or `blocked-by-missing-sink/source-capability`.
- **FR-003**: The module-facing sink/source interface MUST provide a way to compute contiguous bytes, frames, or samples available before wrap for an acquired source or sink fragment.
- **FR-004**: The module-facing sink/source interface MUST provide a way to wrap a pointer within an acquired source or sink fragment without exposing `struct audio_stream`.
- **FR-005**: The module-facing sink/source interface MUST provide a way to support the limited backward inspection required by legacy modules such as `volume` and `asrc`.
- **FR-006**: The source-side API MUST provide a latest-feeding-time query symmetric with the sink-side timing accessor used by DP scheduling.
- **FR-007**: The feature MUST reuse existing sink/source metadata getters for frame format, valid format, rate, channels, and buffer format wherever they already cover legacy prepare-path needs.
- **FR-008**: The feature MUST identify already migrated reference modules that can serve as implementation patterns for later Phase 1 conversion work.
- **FR-009**: The feature MUST NOT expose `struct audio_stream` or `struct comp_buffer` as part of the new module-facing API surface.
- **FR-010**: The feature MUST NOT include direct bind, buffer-factory policy, or copier internal-storage exposure; those remain in later roadmap phases.
- **FR-011**: The feature MUST comply with the repository constitution: plain C only, no new dependencies, and no new or modified test files.

### Key Entities *(include if feature involves data)*

- **Legacy Module**: A runtime module still using `.process_audio_stream` or `.process_raw_data`, together with its current legacy buffer dependencies and migration complexity.
- **Source Fragment**: The tuple returned by source acquisition, consisting of data pointer, circular-buffer start pointer, and buffer size, used by a module for read access.
- **Sink Fragment**: The tuple returned by sink acquisition, consisting of writable pointer, circular-buffer start pointer, and buffer size, used by a module for write access.
- **Fragment Helper**: A sink/source-side helper that performs wrap-boundary or reverse-span calculations on an acquired fragment without exposing `audio_stream` internals.
- **Capability Gap**: A true missing sink/source-side function or helper that blocks migration of at least one legacy module.
- **Migration Cluster**: A group of legacy modules that share similar API needs and can be migrated in the same sequence.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: The feature documents a complete inventory of the `25` runtime legacy modules currently using `.process_audio_stream` or `.process_raw_data`.
- **SC-002**: The resulting design reduces the list of true sink/source API gaps to an explicit minimal set and rejects false gaps already covered by existing getters.
- **SC-003**: The design demonstrates that raw-data codec modules can obtain required metadata through existing sink/source getters without new buffer-format accessors.
- **SC-004**: The design defines a fragment-helper contract that covers the wrap-boundary operations needed by routing, DSP, and timing-sensitive modules.
- **SC-005**: The design includes a source-side timing accessor for future DP-to-DP scheduling without introducing unrelated P2/P4 implementation work.
- **SC-006**: The first-phase plan leaves direct bind, buffer factory, and copier internal-storage redesign explicitly out of scope.

## Assumptions

- The scope of this feature is the first enablement phase of the larger Pipeline 2.0 roadmap, not the full migration of every legacy module.
- Runtime modules under `src/audio/` are in scope; test code under `test/` is not.
- Existing sink/source getters for channels, rate, frame format, valid format, and buffer format are already sufficient for most prepare-path metadata access.
- The main missing capability is not a new transport model but a helper layer for circular-fragment navigation plus source-side timing symmetry.
- `copier` and `asrc` are analyzed in this phase, but their most invasive redesign work remains later than the basic API extension.
- The implementation must remain in plain C and must not add external dependencies or new test files.