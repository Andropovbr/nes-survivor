---
name: nes-build-check
description: Run the NES Survivor repeatable build/test quality gate and return a compact factual report. Use after implementation or before PR completion; do not claim runtime validation.
---

Goal: verify the repository using existing tooling without redesigning or fixing unrelated code.

1. Read AGENTS.md and the repository build instructions/Makefile needed to identify canonical commands.
2. Start from a clean-build path when the repository provides one.
3. Run the relevant host-side tests.
4. Build the ROM.
5. Review compiler/linker warnings.
6. Inspect linker/map/resource output when produced.
7. Do not modify code merely to silence unrelated pre-existing warnings.
8. Do not run or claim Mesen validation unless the task separately invokes the runtime-check workflow.
9. If a command is unavailable on Windows, follow the AGENTS.md tool-availability checks before reporting it missing.
10. Stop on a meaningful failure and report the first actionable cause plus any dependent checks that could not run.

Return:
- clean build: PASS/FAIL;
- tests: PASS/FAIL/N/A;
- warnings: count or concise summary;
- linker/resource output: inspected/not available;
- ROM produced: yes/no;
- runtime validation: NOT PERFORMED by this skill;
- blockers.
