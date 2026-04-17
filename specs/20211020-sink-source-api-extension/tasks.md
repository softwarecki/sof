# Tasks: Sink/Source API Extension for Phase 1 Migration

**Input**: Design documents from `/specs/20211020-sink-source-api-extension/`
**Prerequisites**: `plan.md` (required), `spec.md` (required for user stories), `research.md`, `data-model.md`, `contracts/`, `quickstart.md`

**Tests**: No new test-file tasks are included. Validation for this feature uses existing build and audit workflows only, per the feature scope and repository constitution.

**Organization**: Tasks are grouped by user story to keep the API extension, the legacy-module inventory, and the scope-guard work independently deliverable.

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Freeze the implementation scope and validation path before touching the public API

- [X] T001 [P] Lock helper naming, pilot-module choices, and sequencing assumptions in specs/20211020-sink-source-api-extension/plan.md and specs/20211020-sink-source-api-extension/quickstart.md
- [X] T002 [P] Freeze the canonical inventory structure and migration-cluster vocabulary in specs/20211020-sink-source-api-extension/research.md and specs/20211020-sink-source-api-extension/data-model.md

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Add the shared contract and plumbing that every user story depends on

**⚠️ CRITICAL**: No user story implementation should start before this phase is complete

- [X] T003 [P] Add fragment descriptor types and helper prototypes to src/include/module/audio/source_api.h and src/include/module/audio/sink_api.h
- [X] T004 [P] Add SOF-side wrapper declarations for fragment helpers and source-side LFT access in src/include/sof/audio/source_api.h, src/include/sof/audio/sink_api.h, and src/include/sof/audio/sink_source_utils.h
- [X] T005 [P] Add source-side `get_lft` operation plumbing in src/audio/source_api_helper.c, src/audio/buffers/comp_buffer.c, and src/audio/buffers/ring_buffer.c
- [X] T006 Add Doxygen comments and compatibility notes for the new helper/accessor surface in src/include/module/audio/source_api.h, src/include/module/audio/sink_api.h, and src/include/sof/audio/sink_source_utils.h

**Checkpoint**: Shared API contract and provider plumbing are in place; user story work can now proceed

---

## Phase 3: User Story 1 - Add the Missing Module-Facing Sink/Source Capabilities (Priority: P1) 🎯 MVP

**Goal**: Deliver the missing fragment-navigation and source-side timing capabilities that unblock Phase 1 migration work

**Independent Test**: Verify that one simple DSP module and one routing module can be expressed using only sink/source handles plus the new helper layer, without direct `audio_stream_*` or `comp_buffer` access in their processing logic

### Implementation for User Story 1

- [X] T007 [US1] Implement generic fragment wrap and reverse-span helpers in src/audio/sink_source_utils.c
- [X] T008 [US1] Implement frame-distance and sample-distance helper wrappers in src/audio/sink_source_utils.c and src/include/sof/audio/sink_source_utils.h
- [X] T009 [P] [US1] Replace prepare-time `comp_buffer->stream` metadata access with existing sink/source getters in src/audio/codec/dts/dts.c, src/audio/module_adapter/module/cadence_ipc3.c, src/audio/nxp/eap.c, src/audio/module_adapter/module/waves/waves.c, and src/audio/module_adapter/module/passthrough.c
- [X] T010 [P] [US1] Convert the simple DSP pilot from `.process_audio_stream` to `.process` in src/audio/dcblock/dcblock.c using the new fragment helpers and sink/source handles
- [X] T011 [P] [US1] Convert the routing pilot from `.process_audio_stream` to `.process` in src/audio/mixer/mixer.c and src/audio/mixer/mixer_generic.c using acquired source and sink fragments
- [X] T012 [US1] Validate the helper contract against reverse-scan users in src/audio/volume/volume.c and src/audio/asrc/asrc.c, then adjust src/audio/sink_source_utils.c and src/include/sof/audio/sink_source_utils.h if any backward-inspection gap remains

**Checkpoint**: The helper layer is usable by real modules, and the first migration pilots prove the new contract works in both simple and routing paths

---

## Phase 4: User Story 2 - Produce a Complete Legacy Module Inventory and Migration Map (Priority: P1)

**Goal**: Deliver a reliable, file-backed inventory of all legacy runtime modules and clearly separate true API blockers from refactor-only work

**Independent Test**: Audit the runtime module tree and confirm that every module still using `.process_audio_stream` or `.process_raw_data` appears exactly once in the documented clusters, with a blocker classification tied to the final helper contract

### Implementation for User Story 2

- [X] T013 [US2] Build the canonical legacy-module inventory and migration clusters in specs/20211020-sink-source-api-extension/research.md
- [X] T014 [P] [US2] Record capability-gap, fragment-helper, and cluster relationships in specs/20211020-sink-source-api-extension/data-model.md
- [X] T015 [P] [US2] Add reference-module mapping and pilot-module rationale in specs/20211020-sink-source-api-extension/quickstart.md
- [X] T016 [US2] Capture refactor-only versus API-blocked classification and next-migration order in specs/20211020-sink-source-api-extension/plan.md and specs/20211020-sink-source-api-extension/research.md

**Checkpoint**: The migration backlog is explicit, auditable, and ready to drive the next conversion phases without rediscovering blockers

---

## Phase 5: User Story 3 - Keep the First Phase Focused and Forward-Compatible (Priority: P2)

**Goal**: Lock the boundary of this first phase so it stays compatible with later roadmap work without absorbing direct bind, buffer-factory, or copier-redesign scope

**Independent Test**: Review the final contract and plan and confirm they include the needed helper and timing additions while explicitly deferring direct bind, buffer-factory policy, and copier internal-storage work

### Implementation for User Story 3

- [X] T017 [US3] Lock out-of-scope items and source-side LFT fallback semantics in specs/20211020-sink-source-api-extension/contracts/sink-source-api-extension.md
- [X] T018 [P] [US3] Update sequencing and dependency boundaries in specs/20211020-sink-source-api-extension/plan.md to keep direct bind, buffer factory, and copier redesign deferred behind this phase
- [X] T019 [P] [US3] Update specs/20211020-sink-source-api-extension/quickstart.md to codify the no-test-file rule, validation workflow, and deferred `asrc` and `copier` work

**Checkpoint**: The first-phase scope is fixed, forward-compatible, and protected from roadmap leakage

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Align public documentation, run the agreed validation path, and leave a clean handoff state

- [X] T020 [P] Refresh public API wording and contract alignment in src/include/sof/audio/source_api.h, src/include/sof/audio/sink_api.h, and specs/20211020-sink-source-api-extension/contracts/sink-source-api-extension.md
- [ ] T021 [P] Run the validation workflow described in specs/20211020-sink-source-api-extension/quickstart.md and record any deltas in specs/20211020-sink-source-api-extension/research.md
- [X] T022 Clean up compatibility notes and remaining migration guidance in src/audio/source_api_helper.c, src/audio/sink_api_helper.c, and specs/20211020-sink-source-api-extension/plan.md

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies; can start immediately
- **Foundational (Phase 2)**: Depends on Setup completion; blocks all user stories
- **User Story 1 (Phase 3)**: Depends on Foundational completion
- **User Story 2 (Phase 4)**: Depends on Foundational completion and should use the finalized helper contract from Phase 2 as the blocker baseline
- **User Story 3 (Phase 5)**: Depends on Foundational completion and should incorporate the resulting US1 and US2 boundaries
- **Polish (Phase 6)**: Depends on all desired user stories being complete

### User Story Dependencies

- **User Story 1 (P1)**: Can start after Foundational; no dependency on other user stories
- **User Story 2 (P1)**: Can start after Foundational; independent from US1 implementation, but should reflect the same finalized helper contract
- **User Story 3 (P2)**: Can start after Foundational, but it is safest to finish after US1 and US2 because it locks the final scope boundary

### Within Each User Story

- Shared helper and accessor declarations must exist before any pilot conversion begins
- Raw-data metadata cleanup should happen before using those modules as evidence that no extra getter API is required
- Pilot conversions should finish before reverse-scan validation is closed for `volume` and `asrc`
- Inventory and migration-map documents should be updated only after the blocker set is frozen

### Parallel Opportunities

- **Setup**: T001 and T002 can run in parallel
- **Foundational**: T003, T004, and T005 can run in parallel once setup is frozen; T006 follows them
- **US1**: T009, T010, and T011 can run in parallel after T007 and T008; T012 follows as validation
- **US2**: T014 and T015 can run in parallel after T013; T016 consolidates the results
- **US3**: T018 and T019 can run in parallel after T017
- **Polish**: T020 and T021 can run in parallel; T022 closes remaining cleanup

---

## Parallel Example: User Story 1

```bash
# After the shared helper layer lands:
Task: "Replace prepare-time `comp_buffer->stream` metadata access with existing sink/source getters in src/audio/codec/dts/dts.c, src/audio/module_adapter/module/cadence_ipc3.c, src/audio/nxp/eap.c, src/audio/module_adapter/module/waves/waves.c, and src/audio/module_adapter/module/passthrough.c"
Task: "Convert the simple DSP pilot from `.process_audio_stream` to `.process` in src/audio/dcblock/dcblock.c using the new fragment helpers and sink/source handles"
Task: "Convert the routing pilot from `.process_audio_stream` to `.process` in src/audio/mixer/mixer.c and src/audio/mixer/mixer_generic.c using acquired source and sink fragments"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup
2. Complete Phase 2: Foundational
3. Complete Phase 3: User Story 1
4. **STOP and VALIDATE**: Confirm the fragment-helper layer and source-side LFT support are enough for one simple DSP pilot and one routing pilot

### Incremental Delivery

1. Land Setup + Foundational to freeze the API contract
2. Land User Story 1 to prove the helper layer on real modules
3. Land User Story 2 to freeze the migration backlog and blocker map
4. Land User Story 3 to lock the first-phase boundary before broader Phase 1 migration work starts
5. Finish with Polish to leave a validated handoff state

### Single-Engineer Strategy

With one engineer, the recommended order is:

1. Phase 1 → Phase 2
2. User Story 1 first, because it contains the actual shared API and pilot code work
3. User Story 2 next, using the finalized contract and pilot findings
4. User Story 3 after the prior two stories, to lock the phase boundary based on real implementation results
5. Phase 6 polish at the end

---

## Notes

- `[P]` tasks touch different files and can be parallelized safely
- Story labels map each task directly to a user story in spec.md
- No new test-file tasks are listed because the feature and constitution explicitly keep new tests out of scope
- Validation is still required through existing build and audit workflows referenced in quickstart.md
- Avoid introducing direct bind, buffer-factory, or copier-internal-storage work into this task set