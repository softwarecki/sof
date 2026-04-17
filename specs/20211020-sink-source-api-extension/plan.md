# Implementation Plan: Sink/Source API Extension for Phase 1 Migration

**Branch**: `20211020-sink-source-api-extension` | **Date**: 2026-04-16 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/specs/20211020-sink-source-api-extension/spec.md`

## Summary

The goal of this feature is to close the small set of real sink/source API gaps that still block Phase 1 module migration, while also producing a reliable inventory of all legacy runtime modules and separating true API work from refactor-only conversions. The finalized helper surface is the `audio_fragment_*` family plus thin SOF-side acquisition wrappers for `source_fragment` and `sink_fragment`, alongside a source-side latest-feeding-time accessor with an explicit `UINT32_MAX` fallback when a provider does not implement `get_lft()`.

The code pilots for this phase are `dcblock` for simple 1-in/1-out processing and `mixer` for multi-source routing. Larger architectural work such as direct bind, buffer factory, and `copier` internal-storage redesign stays out of scope.

## Technical Context

**Language/Version**: C using the repository's existing firmware toolchain configuration  
**Primary Dependencies**: Existing SOF firmware code only: `source_api.h`, `sink_api.h`, `module_adapter`, `sink_source_utils`, legacy `audio_stream` and `comp_buffer` code used as migration references only  
**Storage**: N/A for persistent storage; in-memory circular audio buffers and internal staging buffers only  
**Testing**: Existing cmocka and firmware build validation only; static inventory and grep-based audits; no new tests in scope  
**Target Platform**: SOF firmware on Zephyr for Intel DSP platforms and existing host or stub build environments
**Project Type**: Embedded firmware subsystem change  
**Performance Goals**: Preserve current runtime behavior and hot-path complexity for migrated modules; avoid adding extra copies or new buffer abstractions in this phase  
**Constraints**: Plain C only, zero new dependencies, no test-file changes, keep module-facing code free of `audio_stream` and `comp_buffer`, no direct-bind or buffer-factory work in this phase  
**Scale/Scope**: Legacy runtime modules across 5 migration clusters, with `dcblock` and `mixer` promoted to Phase 1 reference pilots

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- **Pure C, Simple and Readable**: PASS. The design keeps implementation in plain C and prefers small helper functions over new abstractions.
- **Zero New Dependencies**: PASS. All work stays inside the existing SOF codebase.
- **No New Tests**: PASS. Validation is limited to existing tests, builds, and static audits.
- **Ask, Don't Guess**: PASS for planning. The plan narrows scope to validated gaps and avoids introducing unrelated API surface.
- **Documentation Discipline**: PASS. The feature will update interface documentation in the affected headers and keep design artifacts in `specs/`.

**Post-Design Re-check**: PASS. The proposed contract remains additive, minimal, and aligned with the constitution.

## Project Structure

### Documentation (this feature)

```text
specs/20211020-sink-source-api-extension/
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
└── contracts/
    └── sink-source-api-extension.md
```

### Source Code (repository root)

```text
src/
├── include/
│   ├── module/audio/
│   │   ├── source_api.h
│   │   └── sink_api.h
│   └── sof/audio/
│       ├── source_api.h
│       └── sink_api.h
├── audio/
│   ├── sink_source_utils.c
│   ├── module_adapter/module_adapter.c
│   ├── volume/
│   ├── mixer/
│   ├── mux/
│   ├── selector/
│   ├── asrc/
│   ├── copier/
│   ├── codec/dts/
│   ├── nxp/
│   └── module_adapter/module/
│       ├── cadence_ipc3.c
│       ├── passthrough.c
│       ├── waves/
│       └── dolby/
└── schedule/
    └── zephyr_dp_schedule*.c
```

**Structure Decision**: This feature is a firmware-only interface and migration-enablement change. Documentation lives entirely under `specs/20211020-sink-source-api-extension/`. Code changes are expected only in `src/include/` and `src/audio/`, with no test-tree modifications.

## Implementation Slices

### Slice 1. Freeze the legacy inventory and blocker map

**Goal**: make the migration backlog explicit before changing headers.

**Work**:
- confirm the 20 runtime modules still using `.process_audio_stream`
- confirm the 5 runtime modules still using `.process_raw_data`
- assign each module to one migration cluster
- separate true API blockers from refactor-only work

**Expected output**:
- stable module inventory
- stable blocker classification

### Slice 2. Add the fragment-helper contract

**Goal**: cover the circular-buffer navigation operations that legacy modules still perform via `audio_stream` helpers.

**Work**:
- add helper-layer definitions for acquired source and sink fragments
- add helpers for:
  - bytes until wrap
  - frames or samples until wrap
  - pointer wrap within a fragment
  - reverse-span or backward-inspection distance
- keep the API additive and reusable by simple DSP, routing, and timing-sensitive modules

**Expected output**:
- a minimal helper layer usable without exposing `audio_stream`

### Slice 3. Add source-side LFT symmetry

**Goal**: keep the sink/source timing contract future-compatible with DP-to-DP scheduling.

**Work**:
- add source-side latest-feeding-time support to the source contract
- mirror the sink-side wrapper model closely enough for later scheduler use
- keep fallback behavior explicit for source implementations that do not provide LFT yet

**Expected output**:
- a source-side timing accessor with documented semantics

### Slice 4. Validate the design on refactor-only modules

**Goal**: prove that the first extension phase is small and that some modules can move immediately once the helper layer exists.

**Work**:
- use existing sink/source getters to replace prepare-time metadata reads in raw-data codec modules
- choose one simple DSP module as an early migration pilot
- choose one routing module as a helper-layer pilot

**Expected output**:
- confirmation that raw-data codecs do not need new core getters
- confirmation that routing modules need helper coverage, not a new reservation API

### Slice 5. Leave later-scope modules prepared but not absorbed

**Goal**: make sure this feature helps later work without expanding into later roadmap phases.

**Work**:
- document `asrc` as helper-layer dependent and later-migration ready
- document `copier` as analyzed but out of scope for first implementation
- explicitly defer direct bind, buffer factory, and internal-storage exposure

**Expected output**:
- a clean boundary between P1 API extension and later P2 or P4 work

## Migration Impact

| Cluster | Modules | API work needed in this feature? | Notes |
| --- | --- | --- | --- |
| Simple DSP | `eq_fir`, `eq_iir`, `drc`, `multiband_drc`, `crossover` | Low | Existing source/sink acquisition plus helper layer is sufficient; `dcblock` is now the reference pilot. |
| Simple DSP with reverse scan | `volume` | Yes | Needs bounded backward-inspection helpers, which are now available in the shared fragment layer. |
| Routing | `mux`, `demux`, `selector` | Yes | Needs wrap-boundary helpers; `mixer` is now the reference pilot. |
| Complex and ML | `tdfb`, `mfcc`, `tflm-classify`, `google_ctc_audio_processing`, `aria`, `rtnr` | Low to medium | Mostly refactoring after helper layer exists. |
| Timing-sensitive | `asrc`, `copier` | Partial | `asrc` depends on helper completeness beyond the pilots; `copier` stays later-scope. |
| Raw-data codecs | `dts`, `cadence_ipc3`, `nxp_eap`, `waves`, `passthrough` | No new core API expected | `dts`, `nxp_eap`, `waves`, and `passthrough` are refactored to current getters in prepare; `cadence_ipc3` required only an audit. |

## Recommended Sequencing

1. Freeze the module inventory and blocker classification.
2. Land the fragment-helper contract.
3. Land source-side LFT symmetry.
4. Convert the raw-data prepare-path metadata users, treating `cadence_ipc3` as an audit-only no-op if its prepare path does not use legacy metadata access.
5. Convert `dcblock` as the simple DSP pilot.
6. Convert `mixer` as the routing pilot.
7. Migrate `volume` next to exercise the rewind helpers, then use `mixer` as the reference for `mux`, `demux`, and `selector`.
8. Re-evaluate `asrc` after the helper layer proves stable outside the pilots.
9. Keep `copier`, direct bind, buffer factory, and internal-storage redesign in later roadmap work.

## Phase 1 Execution Notes

- The finalized helper names are `audio_fragment_bytes_without_wrap()`, `audio_fragment_rewind_bytes_without_wrap()`, `audio_fragment_wrap()`, `audio_fragment_wrap_w()`, `audio_fragment_rewind_wrap()`, and `audio_fragment_rewind_wrap_w()`.
- `source_get_data_fragment()` and `sink_get_buffer_fragment()` are thin wrappers over the existing acquisition APIs; they package the acquired tuple without changing reservation semantics.
- `source_get_last_feeding_time()` returns `UINT32_MAX` when a source provider has not implemented `source_ops.get_lft()` yet.
- `dts`, `nxp_eap`, `waves`, and `passthrough` now use existing sink/source getters in prepare-time metadata paths; `cadence_ipc3` required no prepare-time metadata rewrite.
- `dcblock` and `mixer` are now the preferred migration references for the next simple-DSP and routing conversions.
- Validation still needs a maintainer-approved representative build in an environment that allows terminal execution.

## Effort Estimate

| Work item | Points |
| --- | ---: |
| Inventory and blocker validation | 3 |
| Fragment-helper contract and header design | 4 |
| Helper implementation in shared utility layer | 4 |
| Source-side LFT symmetry | 1 |
| Raw-data codec metadata refactor path | 3 |
| Simple DSP pilot migration | 2 |
| Routing pilot migration | 3 |
| Documentation and static regression sweep | 2 |
| **Total** | **22** |

`22 points` corresponds to about `2.75 weeks` for one engineer under the agreed planning scale.

## Complexity Tracking

No constitution violations identified.
