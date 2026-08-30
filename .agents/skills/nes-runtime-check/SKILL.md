---
name: nes-runtime-check
description: Guide and record focused Mesen runtime validation for NES Survivor changes. Use when gameplay, rendering, input, state transitions, NMI/PPU or sprite behavior changed.
---

This skill records actual emulator validation; it must never fabricate execution.

1. Identify the exact changed behavior and minimal reproduction path.
2. Build the current ROM.
3. If Mesen can be run in the current environment, execute the ROM and exercise the path.
4. Observe only relevant checks:
   - boot and state transitions;
   - controls/gameplay behavior;
   - sprite corruption/flicker/OAM priority;
   - PPU writes/rendering;
   - NMI/frame stability;
   - relevant RAM/debugger state;
   - boundary or saturation scenario requested by the task.
5. If emulator execution is unavailable, do not simulate it. Return `Mesen runtime validation: NOT PERFORMED` and explain the concrete limitation.
6. Do not call a successful ROM build runtime validation.

Return:
- ROM/build used;
- scenario exercised;
- observed result;
- debugger/evidence used, if any;
- Mesen validation: PASS/FAIL/NOT PERFORMED;
- follow-up needed.
