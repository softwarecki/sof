# Phase 0 Research: Sink/Source API Extension

## Decision 1: Limit the inventory to runtime modules under `src/audio/`

- **Decision**: Inventory only runtime modules in `src/audio/` and `src/audio/module_adapter/module/` that still use `.process_audio_stream` or `.process_raw_data`.
- **Rationale**: The first migration phase is about firmware runtime behavior, not unit-test fixtures or tools. The constitution also keeps `test/` out of scope unless explicitly requested.
- **Alternatives considered**:
  - Include test code and helper fixtures: rejected because it obscures the runtime blocker map.
  - Include all buffer-related code in the tree: rejected because the feature is about module-facing migration blockers, not every internal audio-buffer user.

## Decision 2: Treat only fragment-navigation and source-side timing symmetry as true API gaps

- **Decision**: The true sink/source API gaps for this phase are:
  - circular-fragment navigation helpers for acquired source and sink fragments
  - limited backward-inspection helpers needed by `volume` and `asrc`
  - a source-side latest-feeding-time accessor symmetric with `sink_get_last_feeding_time()`
- **Rationale**: The code audit shows that several initially suspected gaps are already covered by existing sink/source getters. In particular, frame format, valid format, rate, channels, and buffer format are already available through the current API.
- **Alternatives considered**:
  - Add new metadata getters for buffer format: rejected because `source_get_buffer_fmt()` and `sink_get_buffer_fmt()` already exist.
  - Add a large new family of module-level buffer APIs: rejected because the existing source/sink acquisition model is already sufficient for most cases.

## Decision 3: Do not add a multi-source or multi-sink atomic reservation API in this phase

- **Decision**: Do not introduce an atomic multi-source or multi-sink acquisition API as part of the first extension phase.
- **Rationale**: Reservation state is tracked per `sof_source` and per `sof_sink`, not globally. Modules such as `mixer` can already hold one acquired fragment per input source at the same time, and `demux` can do the same per sink. The real blocker is wrap-boundary navigation on the acquired fragments, not a missing cross-stream reservation primitive.
- **Alternatives considered**:
  - Introduce `source_get_data_concurrent()` / `sink_get_buffer_concurrent()`: rejected because they add API surface without solving the core issue.
  - Keep using direct `audio_stream` pointers in multi-pin modules: rejected because it defeats the whole migration goal.

## Decision 4: Use a fragment-helper layer instead of re-exposing `audio_stream`

- **Decision**: Extend the sink/source model with helper functions operating on acquired fragments, rather than exposing `struct audio_stream` or raw buffer internals back to modules.
- **Rationale**: Legacy modules mostly rely on a small set of circular-buffer operations: bytes until wrap, samples until wrap, pointer wrap, and bounded reverse span. Those operations can be expressed on top of the `(data_ptr, buffer_start, buffer_size)` tuple already returned by `source_get_data()` and `sink_get_buffer()`.
- **Alternatives considered**:
  - Reintroduce `audio_stream` into module-facing code: rejected because it would preserve the hybrid model instead of removing it.
  - Force every module to open-code pointer arithmetic: rejected because it duplicates subtle wrap logic across many modules.

## Decision 5: Raw-data codecs do not need new core sink/source accessors

- **Decision**: Treat raw-data codec modules as `refactor-only` for metadata access unless later implementation uncovers a concrete missing primitive.
- **Rationale**: The audited raw-data codecs mainly reach through `comp_buffer->stream` in prepare-time logic to get format, rate, channels, or buffer format. Those are already available through current sink/source getters. The prepare paths in `dts`, `nxp_eap`, `waves`, and `passthrough` were converted to those getters. `cadence_ipc3` was audited separately and did not contain a prepare-time `comp_buffer->stream` metadata read that required replacement; its remaining raw-buffer usage is process-time local buffering and stays out of scope for this phase.
- **Alternatives considered**:
  - Add non-reserving metadata or free-space APIs for codecs: rejected for now because the current getters and size queries already provide the needed information.
  - Keep `comp_buffer` access in codec prepare paths: rejected because it would leave known legacy dependencies in place.

## Decision 6: Add source-side LFT symmetry in this phase

- **Decision**: Add a source-side latest-feeding-time query as part of this feature even though the main consumer is later DP-to-DP scheduling work.
- **Rationale**: The addition is small, local, and forward-compatible. Including it here prevents future churn in the sink/source contract when the scheduler work starts.
- **Alternatives considered**:
  - Defer source-side LFT until the DP scheduling phase: rejected because it would reopen the same API design area later.

## Decision 7: Keep `copier` and direct binding out of scope for first implementation

- **Decision**: Analyze `copier` as a critical downstream consumer of the new helper layer, but keep direct bind, internal-storage exposure, and gateway redesign outside this feature.
- **Rationale**: `copier` is the most architecture-sensitive module in the runtime. The first phase should only deliver the minimum sink/source contract needed to unblock later conversion, not absorb P2 work.
- **Alternatives considered**:
  - Fold `copier` direct-bind work into this feature: rejected because it merges P1 and P2 scope.
  - Ignore `copier` completely: rejected because the helper-layer design must remain compatible with its later needs.

## Legacy Module Inventory

### Modules still using `.process_audio_stream` after the Phase 1 pilot conversions

- **Simple 1-in/1-out DSP**: `eq_fir`, `eq_iir`, `drc`, `multiband_drc`, `crossover`
- **Simple DSP with backward scan**: `volume`
- **Multi-pin and routing**: `mux`, `demux`, `selector`
- **Timing-sensitive**: `asrc`, `copier`
- **Complex or ML/analysis**: `tdfb`, `mfcc`, `tflm-classify`, `google_ctc_audio_processing`, `aria`, `rtnr`

### Modules still using `.process_raw_data`

- `dts`
- `cadence_ipc3`
- `nxp_eap`
- `waves`
- `passthrough`

### Pilot migrations completed in this phase

- `dcblock`
- `mixer`

### Already migrated reference implementations

- `tone`
- `template`
- `level_multiplier`
- `up_down_mixer`
- `igo_nr`
- `stft_process`
- `sound_dose`
- `src`
- `dcblock`
- `mixer`

## Verified Existing Capabilities That Do Not Need New API Work

- `source_get_frm_fmt()` / `sink_get_frm_fmt()`
- `source_get_valid_fmt()` / `sink_get_valid_fmt()`
- `source_get_rate()` / `sink_get_rate()`
- `source_get_channels()` / `sink_get_channels()`
- `source_get_buffer_fmt()` / `sink_get_buffer_fmt()`
- `source_get_data_available()` / `sink_get_free_size()`
- `source_get_data_frames_available()` / `sink_get_free_frames()`
- per-source and per-sink fragment acquisition through `source_get_data()` and `sink_get_buffer()`

## True Capability Gaps To Cover In This Feature

1. **Fragment wrap-boundary helpers**
   - Needed by `mixer`, `mux`, `demux`, `selector`, `volume`, `asrc`, and some optimized DSP paths.
   - Required operations:
     - bytes until wrap
     - frames or samples until wrap
     - pointer wrap inside an acquired fragment

2. **Bounded backward-inspection helpers**
   - Needed by `volume` zero-crossing logic and `asrc` pointer-heavy copy paths.
   - Required operations:
     - bytes available before the current pointer when moving backward
     - pointer rewind within the logical circular fragment

3. **Source-side latest feeding time**
   - Needed for later DP-to-DP scheduling work.
   - Best expressed as a source-side mirror of the existing sink-side LFT query.

No additional backward-inspection gap remained after auditing `volume` and `asrc`; the shared helper layer now carries both rewind-distance and rewind-wrap equivalents for follow-on migrations.

## Migration Impact By Cluster

| Cluster | Modules | Needs new API work? | Notes |
| --- | --- | --- | --- |
| Simple DSP | `eq_fir`, `eq_iir`, `drc`, `multiband_drc`, `crossover` | Mostly no | Existing acquisition plus helper layer is enough; `dcblock` is now the pilot reference. |
| Simple DSP with backward scan | `volume` | Yes | Uses rewind-distance helpers that are now present in the shared fragment layer. |
| Routing and multi-pin | `mux`, `demux`, `selector` | Yes | Needs wrap-boundary helpers, not a new multi-stream reservation API; `mixer` is now the pilot reference. |
| Complex and ML | `tdfb`, `mfcc`, `tflm-classify`, `google_ctc_audio_processing`, `aria`, `rtnr` | Mostly no | Existing source/sink model is already close to sufficient. |
| Timing-sensitive | `asrc`, `copier` | Partially | `asrc` can follow once helper usage beyond the pilots is validated; `copier` remains later-scope analysis. |
| Raw-data codecs | `dts`, `cadence_ipc3`, `nxp_eap`, `waves`, `passthrough` | No new core API expected | `dts`, `nxp_eap`, `waves`, and `passthrough` now use current getters in prepare; `cadence_ipc3` required no prepare-time metadata rewrite. |

## Validation Results

- A static inventory sweep still finds the remaining `.process_audio_stream` and `.process_raw_data` users, while `dcblock` and `mixer` no longer appear in the `.process_audio_stream` set.
- A targeted reverse-scan audit confirmed that `volume` still depends on `audio_stream_rewind_bytes_without_wrap()` and `audio_stream_rewind_wrap()`, and the shared fragment helper layer now exposes direct equivalents for both operations.
- A targeted tree search did not find additional rewind-helper users under `src/audio/asrc/`.
- A representative firmware build via `scripts/xtensa-build-zephyr.py -p ptl` was attempted as the documented validation path, but terminal execution was skipped, so build validation remains unconfirmed for this workspace.