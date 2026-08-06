; NROM startup and interrupt vectors.
;
; Reset owns CPU/PPU initialization, clears internal RAM, initializes the
; cc65 software stack and runtime, prepares hidden OAM, then calls C main.

.export __STARTUP__ : absolute = 1
.export reset

.import _main
.import _oam_shadow
.importzp _nes_frame_counter
.import copydata, initlib, donelib
.import nmi_handler
.importzp c_sp
.import __STACK_START__, __STACK_SIZE__

PPUCTRL   = $2000
PPUMASK   = $2001
PPUSTATUS = $2002
OAMADDR   = $2003
PPUSCROLL = $2005
PPUADDR   = $2006
PPUDATA   = $2007
APUSTATUS = $4015
JOY2      = $4017
DMCFREQ   = $4010

.segment "HEADER"
    .byte "NES", $1A
    .byte 2                  ; 2 x 16 KiB PRG-ROM (NROM-256)
    .byte 1                  ; 1 x 8 KiB CHR-ROM
    .byte $00                ; mapper 0, horizontal mirroring, no trainer
    .byte $00                ; mapper 0, iNES 1.0
    .res 8, $00

.segment "STARTUP"

.proc reset
    sei
    cld

    ldx #$40
    stx JOY2                 ; disable APU frame IRQ
    ldx #$FF
    txs
    inx                      ; X = 0
    stx PPUCTRL
    stx PPUMASK
    stx DMCFREQ              ; disable DMC IRQ
    stx APUSTATUS            ; silence all APU channels

    bit PPUSTATUS
@first_vblank:
    bit PPUSTATUS
    bpl @first_vblank

    lda #$00
@clear_ram:
    sta $0000,x
    sta $0100,x
    sta $0200,x
    sta $0300,x
    sta $0400,x
    sta $0500,x
    sta $0600,x
    sta $0700,x
    inx
    bne @clear_ram

@second_vblank:
    bit PPUSTATUS
    bpl @second_vblank

    lda #<(__STACK_START__ + __STACK_SIZE__)
    sta c_sp
    lda #>(__STACK_START__ + __STACK_SIZE__)
    sta c_sp+1

    jsr initialize_ppu_memory

    lda #$FF
    ldx #$00
@hide_oam:
    sta _oam_shadow,x
    inx
    bne @hide_oam

    lda #$00
    sta OAMADDR
    sta _nes_frame_counter

    jsr copydata
    jsr initlib

    lda #$00
    sta PPUSCROLL
    sta PPUSCROLL
    lda #%10000000           ; enable NMI, base nametable $2000
    sta PPUCTRL
    lda #%00011110           ; enable background and sprites safely
    sta PPUMASK

    jsr _main
    jsr donelib

@halt:
    jmp @halt
.endproc

.proc initialize_ppu_memory
    lda PPUSTATUS
    lda #$20
    sta PPUADDR
    lda #$00
    sta PPUADDR

    ldx #$10                 ; clear $2000-$2FFF while rendering is disabled
    ldy #$00
    lda #$00
@clear_nametable_byte:
    sta PPUDATA
    iny
    bne @clear_nametable_byte
    dex
    bne @clear_nametable_byte

    lda PPUSTATUS
    lda #$3F
    sta PPUADDR
    lda #$00
    sta PPUADDR
    ldx #$20
    lda #$0F                 ; NES black for every palette entry
@clear_palette:
    sta PPUDATA
    dex
    bne @clear_palette
    rts
.endproc

.proc irq_handler
    rti
.endproc

.segment "VECTORS"
    .addr nmi_handler
    .addr reset
    .addr irq_handler
