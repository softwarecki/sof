# Quickstart: Implementing the First Sink/Source API Extension Phase

## Goal

Implement only the first enablement phase:

- close the real sink/source API gaps
- prepare the module inventory and migration order
- avoid pulling in direct-bind or buffer-factory work

## Suggested Implementation Order

1. **Lock the inventory and blocker map**
   - Freeze the list of runtime legacy modules.
   - Confirm which blockers are true API gaps and which are only refactoring work.
   - Keep pilot modules out of the remaining legacy inventory once they convert to `.process`.

2. **Add the fragment-helper layer**
   - Extend the module-facing sink/source headers with helper functions operating on acquired fragments.
   - Prefer generic byte-based helpers plus small frame or sample wrappers rather than reintroducing `audio_stream`.
   - Keep the helper logic reusable by `volume`, `mixer`, `mux`, `demux`, `selector`, and `asrc`.

3. **Add source-side timing symmetry**
   - Introduce a source-side latest-feeding-time query.
   - Mirror the sink-side timing contract closely enough that later DP scheduling code can use it without reopening the API design.

4. **Refactor the metadata-only raw-data modules first**
   - Start with `passthrough`, `dts`, `cadence_ipc3`, `nxp_eap`, and `waves`.
   - Replace prepare-time access to `comp_buffer->stream` with existing sink/source getters.
   - Record `cadence_ipc3` as an audited no-op if its prepare path does not actually depend on `comp_buffer->stream` metadata.
   - Do not add new API for these modules unless the code audit uncovers a concrete missing primitive.

5. **Use one simple DSP and one routing module as validation pilots**
   - The chosen early validation set for this phase is:
     - `dcblock` for a simple 1-in/1-out conversion
     - `mixer` for wrap-boundary helper validation and multi-source acquisition

6. **Defer the highest-risk modules**
   - Use `volume` as the first remaining rewind-helper validation target after the pilots.
   - Keep `asrc` after the helper layer is stable.
   - Keep `copier` analysis-only in this first phase.

## Recommended Reference Modules

- `src/audio/template/` for baseline source/sink processing flow
- `src/audio/level_multiplier/` for optimized format-specific processing in the new model
- `src/audio/src/` for timing-sensitive sink/source processing without legacy stream access
- `src/audio/up_down_mixer/` for a migrated multi-stream processing example
- `src/audio/dcblock/` as the Phase 1 simple DSP pilot using fragment descriptors
- `src/audio/mixer/` as the Phase 1 routing pilot using multi-source fragment acquisition

## Pilot Module Rationale

- `dcblock` proves that a simple 1-in/1-out DSP path can move from `.process_audio_stream` to `.process` using only sink/source handles and fragment helpers.
- `mixer` proves that the helper layer is sufficient for wrap-aware, alignment-sensitive multi-source routing without adding a multi-stream reservation API.

## Files Most Likely To Change During Implementation

- `src/include/module/audio/source_api.h`
- `src/include/module/audio/sink_api.h`
- `src/include/sof/audio/source_api.h`
- `src/include/sof/audio/sink_api.h`
- `src/audio/sink_source_utils.c`
- `src/audio/module_adapter/module_adapter.c`

## Verification Approach

Use existing checks only. Do not add or modify test files in this phase.

1. Run a static inventory sweep over runtime modules.
2. Verify that raw-data codecs can use existing sink/source metadata getters.
3. Verify that the fragment-helper layer covers wrap-boundary and rewind-distance calculations needed by representative legacy modules, including `volume` and `asrc` audits for backward inspection.
4. Run the repository's standard build and existing regression checks for a representative firmware target chosen by the maintainer.
5. If the representative build step cannot be run in the current workspace, record that validation delta explicitly instead of treating it as passed.

## Useful Audit Queries

```text
rg "\.process_audio_stream\s*=|\.process_raw_data\s*=" src/audio
rg "audio_stream_|comp_buffer" src/audio
rg "source_get_|sink_get_" src/audio
```

## Exit Criteria For This Feature

- The sink/source contract contains the fragment-navigation helpers required by legacy modules.
- Source-side LFT symmetry is defined.
- The legacy module inventory is frozen and categorized.
- Raw-data codec modules are confirmed to be `refactor-only` for metadata access, with `cadence_ipc3` explicitly audited as a no-op for prepare-time getter replacement.
- `dcblock` and `mixer` serve as the Phase 1 pilot references for the next migration slice.
- Direct bind and copier redesign remain explicitly deferred.