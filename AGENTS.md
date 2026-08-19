# AGENTS.md

## Project purpose

This repository contains a real NES game that combines:

- the fixed-arena action of classic games such as Robotron: 2084;
- automatic weapons;
- progressive enemy waves;
- XP collection;
- level-up upgrade choices;
- character and weapon progression associated with modern survivor-like games.

The initial target is **NROM / Mapper 0**, one arena, and no scrolling.

The game is primarily written in **C**, with **6502 Assembly used only where it provides a clear, justified benefit**.

## Core design principles

1. Build a fun, responsive game before expanding content.
2. Respect actual NES hardware limitations.
3. Prefer small, reviewable milestones.
4. Keep gameplay systems modular and data-driven.
5. Keep balance values easy to tune.
6. Avoid premature optimization.
7. Measure before moving C code into Assembly.
8. Preserve buildability and runtime correctness after every change.
9. Do not silently expand scope.
10. Document architecture, constraints, tests, and memory use continuously.

## Current product constraints

Unless a milestone explicitly changes them:

- Target mapper: NROM / Mapper 0.
- Arena count: one.
- Scrolling: none.
- Player controls: directional pad; no dual-stick design.
- Weapons: automatic firing.
- Maximum equipped weapons during a run: configurable, initially four.
- Level-up choices: configurable, initially three.
- Enemies drop XP.
- XP entities must use a fixed-size pool and a condensation strategy.
- Enemy waves become progressively harder.
- Characters may have different starting weapons and attribute modifiers.
- New characters may be unlocked through gameplay objectives.
- Long-term progression is expected to use a password system.

## Toolchain

Use the repository's existing toolchain.

For a new repository, prefer:

- cc65 for C compilation;
- ca65 for Assembly;
- ld65 for linking;
- an NROM-compatible linker configuration;
- Mesen for runtime debugging and validation.

Do not replace the toolchain without documenting the reason and migration impact.

## Source organization

Keep modules focused. Avoid large files that combine unrelated systems.

Expected long-term boundaries include:

- game state and run lifecycle;
- player;
- input;
- PPU and rendering;
- NMI and frame synchronization;
- OAM handling;
- enemies;
- weapons;
- projectiles;
- XP gems and condensation;
- wave generation;
- upgrades and rarity;
- character definitions;
- unlock objectives;
- password encoding;
- deterministic RNG;
- tuning and limits.

Do not create empty placeholder modules merely to match an intended architecture. Add modules when a milestone needs them.

## Header responsibilities

Headers should expose the smallest useful interface.

- Do not expose internal mutable state without a reason.
- Avoid circular includes.
- Use forward declarations and shared type headers when appropriate.
- Put compile-time gameplay limits and balance constants in `tuning.h` or a clearly named related file.
- Put hardware constants in hardware-specific headers, not in `tuning.h`.
- Avoid magic numbers in gameplay code.

## Tuning rules

Values expected to change during balancing must be centralized.

Examples:

- player base attributes;
- character modifiers;
- weapon damage;
- weapon cooldown;
- projectile speed;
- enemy HP;
- enemy speed;
- spawn cadence;
- wave composition;
- XP values;
- XP required per level;
- XP growth per level;
- maximum active enemies;
- maximum projectiles;
- maximum XP gems;
- XP merge distance or region size;
- upgrade rarity weights;
- invulnerability time;
- pickup radius;
- arena boundaries.

Use clear names and comments with units.

Examples:

```c
#define MAX_ACTIVE_ENEMIES       12
#define MAX_ACTIVE_PROJECTILES   16
#define MAX_ACTIVE_XP_GEMS        8
#define LEVEL_UP_CHOICE_COUNT      3
#define MAX_EQUIPPED_WEAPONS       4
#define PLAYER_INVULN_FRAMES      60
```

These values are examples, not permanent decisions.

## Data-oriented design

Use fixed-size pools. Never use heap allocation.

Prefer arrays of compact fields or compact structs according to measured code quality and performance.

For every important runtime pool, document:

- maximum element count;
- bytes per element;
- total RAM cost;
- inactive-slot representation;
- allocation strategy;
- update strategy;
- rendering strategy;
- behavior when the pool is full.

Keep static definitions separate from runtime state.

Examples:

- `WeaponDefinition` describes immutable weapon properties.
- `WeaponRuntime` stores cooldown, level, and temporary state.
- `CharacterDefinition` describes base attributes and starting weapon.
- `EnemyDefinition` describes type defaults.
- enemy instances store only per-instance state.

Avoid unnecessary pointers. IDs and array indexes are usually preferable on the NES.

## Numeric conventions

- Use `<stdint.h>` integer types.
- Avoid `int` where its width or cost is ambiguous.
- Avoid floating point.
- Document fixed-point formats.
- Use one consistent convention for percentage modifiers.
- Prevent accidental overflow.
- When overflow is intentional, comment it.
- Use saturating arithmetic where exceeding a cap would create bugs.
- Keep expensive division and multiplication out of hot paths when tables, shifts, or incremental calculations are clearer.

## C and Assembly boundary

C is the default language for gameplay systems.

Assembly is appropriate for routines such as:

- NMI entry and exit;
- OAM DMA;
- controller reads;
- optimized memory copy or clear;
- carefully measured collision or entity loops;
- fixed-point helpers;
- other routines proven to be performance-critical.

Every handwritten Assembly routine must document:

- purpose;
- why C was insufficient or undesirable;
- calling convention;
- input parameters;
- output values;
- registers clobbered;
- zero-page variables used;
- reentrancy or interrupt assumptions;
- expected cycle cost when relevant.

Do not move a routine into Assembly only because it runs every frame. First inspect generated code or measure runtime behavior.

Assembly interfaces should be small and stable.

## Frame and PPU safety

- Synchronize the main loop with NMI.
- Keep PPU writes inside valid rendering-disabled or VBlank periods.
- Use an OAM shadow buffer.
- Do not perform uncontrolled PPU writes from arbitrary gameplay modules.
- Track the VBlank workload.
- Document NMI work and estimated cycle costs.
- Preserve registers correctly in interrupt handlers.
- Avoid long or unbounded work inside NMI.
- Gameplay updates normally run outside NMI.
- NMI should perform bounded hardware transfer work and frame signaling.

## Sprite policy

The NES supports:

- 64 hardware sprites total;
- 8 hardware sprites per scanline.

Therefore:

- gameplay entity count and rendered sprite count must not be assumed to be identical;
- use fixed rendering budgets;
- define sprite priorities;
- allow nonessential effects to be skipped;
- prefer flicker management over uncontrolled disappearance;
- do not let XP gems permanently starve the player, enemies, or important projectiles from OAM;
- keep the player and dangerous threats at higher rendering priority than cosmetic effects;
- document multi-sprite entities and their worst-case scanline cost.

When a sprite budget is exceeded, behavior must be deterministic and documented.

## XP gem requirements

XP must never be silently lost because the gem pool is full.

Use a configurable fixed-size gem pool.

Supported condensation approaches include:

- merging XP into the nearest active gem;
- limiting gems by arena region;
- accumulating pending XP before spawning a gem;
- upgrading an existing gem's value;
- combining multiple strategies.

The selected strategy must document:

- maximum active gems;
- RAM cost;
- sprite cost;
- merge behavior;
- collection behavior;
- what happens when the pool is full;
- whether remaining gems are collected at wave end.

XP gem behavior must be tested under pool saturation.

## Weapon rules

The player may eventually hold multiple automatic weapons.

Weapon code should support:

- a static weapon definition;
- per-run weapon level;
- cooldown;
- targeting policy;
- projectile or effect behavior;
- upgrade-driven behavior changes;
- configurable limits.

Do not hardcode a separate update loop for every weapon when a shared mechanism is practical. Specialized behavior is allowed when it improves clarity.

Weapons should be designed with NES budgets in mind. Not every weapon should require many independent projectile sprites.

Consider economical weapon behaviors such as:

- orbiting objects;
- piercing shots;
- beams;
- area pulses;
- short-lived explosions;
- boomerangs;
- chained effects;
- direct damage without a persistent projectile.

## Upgrade rules

Upgrades have rarity and eligibility.

The level-up system is expected to offer three choices, subject to tuning.

Upgrade selection must eventually consider:

- rarity weights;
- current weapons;
- weapon level caps;
- character restrictions;
- mutually exclusive upgrades;
- duplicate prevention within one choice set;
- deterministic RNG for testing;
- fallback behavior when fewer than three normal choices are eligible.

Separate upgrade presentation from upgrade application.

Upgrades may:

- modify player attributes;
- modify weapon numeric values;
- add weapon behavior flags;
- unlock a new weapon;
- evolve or transform a weapon;
- affect XP collection or defense.

## Character rules

Character definitions should be table-driven.

Each character may define:

- starting weapon;
- base HP;
- movement speed modifier;
- armor;
- damage or power modifier;
- cooldown modifier;
- pickup radius modifier;
- unlock condition ID;
- visual or palette identifiers.

Avoid duplicated character-specific gameplay code unless the character has a genuinely unique mechanic.

## Wave rules

Wave difficulty should be data-driven and reproducible.

Wave configuration may control:

- enemy type weights;
- maximum simultaneous enemies;
- spawn interval;
- total enemy count;
- enemy stat scaling;
- obstacles;
- elite enemies;
- events;
- bosses or minibosses.

Avoid implementing progression only as a universal HP increase. Add behavioral and composition variety.

Wave generation must respect active entity limits. When the enemy pool is full, spawning should be delayed or handled predictably rather than corrupting memory.

## RNG rules

Use a deterministic pseudo-random number generator.

- Allow a known seed in debug builds.
- Keep RNG use explicit.
- Avoid coupling cosmetic randomness with gameplay randomness if it prevents reproducible tests.
- Document whether RNG state is included in passwords or run state.
- Tests involving upgrades, waves, or drops should use fixed seeds.

## Password rules

The password system is persistent progression, not a full save state.

Expected password data may include:

- unlocked characters;
- unlocked weapons or upgrades;
- completed objectives;
- highest wave or milestones;
- version and checksum information.

Do not include unnecessary per-run transient state.

Password encoding must detect invalid input and version incompatibility. Do not implement it until the relevant progression state is defined.

## Game-state architecture

Use explicit game states, such as:

- boot;
- title;
- character selection;
- run initialization;
- active wave;
- level-up choice;
- wave transition;
- game over;
- results;
- password entry.

Do not scatter state transitions across unrelated modules.

Pausing gameplay for a level-up choice should be explicit and deterministic.

## Performance discipline

Correctness comes first, but performance budgets must be visible.

For code that may become expensive:

1. Keep the first implementation simple.
2. Inspect generated Assembly when useful.
3. Measure frame time or estimate cycles.
4. Identify the actual bottleneck.
5. Optimize only the bottleneck.
6. Add regression tests or documentation.

Potential hot paths include:

- enemy updates;
- projectile updates;
- collision checks;
- target selection;
- OAM construction;
- XP attraction;
- spawning;
- NMI transfers.

Use techniques such as staggered updates only when needed and document their gameplay impact.

## Collision rules

Prefer simple collision shapes.

- Use bounding boxes, points, or circles approximated with inexpensive math.
- Avoid checking every object against every other object when counts grow.
- Use categories and directional queries.
- Consider spatial regions only if measurement justifies the complexity.
- Collision code must respect inactive slots and pool limits.
- Document whether coordinates represent sprite origin, center, or collision origin.

## Memory budgeting

Maintain a current RAM and ROM budget.

Documentation must include:

- zero-page usage;
- stack assumptions;
- OAM shadow;
- global state;
- each entity pool;
- temporary buffers;
- nametable or update buffers;
- audio engine memory;
- remaining headroom;
- PRG usage;
- CHR usage.

Any milestone that adds a significant system must update the budget.

Do not rely only on source-level estimates. Inspect linker map output.

## Testing requirements

Every feature must include validation appropriate to its level.

Use:

- host-side tests for pure logic;
- compile-time assertions where possible;
- ROM build tests;
- linker-map checks;
- emulator runtime tests;
- debug instrumentation when useful.

Always test:

- normal behavior;
- boundary values;
- pool saturation;
- invalid IDs;
- maximum level;
- maximum and minimum attributes;
- arithmetic overflow risks;
- empty eligible-upgrade sets;
- duplicate upgrade choices;
- character and weapon table bounds;
- XP gem saturation;
- enemy and projectile pool saturation.

A successful compilation alone is not sufficient evidence that a gameplay feature works.

## Emulator validation

Prefer Mesen for runtime validation.

When a change affects runtime behavior:

- boot the ROM;
- exercise the changed feature;
- inspect relevant RAM or debugger state when needed;
- verify that NMI remains stable;
- check for sprite corruption;
- check for unintended PPU writes;
- observe scanline flicker and OAM priority;
- report exactly what was validated.

Do not claim emulator validation if the ROM was not actually run.

## Documentation requirements

Documentation is part of every milestone.

Update relevant files when behavior changes:

- `README.md`;
- architecture documentation;
- memory map and budgets;
- build instructions;
- tuning documentation;
- gameplay-system documentation;
- known limitations;
- roadmap or milestone status.

Keep documentation in English unless the repository explicitly establishes another language.

Do not leave stale examples or contradictory plans.

## Code comments

Comments should explain:

- why a decision exists;
- NES constraints;
- invariants;
- non-obvious arithmetic;
- pool-full behavior;
- interrupt assumptions;
- Assembly calling conventions;
- deliberately simplified behavior.

Do not comment every obvious assignment.

## Error handling and assertions

The release ROM may need lightweight handling, but debug builds should fail loudly where possible.

Use assertions, debug colors, emulator breakpoints, logging hooks, or known memory markers to detect:

- invalid IDs;
- pool overflow;
- impossible state transitions;
- table index errors;
- invalid upgrade combinations;
- NMI queue overflow;
- unsupported password versions.

Never allow an out-of-bounds write as a normal failure mode.

## Branch change log requirements

Every implementation branch must maintain a human-readable development log under `docs/changes/`.

This log is part of the implementation itself and must be updated whenever a meaningful code change is made.

The goal is to preserve the reasoning behind changes, not merely duplicate the Git diff.

### Directory structure

Use:

```text
docs/
  changes/
    en/
    pt-BR/
```

For each branch, maintain one English file and one Brazilian Portuguese file.

Recommended naming:

```text
docs/changes/en/<branch-name>.md
docs/changes/pt-BR/<branch-name>.md
```

Sanitize branch names when necessary so `/` does not create unintended nested directories.

Example:

```text
feature/enemy-collision
```

may become:

```text
docs/changes/en/feature-enemy-collision.md
docs/changes/pt-BR/feature-enemy-collision.md
```

### When to update the log

Update the branch log after every meaningful implementation step that would normally justify a commit or push.

Do not create noise for:

* whitespace-only changes;
* formatting-only changes;
* temporary debug edits that are removed before completion;
* generated files;
* mechanical changes with no technical relevance.

A pushed branch must never contain meaningful source changes without a corresponding log update.

### Required content

Each entry must contain:

1. Date.
2. Short title.
3. What changed.
4. Why the change was needed.
5. Relevant NES constraint or design consideration.
6. Main files affected.
7. Relevant code excerpt.
8. Explanation of the code excerpt.
9. Performance impact, when applicable.
10. RAM / Zero Page / PRG / CHR impact, when applicable.
11. Tests and validation performed.
12. Known limitations or follow-up work.

### Code excerpts

Include the smallest useful excerpt that explains the implementation.

Do not dump full files or large diffs.

Prefer focused examples such as:

```c
for (i = 0; i < MAX_ENEMIES; ++i) {
    if (!enemy_active[i]) {
        continue;
    }

    update_enemy(i);
}
```

Then explain:

* what the code does;
* why this approach was chosen;
* what NES-specific trade-off exists;
* whether it affects CPU, RAM, OAM, CHR, NMI, or VBlank budgets.

For Assembly, also mention relevant registers, memory use, and cycle implications when meaningful.

### Before / after excerpts

When a refactor changes an important algorithm or optimization strategy, prefer showing both the relevant old and new forms.

Example:

```text
Before:
<small old excerpt>

After:
<small new excerpt>
```

Then explain the practical effect.

Do not reproduce large Git diffs.

### Performance documentation

For changes touching a hot path, include measured results whenever reliable measurements are available.

Examples:

```text
Scenario: 16 active enemies

Before:
Main loop: 25,420 cycles

After:
Main loop: 20,180 cycles

Difference:
-5,240 cycles (-20.6%)
```

Never invent measurements.

If performance was not measured, explicitly state:

```text
Performance impact: not measured.
```

Do not describe an optimization as faster unless measurement or reliable generated-code analysis supports the claim.

### Resource impact

Record resource changes when they are relevant.

Examples:

```text
PRG-ROM: +94 bytes
CHR-ROM: unchanged
RAM: -8 bytes
Zero Page: unchanged
Hardware sprites: unchanged
```

For graphical changes, include useful tile information when available:

```text
CHR tiles before: 44
CHR tiles after: 8
Tiles saved: 36
```

### Testing and validation

Each entry must document exactly what was run.

Example:

```text
Validation:

- clean ROM build: PASS
- host-side tests: PASS
- CHR validation: PASS
- performance benchmark: PASS
- Mesen runtime validation: PASS
```

Do not claim emulator testing unless the ROM was actually executed in the emulator.

### English and Portuguese synchronization

The English and Brazilian Portuguese logs must describe the same technical changes.

They do not need to be literal translations, but neither version may omit important technical information present in the other.

English remains the canonical repository language for architecture and source documentation.

The Portuguese branch log exists primarily as a development diary and educational reference.

### Suggested entry format

English:

````markdown
## 2026-08-19 — Optimized enemy sprite construction

### What changed

...

### Why

...

### Relevant code

```c
...
````

### How it works

...

### NES considerations

...

### Performance

...

### Resource impact

...

### Validation

...

### Limitations / follow-up

...

````

Portuguese:

```markdown
## 2026-08-19 — Otimização da montagem dos sprites dos inimigos

### O que mudou

...

### Por que foi necessário

...

### Trecho relevante

```c
...
````

### Como funciona

...

### Considerações sobre o NES

...

### Desempenho

...

### Impacto em recursos

...

### Validação

...

### Limitações / próximos passos

...

```

### Relationship with Git history

The branch log complements Git history; it does not replace it.

Git answers:

> What lines changed?

The development log should answer:

> Why did they change, how does the solution work, and what effect did it have on the NES budgets?

Commit messages should remain concise.

Do not paste commit messages into the documentation as a substitute for technical explanation.

### Branch completion

Before declaring a branch complete:

1. Review the complete branch diff against its base branch.
2. Ensure every meaningful change is represented in the branch log.
3. Remove obsolete entries describing approaches that were later reverted.
4. Make sure English and Portuguese logs are synchronized.
5. Verify code excerpts still match the final implementation.
6. Add final benchmark and resource numbers when available.
7. Record tests and emulator validation.
8. Ensure no speculative or unverified claim remains in the documentation.

The final branch log must describe the final state of the branch, not merely the chronological sequence of experiments.

### CI validation

When practical, CI should verify that meaningful changes to source, assets, linker configuration, or build scripts are accompanied by updates under `docs/changes/`.

This check should initially be advisory rather than blocking if reliable detection cannot be implemented without excessive false positives.

Do not create empty or meaningless documentation changes merely to satisfy CI.
```


## Build quality

Before finishing any task:

1. Build from a clean state.
2. Run all relevant tests.
3. Review warnings.
4. Inspect linker and memory output.
5. Run the ROM when runtime behavior changed.
6. Update documentation.
7. Summarize known limitations.
8. Keep the working tree limited to the requested scope.

Do not commit generated binaries unless repository policy requires them.

## Scope control

Do not:

- introduce scrolling without an explicit milestone;
- migrate away from NROM without an explicit decision;
- implement unrelated engine features;
- add complex abstractions for hypothetical future games;
- refactor the whole repository while implementing a small feature;
- add dependencies without justification;
- hide gameplay limits inside implementation files;
- silently change established controls or tuning;
- expand a milestone after discovering optional improvements.

Record optional improvements as follow-up work.

## Milestone response format

Before implementation, report:

- repository assessment;
- assumptions;
- plan;
- expected files;
- technical budgets;
- major risks.

After implementation, report:

- summary;
- files changed;
- architecture decisions;
- build result;
- tests;
- emulator validation;
- memory and ROM usage;
- known limitations;
- recommended next milestone.

Be precise. Distinguish completed work from proposed work.

## Definition of done

A task is done only when:

- the requested behavior is implemented;
- the ROM builds;
- tests pass;
- relevant runtime behavior is validated;
- memory and hardware limits are still respected;
- documentation is current;
- no unrelated regressions are known;
- limitations and remaining work are stated honestly.
