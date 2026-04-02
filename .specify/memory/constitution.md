<!-- Sync Impact Report
  Version change: 0.0.0 → 1.0.0 (initial ratification)
  Modified principles: N/A (first version)
  Added sections:
    - Core Principles (5 principles)
    - Technology Constraints
    - Development Workflow
    - Governance
  Removed sections: N/A
  Templates requiring updates:
    - .specify/templates/plan-template.md — ✅ no changes needed (Constitution Check section is generic)
    - .specify/templates/spec-template.md — ✅ no changes needed (technology-agnostic)
    - .specify/templates/tasks-template.md — ✅ no changes needed (test tasks already marked OPTIONAL)
    - .specify/templates/checklist-template.md — ✅ no changes needed (generic)
    - .specify/templates/agent-file-template.md — ✅ no changes needed (generic)
  Follow-up TODOs: none
-->

# Sound Open Firmware (SOF) Constitution

## Core Principles

### I. Pure C, Simple and Readable (NON-NEGOTIABLE)

- All code MUST be written in plain C. No C++ constructs, no external scripting languages in firmware code paths.
- Code MUST be simple and readable. Favor explicit logic over clever abstractions.
- Functions MUST be short and single-purpose. If a function exceeds ~60 lines, it MUST be split.
- Variable and function names MUST be descriptive and follow existing SOF naming conventions (`snake_case`).
- Comments MUST explain *why*, not *what*. The code itself MUST be clear enough to show *what*.

**Rationale**: SOF runs on resource-constrained DSP hardware. Simple C code is easier to audit, debug on bare-metal/JTAG, and reason about in real-time contexts. Complexity kills firmware reliability.

### II. Zero New Dependencies (NON-NEGOTIABLE)

- No new external libraries or dependencies MUST be added without explicit user approval.
- No new Zephyr subsystems MUST be enabled unless the user explicitly requests it.
- If a feature requires functionality not present in the current codebase, the agent MUST ask the user before introducing any dependency.
- Reuse existing SOF utilities (`src/lib/`, `src/math/`, `src/include/`) and Zephyr APIs already linked into the build.

**Rationale**: Every dependency increases binary size, attack surface, and maintenance burden on an embedded platform with strict memory constraints. The user decides what enters the build.

### III. No New Tests (NON-NEGOTIABLE)

- The agent MUST NOT create, add, or modify test files unless the user explicitly requests it.
- Existing tests MUST NOT be broken by code changes. If a change causes test regressions, the agent MUST report this and ask the user how to proceed.
- The `test/` and `tools/` directories are out of scope for agent modifications unless directed.

**Rationale**: Test creation and strategy are decisions the user retains full control over. The agent implements features; the user decides when and how to test them.

### IV. Ask, Don't Guess — User Has Full Control

- When the agent encounters ambiguity in requirements, design choices, or implementation alternatives, it MUST stop and ask the user.
- The agent MUST NOT make architectural decisions, choose between implementation alternatives, or assume default behavior without user confirmation.
- Every decision point MUST be presented to the user with clear options and trade-offs.
- If the agent is unsure whether a change is safe, it MUST flag the uncertainty explicitly rather than proceeding silently.

**Rationale**: This is embedded firmware running on production audio hardware. A wrong assumption can cause audio glitches, crashes, or security vulnerabilities. The user is the domain expert and MUST retain full control over all decisions.

### V. Documentation Discipline

- All new C code MUST include Doxygen comments for public functions, structures, and enums.
- Code changes MUST NOT introduce new Doxygen warnings or errors.
- When adding or modifying a file, the agent MUST review any `architecture.md` or `README.md` in the same directory and update them if the code logic changes affect documented behavior.
- Commit messages MUST follow the format: `feature: descriptive title` with a detailed body and `Signed-off-by` line from local git config.

**Rationale**: SOF is an open-source project with external contributors. Accurate documentation is essential for onboarding, code review, and long-term maintenance. Stale docs are worse than no docs.

## Technology Constraints

- **Language**: C (C11 or as configured by the platform's toolchain)
- **RTOS**: Zephyr (as integrated via `west.yml` and `zephyr/` directory)
- **Build System**: CMake with Zephyr's build infrastructure
- **Target Platforms**: Intel audio DSPs (Xtensa/Tensilica), with POSIX stubs for host testing
- **License**: BSD-3-Clause — all new code MUST be compatible with this license
- **Codestyle**: Enforced via `clangd` (NOT `checkpatch`). `clangd` provides better IDE integration and handles non-standard C / assembly correctly.
- **IPC Protocol**: IPC3 and IPC4 as defined by the existing SOF protocol versions

## Development Workflow

- **Before writing code**: The agent MUST read relevant existing source files, headers, and documentation to understand the current architecture and conventions.
- **Before modifying a file**: The agent MUST check for an `architecture.md` or `README.md` in the same directory and plan updates if needed.
- **Code changes**: MUST be minimal and focused. One logical change per commit. No drive-by refactors unless explicitly requested.
- **Error handling**: MUST follow existing SOF patterns — return negative errno values, use `tr_err`/`tr_dbg` logging macros, clean up resources on error paths.
- **Memory management**: MUST use SOF's existing allocation APIs (`rzalloc`, `rballoc`, `k_heap_alloc`) appropriate to the context (kernel vs. userspace).
- **Build verification**: Code changes MUST compile cleanly with no new warnings under the project's standard build configuration.

## Governance

- This constitution supersedes all other development practices for AI-assisted work on this repository.
- Amendments require explicit user approval. The agent MUST NOT modify this document without user direction.
- All code reviews and pull requests MUST verify compliance with these principles.
- If a principle conflicts with a specific task requirement, the agent MUST flag the conflict and let the user decide which takes precedence.
- Use `AGENTS.md` at the repository root as the runtime development guidance file for agent-specific workflow details.

**Version**: 1.0.0 | **Ratified**: 2026-04-02 | **Last Amended**: 2026-04-02
