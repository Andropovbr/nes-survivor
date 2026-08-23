# Branch development log: add-state-machine

## 2026-08-23 — Moved title blink VRAM writes into NMI

### What changed

The periodic `Press Start` update no longer disables and re-enables rendering
from C. `screen.c` publishes one show/hide byte, and the next NMI writes exactly
11 tiles at `$220A`, clears the request and restores zero scroll. PPUCTRL masks,
CHR layout assertions and Mesen checks now explicitly enforce sprite pattern
table 0 and background pattern table 1.

### Why

The old C path assumed that code awakened after NMI would finish before VBlank
ended. When it re-enabled rendering after changing `$2006/$2007`, the PPU could
resume during a visible scanline with a transient internal VRAM address. This
caused a brief fragment of the prompt at the wrong position. A fixed VBlank
transfer corrects the timing and latch cause instead of masking the artifact.

### Relevant code

```c
if (visible != 0U) {
    screen_title_prompt_update = TITLE_PROMPT_UPDATE_SHOW;
} else {
    screen_title_prompt_update = TITLE_PROMPT_UPDATE_HIDE;
}
```

```asm
lda PPUSTATUS
lda #>TITLE_PROMPT_ADDRESS
sta PPUADDR
lda #<TITLE_PROMPT_ADDRESS
sta PPUADDR
```

The C assignment is atomic on 6502. NMI owns the complete PPU transfer, reads
status to reset the shared `$2005/$2006` latch and clears the request only after
all 11 bytes are written.

### NES considerations

`PPUCTRL=$90`: bit 7 enables NMI, bit 4 selects background pattern table 1 at
`$1000`, and clear bit 3 selects sprite pattern table 0 at `$0000`. Nametable
entries remain 8-bit indexes. `chr.s` asserts that both physical CHR halves are
exactly 4 KiB. The optional transfer is fixed-size; no generic VRAM command
buffer was introduced.

### Performance

Generated-instruction estimate:

- normal NMI: approximately 590 cycles, previously approximately 583;
- prompt show NMI: approximately 788 cycles total;
- NTSC VBlank budget: approximately 2,273 cycles.

The 1,750-frame Mesen stress test reached all 12 Bats with 1,735 NMIs and 1,735
gameplay updates after the transition baseline: zero skipped gameplay updates.

### Resource impact

- PRG-ROM: 7,099 -> 7,043 bytes (`-56`).
- BSS: 80 -> 81 bytes (`+1`) for the pending update mode.
- Zero page, DATA, OAM, hardware sprites and CHR size/content: unchanged.
- The font remains physically in `CHR $1000-$1FFF`; assembly assertions were
  added without changing bytes.

### Main files affected

`src/screen.c`, `src/nmi.s`, `src/crt0.s`, `src/chr.s`, `include/nes.h`,
`tests/test_logic.c`, `tests/mesen_game_states.lua`, architecture/memory docs and
`docs/implementation-notes/game-states.md`.

### Validation

- clean ROM build: PASS
- sim65 logic and compile-time PPUCTRL assertions: PASS
- structural ROM/CHR/map validation: PASS
- Mesen state runtime, 175 frames and several blink phases: PASS
- no partial prompt state in sampled nametable: PASS
- no `$2000/$2001` write during blink: PASS
- observed `PPUCTRL=$90` and glyph `P` in pattern table 1: PASS
- Mesen player/sword/Bat runtime, 450 frames: PASS
- Mesen 12-Bat stress, 1,750 frames: PASS, zero gameplay skips

### Limitations / follow-up

Headless Mesen validates PPU memory and register writes but cannot replace a
human inspection of the rendered frame. Manually observe several blink cycles
and confirm the two CHR halves in PPU Viewer before merge. Full-screen
transitions still intentionally disable rendering; only the periodic small
update moved into NMI.
