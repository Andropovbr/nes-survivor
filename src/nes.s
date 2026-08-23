; Small C-callable hardware interface. ABI details are repeated in nes.h.

.export _nes_wait_frame
.export _nes_read_controller
.export _nes_play_player_hit_sfx
.export _oam_shadow
.exportzp _nes_frame_counter

JOY1 = $4016
APUSTATUS = $4015
NOISE_VOLUME = $400C
NOISE_PERIOD = $400E
NOISE_LENGTH = $400F

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

; void nes_play_player_hit_sfx(void)
; Input/return: none. Clobbers A and flags. Uses no RAM/ZP.
; Main-thread only. A short hardware length counter bounds the effect.
.proc _nes_play_player_hit_sfx
    lda #%00001000          ; enable only the currently owned noise channel
    sta APUSTATUS
    lda #%00011100          ; constant-volume impact at volume 12
    sta NOISE_VOLUME
    lda #%00000100          ; short-mode off, bright low-period noise
    sta NOISE_PERIOD
    lda #%00010000          ; length index 2: about ten NTSC video frames
    sta NOISE_LENGTH
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
