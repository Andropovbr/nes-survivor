# AGENTS.md

## Project

NES Survivor is a real Nintendo Entertainment System game: a fixed-arena survivor-like inspired by Robotron-style action.

Current product direction:

- target: NES, NROM / Mapper 0;
- one arena, no scrolling;
- C with cc65 is the default;
- 6502 Assembly is used only when hardware access or measured performance justifies it;
- automatic weapons, enemy waves, XP, level-up choices, characters, upgrades and long-term password progression are expected systems.

The repository must remain buildable, measurable and understandable while gameplay grows.

## Priority order

When constraints conflict, prefer:

1. Correct runtime behavior.
2. NES hardware safety.
3. Small, reviewable scope.
4. Clear and simple implementation.
5. Measured performance.
6. Resource efficiency.
7. Extensibility that is required by current work.

Do not optimize hypothetical problems or introduce architecture for unrequested future features.

## Cost-aware agent policy

This repository may be worked on by multiple AI runtimes (notably Codex and Gemini CLI).
`AGENTS.md` is the canonical project policy. Provider-specific files are adapters and must
not redefine project architecture or workflow rules.

Shared reusable agent roles live under `.agents/roles/`.
Shared skills live under `.agents/skills/`.


Use the cheapest capable execution path.

Subagents are not free: every spawned agent performs its own model and tool work. Do not delegate merely because delegation is available.

### Default workflow

For ordinary implementation:

1. Inspect only the files needed to understand the task.
2. Decide whether a specialist is actually required.
3. Prefer a lightweight implementation agent for bounded, already-understood work.
4. Use deterministic skills/scripts for build, tests, budget extraction and documentation workflows.
5. Use one focused review pass after implementation.
6. Escalate to a specialist only when evidence reveals an architectural or performance decision.

Avoid parallel write-heavy agents. Parallelism is mainly appropriate for independent read-only exploration, tests, triage or evidence gathering.

### Specialist routing

Use the architecture-reviewer role (`seu_camilo` in provider adapters) only when work includes a real architecture decision, for example:

- ownership or lifetime of runtime state;
- module boundaries with meaningful coupling;
- data representation or pool design;
- RAM / Zero Page layout decisions;
- NMI / PPU contracts;
- C versus Assembly decisions;
- persistent game-state architecture;
- unclear trade-offs that affect multiple systems.

Do not use `seu_camilo` for routine implementation, formatting, straightforward tests, documentation or mechanical refactors.

Use the performance-reviewer role (`relampago_marquinhos` in provider adapters) only for evidence-based performance work, for example:

- a measured or reproducible frame-time problem;
- NMI/VBlank pressure;
- entity-update hot paths;
- collision scaling;
- OAM construction cost;
- generated cc65 code that appears unexpectedly expensive;
- RAM/ZP/PRG pressure requiring trade-off analysis.

Do not invoke it for speculative optimization.

Use the implementation-worker role (`ze_da_oficina` in provider adapters) for bounded implementation after requirements and design are sufficiently clear.

Use the final-reviewer role (`fiscal` in provider adapters) for focused post-change review. The fiscal checks correctness, scope, validation and missing tests; it does not redesign the system.

### Escalation rule

A lightweight agent that encounters an unresolved architectural or performance decision must report the decision and evidence instead of inventing a broad redesign.

Do not ask multiple expensive specialists to independently review the same question unless their domains are genuinely different and both are necessary.

### Communication budget

Keep agent-to-agent reports concise:

- lead with findings or result;
- cite relevant files/symbols;
- include only evidence needed by the parent;
- avoid narrating obvious steps;
- avoid restating this file;
- avoid dumping large diffs or full source files.

Use the `caveman` skill when a task is producing excessive narration or when terse handoff is explicitly useful.

## Toolchain

Use the repository's existing toolchain:

- cc65 for C;
- ca65 for Assembly;
- ld65 for linking;
- existing NROM linker configuration;
- Mesen for runtime debugging and validation.

Do not replace the toolchain without an explicit task and documented migration impact.

On Windows, before claiming `make` is unavailable, verify:

```powershell
where.exe make
Get-Command make -ErrorAction SilentlyContinue
make --version
```

## Source and API design

Keep modules focused and create them when needed by actual work, not to mirror a hypothetical future architecture.

Expected domains include:

- game state / run lifecycle;
- player and input;
- PPU / rendering / NMI;
- OAM;
- enemies;
- weapons and projectiles;
- XP gems;
- waves;
- upgrades;
- characters;
- RNG;
- unlocks / password progression;
- tuning and limits.

Headers expose the smallest useful interface.

- Avoid circular includes.
- Avoid exposing mutable internals without need.
- Prefer IDs and indexes to unnecessary pointers.
- Put gameplay tuning in `tuning.h` or a clearly related tuning file.
- Keep hardware constants in hardware-specific headers.
- Avoid gameplay magic numbers.

## Data and memory rules

The NES has no room for casual allocation.

- Never use heap allocation.
- Prefer fixed-size pools.
- Use `<stdint.h>` fixed-width integer types.
- Avoid floating point.
- Document fixed-point formats.
- Avoid ambiguous-width `int` in resource-sensitive/runtime data.
- Prevent accidental arithmetic overflow.
- Use saturating arithmetic where exceeding a cap is invalid.
- Keep expensive multiply/divide out of hot paths when a simpler representation is clear.

For an important runtime pool, know:

- maximum count;
- bytes per element;
- total RAM cost;
- inactive representation;
- allocation behavior;
- update behavior;
- rendering behavior;
- pool-full behavior.

Static definitions and runtime state should be separate when that distinction is useful.

## C / Assembly boundary

C is the gameplay default.

Assembly is justified for hardware-facing code or a measured bottleneck, such as:

- NMI entry/exit;
- OAM DMA;
- controller reads;
- bounded copy/clear routines;
- measured collision/entity loops;
- fixed-point helpers.

Do not move code to Assembly merely because it runs every frame.

For handwritten Assembly, document when relevant:

- purpose and why Assembly is justified;
- calling convention;
- inputs/outputs;
- clobbered registers;
- Zero Page usage;
- interrupt/reentrancy assumptions;
- measured or estimated cycle cost.

Keep C/Assembly interfaces small and stable.

## Frame, NMI and PPU safety

- Synchronize the main loop with NMI.
- Gameplay normally updates outside NMI.
- NMI should perform bounded hardware-transfer work and frame signaling.
- Keep PPU writes inside valid rendering-disabled or VBlank periods.
- Use the OAM shadow buffer.
- Do not issue uncontrolled PPU writes from arbitrary gameplay modules.
- Preserve registers correctly in interrupt handlers.
- Track VBlank/NMI workload when relevant.

## Sprite policy

NES limits:

- 64 hardware sprites total;
- 8 hardware sprites per scanline.

Therefore:

- entity count is not the same as rendered-sprite count;
- enforce deterministic rendering budgets and priorities;
- player and dangerous threats outrank cosmetic effects;
- nonessential effects may be skipped;
- use deliberate flicker management when needed;
- XP gems must not permanently starve critical sprites;
- document worst-case sprite cost for multisprite entities.

## Gameplay-system constraints

### XP

XP must not disappear silently because a fixed pool is full.

Use a deterministic condensation/overflow strategy such as merging, regional accumulation, pending XP, gem upgrading or a documented combination.

Test saturation.

### Weapons

Support data-driven static definitions and compact per-run runtime state where practical.

Avoid a separate bespoke update loop for every weapon when a clear shared mechanism works.

Design weapons with NES sprite/CPU budgets in mind.

### Upgrades

Presentation and application must remain separate.

Selection should eventually support eligibility, rarity, caps, restrictions, duplicate prevention and deterministic RNG.

Handle fewer eligible choices than the configured offer count predictably.

### Characters

Prefer table-driven character definitions. Avoid character-specific gameplay code unless the mechanic is genuinely unique.

### Waves

Wave configuration must be reproducible and respect entity-pool limits.

Do not model progression only as universal HP inflation.

### RNG

Use deterministic pseudo-randomness.

Allow known seeds in debug/testing contexts and keep gameplay randomness reproducible.

### Password progression

Passwords represent persistent progression, not a full save state.

Do not implement password encoding before the relevant progression state is defined.

## Game states

Use explicit centralized game states/transitions.

Likely states include title/presentation, run initialization, active gameplay, level-up choice, transitions and game over; add future states only when required.

Level-up pause behavior must be explicit and deterministic.

## Performance discipline

Correctness before optimization.

For a suspected hot path:

1. establish a reproducible scenario;
2. inspect or measure;
3. identify the bottleneck;
4. make the smallest useful optimization;
5. compare before/after under the same scenario;
6. record the result.

Potential hot paths include enemy/projectile updates, collisions, targeting, OAM construction, XP attraction, spawning and NMI transfers.

Never report an optimization as faster without measurement or reliable generated-code evidence.

If not measured, say `Performance impact: not measured.`

## Collision

Prefer inexpensive shapes and algorithms.

- Respect inactive slots and pool limits.
- Document coordinate conventions.
- Avoid unnecessary all-pairs checks as counts rise.
- Add spatial partitioning only when measurement justifies its complexity.

## Resource budgets

Significant systems must account for resource impact.

Track, when available:

- Zero Page;
- stack assumptions;
- RAM globals and pools;
- OAM shadow;
- temporary/update buffers;
- audio memory;
- PRG-ROM;
- CHR-ROM;
- remaining headroom.

Use linker/map output rather than source-level guesses when measuring compiled memory.

Use the `nes-budget` skill for consistent reporting.

## Validation

Compilation alone is not proof that a gameplay feature works.

Choose validation appropriate to the change:

- host-side tests for pure logic;
- compile-time assertions where appropriate;
- clean ROM build;
- linker/map inspection;
- emulator validation for runtime behavior;
- debug instrumentation when useful.

Pay special attention to boundaries, invalid IDs, pool saturation, arithmetic overflow and table bounds.

### Mesen

When runtime behavior changed, prefer running the ROM in Mesen and exercising the changed path.

Check relevant items such as:

- boot/state transitions;
- visible behavior;
- RAM/debugger state;
- NMI stability;
- sprite corruption/flicker;
- OAM priority;
- unintended PPU writes.

Never claim emulator validation if the ROM was not actually executed.

Use the `nes-runtime-check` skill for the checklist.

## Documentation

English is the canonical language for architecture/source documentation unless an existing file establishes otherwise.

Update only documentation affected by the task.

Do not leave stale examples or contradictory architecture statements.

### Implementation notes

Meaningful gameplay, architecture, performance, rendering, memory-management or NES-hardware changes must create or update a human-oriented note under:

`docs/implementation-notes/`

These notes are written in Brazilian Portuguese and bridge the implementation with study/review/video material.

When relevant include:

- original problem;
- chosen solution and execution flow;
- small excerpts from actual code;
- NES constraints/trade-offs;
- measured performance/resource cost, clearly distinguished from estimates;
- what to observe in Mesen;
- limitations/follow-ups.

Do not require an implementation note for trivial formatting, typo or cosmetic documentation changes.

Use the `implementation-note` skill to keep this workflow compact.

### Branch change logs

Every implementation branch maintains synchronized human-readable logs:

```text
docs/changes/en/<sanitized-branch-name>.md
docs/changes/pt-BR/<sanitized-branch-name>.md
```

Update them after meaningful implementation steps, not after formatting noise.

The final logs describe the final branch state and include, when applicable:

- date/title;
- what changed and why;
- NES constraint/design consideration;
- affected files;
- small representative code excerpt;
- performance/resource impact;
- exact validation performed;
- limitations/follow-up.

Never invent benchmark/resource values.

Do not paste large diffs.

## Scope control

Do not:

- add scrolling without an explicit milestone;
- migrate away from NROM without an explicit decision;
- implement unrelated engine systems;
- add speculative abstractions;
- refactor the repository while solving a small local task;
- add dependencies without justification;
- hide gameplay limits inside implementation files;
- silently change established controls/tuning;
- expand scope because optional improvements were discovered.

Record optional improvements as follow-up work.

## Build quality

Before declaring implementation complete:

1. clean build;
2. relevant tests;
3. warning review;
4. linker/resource inspection when relevant;
5. runtime validation when behavior changed;
6. required docs/log updates;
7. final diff/scope review.

Do not commit generated binaries unless repository policy explicitly requires them.

Prefer the `nes-build-check` skill for the repeatable portion.

## Definition of done

A task is done only when:

- requested behavior is implemented;
- ROM builds;
- relevant tests pass;
- runtime behavior is validated when applicable;
- NES limits remain respected;
- required documentation is current;
- working tree remains in scope;
- limitations and unverified items are stated clearly.
