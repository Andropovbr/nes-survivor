---
name: nes-budget
description: Extract and compare NES Survivor RAM, Zero Page, PRG-ROM and CHR-ROM usage from real build/linker artifacts. Use when a change affects memory/resources or before claiming a resource delta.
---

Never estimate compiled resource usage when build/linker evidence is available.

1. Locate the repository's linker configuration, map output and build artifacts.
2. Build first if current artifacts are missing/stale and the task permits it.
3. Extract, where available:
   - Zero Page usage/free space;
   - RAM/BSS/data usage;
   - stack assumptions if represented;
   - PRG-ROM used/free;
   - CHR-ROM used/free;
   - named pool/buffer sizes visible in symbols/map.
4. When asked for a branch delta, compare equivalent artifacts/scenarios from base and head. Do not compare unlike build modes.
5. Mark values that cannot be derived reliably as `not available`.
6. Never infer a byte delta from source code alone and present it as compiled fact.
7. Do not optimize; report evidence for the parent or performance specialist.

Return a compact table:
Resource | Current | Base (if available) | Delta | Evidence

Then list:
- notable new symbols/pools;
- limit/headroom concerns;
- unmeasured items.
