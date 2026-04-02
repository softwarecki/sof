# Feature Specification: Flat Pipeline Traversal and pipeline_module Kernel Structure

**Feature Branch**: `20211019-flat-pipeline-module-struct`  
**Created**: 2026-04-02  
**Status**: Draft  
**Input**: User description: "Replace recursive pipeline buffer traversal with flat ordered module lists. Introduce a new pipeline_module structure for kernel-only fields, preparing a clean split from processing_module which will eventually hold only userspace-accessible fields."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Replace Recursive Traversal with Flat Ordered Module List (Priority: P1)

A firmware developer maintaining pipeline execution code currently deals with a recursive graph walk (`pipeline_for_each_comp`) that traverses components through `bsource_list`/`bsink_list` buffer chains. This recursion makes execution order hard to reason about, produces deep call stacks that are difficult to debug, and relies on a fragile `walking` flag for cycle prevention. The pipeline must instead maintain a flat ordered list of modules built at bind time. At runtime, pipeline operations (copy, trigger, prepare, reset, params) iterate this list in order — no recursion. For IPC3 backward compatibility, the existing recursive traversal may remain but must produce the same flat list during bind, which is then used at runtime.

**Why this priority**: Eliminating recursion is the foundational change that simplifies the entire pipeline execution path. All subsequent improvements (DP-to-DP deadline propagation, better debugging, deterministic ordering) depend on having a predictable flat execution list. It also directly reduces stack usage for complex multi-module topologies.

**Independent Test**: Take an existing IPC4 pipeline topology with 4+ modules (e.g., host → copier → gain → mixin → mixout → copier → DAI). Build the flat list at bind time. Replace `pipeline_copy()` to iterate the flat list instead of recursing. Verify bit-identical output for playback and capture. Verify the same module execution order as the recursive approach.

**Acceptance Scenarios**:

1. **Given** an IPC4 pipeline with modules bound, **When** `pipeline_copy()` is called, **Then** modules are invoked by iterating the flat ordered list — no call to `pipeline_for_each_comp()`.
2. **Given** an IPC4 pipeline, **When** `pipeline_trigger()` is called, **Then** the trigger propagates by iterating the flat list — no recursive graph walk.
3. **Given** an IPC4 pipeline, **When** `pipeline_prepare()` and `pipeline_params()` are called, **Then** they iterate the flat list — no recursion.
4. **Given** an IPC3 pipeline, **When** modules are bound, **Then** the existing recursive traversal produces a flat ordered list during bind, and that list is used at runtime for all operations.
5. **Given** a pipeline with fan-out (one module feeding multiple downstream modules), **When** the flat list is built, **Then** all downstream branches are included in the correct processing order.

---

### User Story 2 - Introduce pipeline_module Kernel Structure (Priority: P2)

A kernel developer working on pipeline infrastructure needs a clear separation between fields that belong to the pipeline/scheduling kernel and fields that are (or will be) accessible to a module running in userspace. Today, `processing_module` contains both kinds of fields, with the kernel-only portion guarded behind `#ifdef SOF_MODULE_API_PRIVATE`. A new `pipeline_module` structure is introduced to hold kernel-owned fields — starting with the flat list linkage from P1 and a back-pointer to `processing_module`. At this stage, existing kernel-only fields are NOT mass-migrated from `processing_module`; the structure is simply established. Going forward, whenever a new kernel-only field is needed, it goes into `pipeline_module`. Over time, existing kernel-only fields will be migrated as opportunities arise, eventually leaving `processing_module` with only userspace-accessible fields.

**Why this priority**: The `pipeline_module` structure is needed to hold the flat list entry (from P1), making it a natural companion to the traversal rework. Establishing it now — even with minimal initial fields — sets the convention for all future kernel-side additions and prevents further pollution of `processing_module` with kernel internals.

**Independent Test**: After introducing `pipeline_module`, verify that: (a) the flat list operates through `pipeline_module` entries, (b) `processing_module` remains unchanged for all existing module code — no module source file needs modification, (c) the new structure compiles on all supported platforms.

**Acceptance Scenarios**:

1. **Given** the new `pipeline_module` structure, **When** a module is instantiated, **Then** a corresponding `pipeline_module` is created and linked to the `processing_module`.
2. **Given** the flat execution list, **When** it is iterated, **Then** it traverses `pipeline_module` entries (not `processing_module` or `comp_dev` directly).
3. **Given** an existing module (e.g., gain, EQ), **When** it is compiled after the change, **Then** it requires zero source modifications — the `processing_module` public interface is unchanged.
4. **Given** a developer adding a new kernel-only field (e.g., a scheduling hint), **When** they decide where to place it, **Then** the convention directs them to `pipeline_module`, not `processing_module`.

---

### Edge Cases

- How does the flat list handle a module that is bound to multiple pipelines (shared component)?
- What happens if `pipeline_module` is accessed after the corresponding `processing_module` is freed (lifecycle mismatch)?
- How does the flat list ordering handle fan-in topologies (multiple producers feeding one mixin)?
- What happens when a module is unbound and re-bound — is the flat list rebuilt or patched?
- How does the flat list behave during partial pipeline construction (some modules bound, others not yet)?

## Requirements *(mandatory)*

### Functional Requirements

**Flat Execution List (P1)**

- **FR-001**: Each pipeline MUST maintain a flat ordered list of modules reflecting the correct processing execution order for that pipeline.
- **FR-002**: For IPC4 pipelines, the flat execution list MUST be built (or updated) during module bind/unbind operations.
- **FR-003**: For IPC3 pipelines, the existing recursive traversal MUST produce the same flat execution list during bind, which is used at runtime.
- **FR-004**: `pipeline_copy()` MUST iterate the flat list to invoke module processing — no recursive `pipeline_for_each_comp()` calls in the IPC4 copy path.
- **FR-005**: `pipeline_trigger()` MUST iterate the flat list to propagate trigger commands — no recursive graph walk in the IPC4 trigger path.
- **FR-006**: `pipeline_prepare()` and `pipeline_params()` MUST iterate the flat list — no recursion in the IPC4 prepare/params path.
- **FR-007**: `pipeline_comp_reset()` MUST iterate the flat list — no recursion in the IPC4 reset path.
- **FR-008**: The flat list MUST correctly represent fan-out topologies (one module feeding multiple downstream modules) and fan-in topologies (multiple modules feeding one module).
- **FR-009**: The flat list MUST be updated when a module is unbound or re-bound — the list reflects the current pipeline graph at all times.
- **FR-010**: The `pipeline_for_each_comp()` recursive function MAY remain in the codebase for IPC3 compatibility but MUST NOT be called from any IPC4 runtime path (copy, trigger, prepare, params, reset).

**pipeline_module Structure (P2)**

- **FR-011**: A new `pipeline_module` structure MUST be introduced to hold kernel-only pipeline/scheduling fields.
- **FR-012**: `pipeline_module` MUST contain at minimum: (a) a flat list linkage entry, (b) a pointer to the associated `processing_module`, and (c) a pointer to the owning pipeline.
- **FR-013**: The flat execution list MUST be composed of `pipeline_module` entries — iteration walks `pipeline_module` nodes.
- **FR-014**: Every `processing_module` instance MUST have a corresponding `pipeline_module` created and linked during module instantiation.
- **FR-015**: The public interface of `processing_module` (fields visible without `SOF_MODULE_API_PRIVATE`) MUST remain unchanged — no existing module source code requires modification.
- **FR-016**: New kernel-only fields added after this change MUST be placed in `pipeline_module`, not `processing_module`.
- **FR-017**: Existing kernel-only fields in `processing_module` are NOT required to be migrated in this change — migration happens incrementally in future work.

### Key Entities

- **pipeline_module**: A new kernel-owned structure representing a module's presence in a pipeline. Contains the flat list linkage, back-pointer to `processing_module`, pointer to the owning `pipeline`, and will accumulate kernel-only fields over time. Created alongside `processing_module` during module instantiation.
- **processing_module**: The existing module structure. After this change, its public interface remains identical. Over time, kernel-only fields (currently behind `SOF_MODULE_API_PRIVATE`) will migrate to `pipeline_module`, leaving only userspace-accessible fields.
- **Flat Execution List**: An ordered sequence of `pipeline_module` entries attached to a `pipeline`, reflecting the correct module processing order. Built at bind time, iterated at runtime.
- **pipeline_for_each_comp**: The existing recursive graph walker. After this change, it is no longer called from IPC4 runtime paths. It may remain for IPC3 compatibility and as a utility for flat list construction.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: All IPC4 pipeline runtime operations (copy, trigger, prepare, params, reset) use flat list iteration — zero calls to `pipeline_for_each_comp()` in IPC4 runtime paths (verified by code review or instrumentation).
- **SC-002**: All existing pipeline test suites pass without modification after the traversal change (bit-identical output for playback and capture).
- **SC-003**: The `pipeline_module` structure exists and is instantiated for every module — verified by compilation on all supported platforms and a runtime assertion that every `processing_module` has a linked `pipeline_module`.
- **SC-004**: Zero existing module source files (gain, EQ, copier, mixin/mixout, etc.) require changes due to this work — the `processing_module` public API is untouched.
- **SC-005**: Stack usage during pipeline operations is measurably reduced compared to the recursive approach for pipelines with 4+ modules (verified by stack analysis or profiling).
- **SC-006**: IPC3 pipeline execution continues to work correctly — recursive traversal produces the flat list at bind time, and runtime uses the flat list.

## Assumptions

- IPC4 is the primary target for eliminating recursion. IPC3 compatibility is preserved by keeping the recursive traversal as a utility that produces the flat list, but the runtime hot path uses the flat list for both IPC versions.
- The `pipeline_module` structure starts minimal (list linkage + back-pointer + pipeline pointer). No mass field migration from `processing_module` is in scope. The convention is established; migration is incremental and opportunistic.
- The flat list ordering for fan-out follows the existing buffer list ordering in `bsink_list` — this preserves the current implicit execution order. If a different ordering is needed in the future, it can be addressed separately.
- The `pipeline_module` lifecycle mirrors `processing_module`: created together, freed together. There is no scenario where one exists without the other.
- This work is scoped to pipeline operations that currently use `pipeline_for_each_comp()`. Other code paths that walk the component graph for non-pipeline purposes (e.g., IPC topology queries) are not in scope.
- The `pipeline_module` header will be a kernel-internal header not visible to userspace module builds, enforcing the separation by build system rather than relying solely on `#ifdef` guards.
