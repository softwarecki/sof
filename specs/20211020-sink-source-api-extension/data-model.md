# Data Model: Sink/Source API Extension

## Legacy Module

Represents one runtime module still coupled to the legacy stream or buffer interface.

| Field | Type | Description |
| --- | --- | --- |
| `name` | string | Module name, for example `volume` or `mixer`. |
| `path` | string | Source file path under `src/audio/`. |
| `process_mode` | enum | One of `source_sink`, `audio_stream`, or `raw_data`. |
| `cluster` | enum | One of `simple_dsp`, `routing`, `timing_sensitive`, `complex_ml`, `raw_codec`. |
| `legacy_dependencies` | set | The legacy operations still used by the module. |
| `true_api_gap` | set | Missing sink/source capabilities that block migration, if any. |
| `reference_module` | optional string | Existing migrated module that can be used as an implementation reference. |
| `phase_priority` | enum | `now`, `after_helper_layer`, `later_phase`. |

### States

1. `inventoried`
2. `classified`
3. `refactor_only` or `api_blocked`
4. `ready_for_migration`

## Source Fragment

Represents the read-only fragment acquired through `source_get_data_fragment()`.

| Field | Type | Description |
| --- | --- | --- |
| `data_ptr` | pointer | Current read pointer returned by acquisition. |
| `buffer_start` | pointer | Logical start of the circular storage. |
| `buffer_size` | `size_t` | Total size of the circular storage in bytes. |

The owning source handle, requested size, and frame size remain call-site context supplied to helper wrappers rather than stored in the descriptor itself.

### Derived values

- `bytes_without_wrap`
- `frames_without_wrap`
- `samples_without_wrap`
- `rewind_bytes_without_wrap`

## Sink Fragment

Represents the writable fragment acquired through `sink_get_buffer_fragment()`.

| Field | Type | Description |
| --- | --- | --- |
| `data_ptr` | pointer | Current write pointer returned by acquisition. |
| `buffer_start` | pointer | Logical start of the circular storage. |
| `buffer_size` | `size_t` | Total size of the circular storage in bytes. |

The owning sink handle, requested size, and frame size remain call-site context supplied to helper wrappers rather than stored in the descriptor itself.

### Derived values

- `bytes_without_wrap`
- `frames_without_wrap`
- `samples_without_wrap`
- `rewind_bytes_without_wrap`

## Fragment Helper

Represents one helper-layer addition that operates on an acquired fragment without exposing `audio_stream`.

| Field | Type | Description |
| --- | --- | --- |
| `name` | string | Helper function name. |
| `layer` | enum | `source`, `sink`, or `shared_fragment_helper`. |
| `operation` | enum | `forward_wrap`, `reverse_wrap`, `distance_without_wrap`, `rewind_distance`, `time_query`, or `acquisition_wrapper`. |
| `modules_unblocked` | set | Legacy modules that depend on the helper. |
| `scope` | enum | `phase1_required` or `later_phase`. |

## Capability Gap

Represents a missing function or helper that must be added before at least one module cluster can migrate.

| Field | Type | Description |
| --- | --- | --- |
| `id` | string | Stable identifier, for example `gap-fragment-wrap`. |
| `title` | string | Short human-readable title. |
| `kind` | enum | `helper_layer`, `public_getter`, `ops_extension`. |
| `severity` | enum | `blocker`, `important`, `future_ready`. |
| `affected_clusters` | set | Migration clusters blocked by the gap. |
| `resolved_by` | set | Helper or API additions that close the gap. |

### Phase 1 gaps resolved by this feature

- `gap-fragment-wrap` -> `audio_fragment_bytes_without_wrap`, `audio_fragment_wrap`, `audio_fragment_wrap_w`
- `gap-fragment-rewind` -> `audio_fragment_rewind_bytes_without_wrap`, `audio_fragment_rewind_wrap`, `audio_fragment_rewind_wrap_w`
- `gap-source-lft` -> `source_ops.get_lft`, `source_get_last_feeding_time`

## Migration Cluster

Represents a group of modules that can be migrated with the same implementation pattern.

| Field | Type | Description |
| --- | --- | --- |
| `name` | string | Cluster name. |
| `modules` | set | Modules in the cluster. |
| `needs_new_api` | bool | Whether the cluster is blocked on feature work in this phase. |
| `reference_modules` | set | Existing sink/source modules to copy patterns from. |
| `planned_order` | integer | Relative migration order once implementation starts. |

## Reference Module

Represents an already migrated module that demonstrates how the new sink/source model should be used.

| Field | Type | Description |
| --- | --- | --- |
| `name` | string | Reference module name. |
| `path` | string | Path under `src/audio/`. |
| `why_useful` | string | The migration pattern it demonstrates. |
| `pilot_reason` | optional string | Why it was chosen as a validation pilot in this phase, if applicable. |

## Relationships

- A `Legacy Module` belongs to one `Migration Cluster`.
- A `Legacy Module` may depend on zero or more `Capability Gaps`.
- A `Capability Gap` is resolved by one or more `Fragment Helpers` or one public accessor.
- A `Legacy Module` migration may cite one or more `Reference Modules`.
- A `Legacy Module` may become a `Reference Module` once a pilot conversion lands, as happened for `dcblock` and `mixer` in this phase.
- A `Source Fragment` or `Sink Fragment` is the runtime object operated on by `Fragment Helpers`.