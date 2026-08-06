; Stable, bounded NMI handler.
;
; Responsibility: preserve registers, upload the complete OAM shadow page,
; restore zero scroll, advance the frame counter, and return. Gameplay never
; runs here. Worst-case cost is approximately 583 CPU cycles including the
; interrupt entry, primarily the 513/514-cycle OAM DMA.

.export nmi_handler
.import _oam_shadow
.importzp _nes_frame_counter

OAMADDR   = $2003
PPUSCROLL = $2005
OAMDMA    = $4014

.segment "CODE"

.proc nmi_handler
    pha
    txa
    pha
    tya
    pha

    lda #$00
    sta OAMADDR
    lda #>_oam_shadow
    sta OAMDMA

    lda #$00
    sta PPUSCROLL
    sta PPUSCROLL

    inc _nes_frame_counter

    pla
    tay
    pla
    tax
    pla
    rti
.endproc
