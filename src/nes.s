; Small C-callable hardware interface. ABI details are repeated in nes.h.

.export _nes_wait_frame
.export _nes_read_controller
.export _oam_shadow
.exportzp _nes_frame_counter

JOY1 = $4016

.segment "ZEROPAGE"
_nes_frame_counter: .res 1
controller_bits:    .res 1

.segment "OAM"
_oam_shadow: .res 256
.assert <_oam_shadow = $00, lderror, "OAM shadow must be page-aligned"

.segment "CODE"

; void nes_wait_frame(void)
; Input/return: none. Clobbers A and flags. Uses no ZP temporaries.
; NMI must be enabled; main-thread only and not reentrant.
.proc _nes_wait_frame
    lda _nes_frame_counter
@wait:
    cmp _nes_frame_counter
    beq @wait
    rts
.endproc

; uint8_t nes_read_controller(void)
; Input: none. Returns A/B/Select/Start/Up/Down/Left/Right as bits 7..0.
; Clobbers A, X, flags, and controller_bits in ZP; preserves Y.
; Main-thread only. DMC DMA must be disabled to avoid read corruption.
.proc _nes_read_controller
    lda #$01
    sta JOY1
    lda #$00
    sta JOY1
    sta controller_bits

    ldx #$08
@read_bit:
    lda JOY1
    and #$01
    cmp #$01                 ; copy the serial bit into carry
    rol controller_bits      ; first bit (A) eventually reaches bit 7
    dex
    bne @read_bit

    lda controller_bits
    rts
.endproc
