# Implementation Plan: Single-Engineer Variant

## Purpose

This document turns the feature scope from [spec.md](spec.md) into a delivery plan optimized for one engineer. It assumes the planning scale agreed for this roadmap:

- `8 points = 1 week` of work for one developer
- the developer is working without AI assistance
- the plan is executed mostly sequentially, with only limited design overlap

This plan also carries forward two planning decisions made while breaking down the roadmap:

- P1 includes all runtime modules that still use `process_audio_stream` or `process_raw_data`
- the flat pipeline list and `pipeline_module` work from [../20211019-flat-pipeline-module-struct/spec.md](../20211019-flat-pipeline-module-struct/spec.md) is treated as a dedicated milestone before P2 and P4

## Single-Engineer Delivery Strategy

For one engineer, the recommended approach is depth-first rather than parallel:

1. Lock down assumptions and test scaffolding first.
2. Deliver the flat execution list and `pipeline_module` as the foundation milestone.
3. Complete the sink/source migration before attempting broad bind/factory refactoring.
4. Start direct binding only after the migration foundation is stable.
5. Start DP-to-DP scheduling only after flat list iteration and bind/factory behavior are in place.
6. Leave scheduler diagnostics and final hardening near the end, when the behavior is stable enough to instrument.

This sequencing reduces context switching and avoids rework in a single-person execution model.

## Summary Estimate

| Phase | Scope | Points | Approx. Weeks | Cumulative Weeks |
| --- | --- | ---: | ---: | ---: |
| 0 | Assumptions and baseline verification | 6 | 0.75 | 0.75 |
| 1 | Flat execution list and `pipeline_module` | 18 | 2.25 | 3.00 |
| 2 | Full sink/source migration (P1) | 60 | 7.50 | 10.50 |
| 3 | Direct bind and buffer factory (P2) | 24 | 3.00 | 13.50 |
| 4 | DP-to-DP scheduling and deadline propagation (P4) | 28 | 3.50 | 17.00 |
| 5 | DP scheduler diagnostics (P5) | 10 | 1.25 | 18.25 |
| 6 | Final hardening, documentation, release gate | 6 | 0.75 | 19.00 |

**Base estimate:** `152 points`, or about `19 weeks` for one engineer.

**Recommended management reserve:** add `2 to 3 extra weeks` for unknowns around `copier`, `cadence_ipc3`, cross-core coherence, and full DP-chain validation on real hardware.

## Suggested Timeline For One Engineer

This is the simplest execution shape for a single owner:

- Weeks 1 to 1.0: Phase 0
- Weeks 1 to 3: Phase 1
- Weeks 4 to 11: Phase 2
- Weeks 12 to 14: Phase 3
- Weeks 15 to 17: Phase 4
- Weeks 18 to 18.25: Phase 5
- Weeks 18.25 to 19: Phase 6

In practice, the engineer may do small forward-looking design spikes during late Phase 2, but the main work should remain sequential.

## Detailed Work Breakdown

### Phase 0. Assumptions and Baseline Verification - 6 Points

Goal: remove planning ambiguity and prepare a stable regression baseline before any architectural change.

| ID | Task | Points | Dependencies | Deliverable |
| --- | --- | ---: | --- | --- |
| 0.1 | Inventory runtime modules still using `process_audio_stream`, `process_raw_data`, `comp_buffer`, or `audio_stream`, and group them into migration clusters. | 2 | None | A complete migration inventory and a list of missing sink/source capabilities. |
| 0.2 | Close the remaining design decisions left in edge cases: invalid topology, fan-out and fan-in ordering, EDF tie-break, trace overflow behavior, unbind while a consumer is active, and 44.1 kHz arithmetic rules. | 2 | 0.1 | A fixed decision matrix for implementation and tests. |
| 0.3 | Prepare the verification baseline: static scans, cmocka pipeline and module-adapter tests, minimal LL-LL / LL-DP / DP-LL topologies, and a seed DP1 -> DP2 -> DP3 -> LL test shape. | 2 | 0.1 | A reusable baseline for regression checks before and after refactors. |

### Phase 1. Foundation Milestone: Flat Execution List and `pipeline_module` - 18 Points

Goal: replace recursive runtime traversal with a flat module list and establish the kernel-side `pipeline_module` structure.

| ID | Task | Points | Dependencies | Deliverable |
| --- | --- | ---: | --- | --- |
| 1.1 | Introduce `pipeline_module` as a kernel-only structure and tie its lifecycle to `processing_module` creation and teardown. | 4 | 0.x | A minimal kernel wrapper ready to hold pipeline and scheduling metadata. |
| 1.2 | Build and maintain a flat execution list for IPC4 during bind, rebind, unbind, and pipeline completion paths. | 5 | 1.1 | Each pipeline owns a deterministic ordered module list. |
| 1.3 | Preserve IPC3 compatibility by letting the recursive walker build the same flat list while runtime starts using the list representation. | 2 | 1.2 | A single runtime representation for both IPC generations. |
| 1.4 | Switch IPC4 runtime paths from `pipeline_for_each_comp()` to flat-list iteration for copy, trigger, prepare, params, reset, and xrun-related flows. | 5 | 1.2 | No recursive graph walk remains in the IPC4 hot path. |
| 1.5 | Add ordering, fan-in, fan-out, and basic stack-usage tests, and update pipeline documentation. | 2 | 1.4 | A frozen foundation milestone for later P2 and P4 work. |

### Phase 2. Full Sink/Source Migration (P1) - 60 Points

Goal: migrate all runtime modules to the sink/source API and remove the hybrid legacy buffer model.

| ID | Task | Points | Dependencies | Deliverable |
| --- | --- | ---: | --- | --- |
| 2.1 | Extend the sink/source API and simplify `module_adapter` so runtime no longer depends on `process_audio_stream` or `process_raw_data` as architectural paths. | 6 | 1.x | A complete migration path with the required API capabilities. |
| 2.2 | Migrate simple single-input/single-output DSP modules: `volume`, `dcblock`, `eq_fir`, `eq_iir`, `drc`, `multiband_drc`, `crossover`. | 10 | 2.1 | Seven basic DSP modules use only sink/source access. |
| 2.2.a | `volume` | 1.5 | 2.1 | Migrated module with unchanged behavior. |
| 2.2.b | `dcblock` | 1.0 | 2.1 | Migrated module with unchanged behavior. |
| 2.2.c | `eq_fir` | 1.5 | 2.1 | Migrated module with unchanged behavior. |
| 2.2.d | `eq_iir` | 1.5 | 2.1 | Migrated module with unchanged behavior. |
| 2.2.e | `drc` | 1.5 | 2.1 | Migrated module with unchanged behavior. |
| 2.2.f | `multiband_drc` | 1.5 | 2.1 | Migrated module with unchanged behavior. |
| 2.2.g | `crossover` | 1.5 | 2.1 | Migrated module with unchanged behavior. |
| 2.3 | Migrate routing and multi-pin modules: `mixer`, `mux`, `demux`, `selector`. | 9 | 2.1 | Multi-pin processing works only through sink/source. |
| 2.3.a | `mixer` | 2.0 | 2.1 | Migrated module with unchanged behavior. |
| 2.3.b | `mux` | 2.0 | 2.1 | Migrated module with unchanged behavior. |
| 2.3.c | `demux` | 2.0 | 2.1 | Migrated module with unchanged behavior. |
| 2.3.d | `selector` | 3.0 | 2.1 | Migrated module with unchanged behavior. |
| 2.4 | Migrate more complex and analysis-heavy modules: `tdfb`, `mfcc`, `tflm-classify`, `google_ctc_audio_processing`, `aria`, `rtnr`. | 12 | 2.1 | Advanced modules use the same sink/source abstraction without behavioral regressions. |
| 2.4.a | `tdfb` | 2.5 | 2.1 | Migrated module with unchanged behavior. |
| 2.4.b | `mfcc` | 2.0 | 2.1 | Migrated module with unchanged behavior. |
| 2.4.c | `tflm-classify` | 1.5 | 2.1 | Migrated module with unchanged behavior. |
| 2.4.d | `google_ctc_audio_processing` | 2.0 | 2.1 | Migrated module with unchanged behavior. |
| 2.4.e | `aria` | 2.0 | 2.1 | Migrated module with unchanged behavior. |
| 2.4.f | `rtnr` | 2.0 | 2.1 | Migrated module with unchanged behavior. |
| 2.5 | Migrate timing and transport-sensitive modules: `asrc`, and `copier` to pure sink/source usage without direct bind yet. | 9 | 2.1 | The most timing-sensitive runtime paths become P2-ready. |
| 2.5.a | `asrc` | 3.0 | 2.1 | Migrated module with unchanged behavior. |
| 2.5.b | `copier` | 6.0 | 2.1 | Migrated module prepared for internal-storage exposure in P2. |
| 2.6 | Migrate `process_raw_data` modules: `dts`, `cadence_ipc3`, `passthrough`, `nxp_eap`, `waves`. | 8 | 2.1 | Runtime processing has one consistent input/output model. |
| 2.6.a | `dts` | 1.5 | 2.1 | Migrated module with unchanged behavior. |
| 2.6.b | `cadence_ipc3` | 2.5 | 2.1 | Migrated module with unchanged behavior. |
| 2.6.c | `passthrough` | 1.0 | 2.1 | Migrated module with unchanged behavior. |
| 2.6.d | `nxp_eap` | 1.5 | 2.1 | Migrated module with unchanged behavior. |
| 2.6.e | `waves` | 1.5 | 2.1 | Migrated module with unchanged behavior. |
| 2.7 | Remove `audio_buffer_attach_secondary_buffer()`, `audio_buffer_sync_secondary_buffer()`, and the remaining hybrid-buffer transition logic once migration is complete. | 5 | 2.2 to 2.6 | Double buffering and temporary sync paths are removed. |
| 2.8 | Add static scans, bit-parity tests, and LL-LL / LL-DP / DP-LL regression coverage after legacy buffer access is removed. | 6 | 2.7 | P1 is formally closed with regression evidence. |

### Phase 3. Direct Module-To-Module Bind and Buffer Factory (P2) - 24 Points

Goal: change bind semantics from module-to-buffer to module-to-module, with the buffer becoming an internal factory decision.

| ID | Task | Points | Dependencies | Deliverable |
| --- | --- | ---: | --- | --- |
| 3.1 | Define the module-to-module bind contract and the buffer-factory API, including the split between direct bind and factory-created intermediate buffers. | 4 | 1.x, 2.1 | A stable architectural contract for binding. |
| 3.2 | Refactor IPC4 bind handling (`ipc_comp_connect`) to the new module-to-module model. | 6 | 3.1 | IPC4 bind is no longer architecturally buffer-centric. |
| 3.3 | Add IPC3 compatibility or a shared wrapper around the factory and flat-list rebuild hooks. | 3 | 3.1 | One bind model without duplicating behavior. |
| 3.4 | Expose `copier` internal storage through sink/source and support direct binding without an intermediate buffer. | 4 | 2.5.b, 3.2 | Direct bind works for `copier`. |
| 3.5 | Expose `mixin_mixout` internal storage and integrate it with direct bind behavior. | 3 | 3.2 | Direct bind works for `mixin_mixout`. |
| 3.6 | Implement buffer-factory policies and safety behavior: same-core DP-DP uses cached ring buffer, cross-core uses shared non-cached buffer, LL-DP uses ring buffer, invalid topology is rejected, and unbind safety is enforced. | 4 | 3.2 to 3.5 | P2 closes with memory, topology, and safety coverage. |

### Phase 4. DP-To-DP Scheduling and Deadline Propagation (P4) - 28 Points

Goal: enable DP-to-DP chains with correct cross-core state visibility, deadline propagation, and delayed-start behavior.

| ID | Task | Points | Dependencies | Deliverable |
| --- | --- | ---: | --- | --- |
| 4.1 | Move or mirror the required scheduling fields into coherent cross-core memory: period, LPT, readiness, `dp_startup_delay`, and startup counters. Tie the new state to `pipeline_module` and DP task data. | 7 | 1.x, 3.1 | A coherent state model for cross-core DP scheduling. |
| 4.2 | Generalize LFT and deadline calculation from DP-to-LL to full DP-to-DP-to-...-LL chains using the formulas in `mpp_layer/dp_scheduling.rst` and the flat module list. | 8 | 4.1, 3.x | `module_get_deadline()` and related paths work for complete chains. |
| 4.3 | Rewrite delayed-start behavior so it waits for the first readiness of the next DP stage, not only for the current stage's first processing completion. | 4 | 4.2 | Correct startup behavior for DP chains. |
| 4.4 | Remove the DP-to-DP hard reject in bind logic and close the `audio_buffer_sink_get_lft()` TODO, including correction-formula handling and non-integer sample-rate arithmetic. | 4 | 4.2, 4.3 | DP-to-DP becomes functionally available, including 44.1 kHz cases. |
| 4.5 | Add multi-core and integration tests: 2-stage and 3-stage DP chains, steady-state checks, underrun and deadline-miss detection, and cross-core coherence validation. | 5 | 4.4 | P4 closes with end-to-end runtime validation. |

### Phase 5. DP Scheduler Diagnostics (P5) - 10 Points

Goal: add low-overhead diagnostic visibility into DP scheduling without perturbing runtime behavior.

| ID | Task | Points | Dependencies | Deliverable |
| --- | --- | ---: | --- | --- |
| 5.1 | Define the trace data model, Kconfig controls, and overflow policy for the accumulation buffer. | 2 | 4.2 | A minimal and predictable trace design. |
| 5.2 | Instrument `scheduler_dp_recalculate()`, EDF selection, task start and finish, and deadline misses. | 3 | 5.1 | Readiness, deadlines, chosen task, and misses are captured. |
| 5.3 | Add accumulation and periodic emission of one consolidated trace line per interval. | 2 | 5.2 | Diagnostic output remains low overhead and usable. |
| 5.4 | Validate trace overhead with A/B runs and update scheduler documentation. | 3 | 5.3 | P5 closes without trace-induced deadline regressions. |

### Phase 6. Final Hardening, Documentation, and Release Gate - 6 Points

Goal: close the roadmap with documentation, cleanup, and a final end-to-end regression gate.

| ID | Task | Points | Dependencies | Deliverable |
| --- | --- | ---: | --- | --- |
| 6.1 | Update architecture and README-style documentation in the pipeline, schedule, copier, and MPP areas. | 2 | 1.x to 5.x | Documentation matches the final architecture. |
| 6.2 | Run the final regression sweep across LL-LL, LL-DP, DP-LL, DP-DP, cross-core, direct-bind, and memory-usage scenarios. | 3 | 2.x to 5.x | A release gate for the whole roadmap. |
| 6.3 | Remove remaining dead guards and transition-only code, and tidy up supporting build and test assets. | 1 | 6.2 | A clean closeout state. |

## Recommended Single-Engineer Execution Rules

The work can technically overlap in places, but for one engineer the best results will usually come from these rules:

- Do not start P2 implementation before Phase 1 is complete and Phase 2.1 has stabilized the API direction.
- Do not start P4 before the flat list is in runtime use and the bind/factory skeleton is already working.
- Finish each migration cluster with regression checks before opening the next cluster.
- Keep the final hardening phase mostly free of new feature work.
- Treat `copier`, `cadence_ipc3`, and cross-core DP validation as explicit schedule risks.

## Primary Code Areas

The single engineer owning this roadmap will spend most of the time in these files and subsystems:

- `src/include/sof/audio/pipeline.h`
- `src/audio/pipeline/pipeline-graph.c`
- `src/audio/pipeline/pipeline-stream.c`
- `src/audio/pipeline/pipeline-params.c`
- `src/ipc/ipc4/helper.c`
- `src/ipc/ipc3/helper.c`
- `src/audio/buffers/audio_buffer.c`
- `src/audio/module_adapter/module_adapter.c`
- `src/include/sof/audio/source_api.h`
- `src/include/sof/audio/sink_api.h`
- `src/include/module/module/base.h`
- `src/audio/module_adapter/module/generic.c`
- `src/schedule/zephyr_dp_schedule.c`
- `src/schedule/zephyr_dp_schedule_thread.c`
- `src/audio/copier/copier.c`
- `src/audio/mixin_mixout/mixin_mixout.c`
- `mpp_layer/dp_scheduling.rst`

## Verification Gates

Each major phase should end with a concrete gate:

1. **Foundation gate**: flat-list runtime iteration is active for IPC4 and order matches the old recursive traversal.
2. **Migration gate**: runtime modules no longer directly depend on `comp_buffer` or `audio_stream` access outside intentional test coverage.
3. **Binding gate**: `copier` and `mixin_mixout` support direct bind where expected, and the factory selects the right buffer type elsewhere.
4. **Scheduling gate**: a 3-stage DP chain (`DP1(5 ms) -> DP2(10 ms) -> DP3(20 ms) -> LL(1 ms)`) runs without deadline misses in steady state.
5. **Diagnostics gate**: trace output is emitted at the configured interval without introducing measurable new misses.
6. **Release gate**: all supported pipeline classes pass regression after cleanup.

## Out of Scope

The following items are intentionally outside this plan:

- host-side tooling beyond the firmware diagnostics already described in P5
- runtime switching of buffer type after bind time
- mass migration of every existing kernel-only field out of `processing_module` beyond what is required for the flat-list and DP-scheduling work

## Final Recommendation

For one engineer, this roadmap should be treated as a roughly `19-week` implementation effort with a strong chance of needing extra reserve for integration. The most stable delivery path is:

`Foundation -> full sink/source migration -> direct bind/factory -> DP-to-DP scheduling -> diagnostics -> hardening`

That order minimizes architectural churn and gives the engineer clear integration checkpoints instead of running multiple unstable workstreams in parallel.