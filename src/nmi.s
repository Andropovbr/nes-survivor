; Stable, bounded NMI handler.
;
; Responsibility: preserve registers, upload the complete OAM shadow page,
; apply an optional fixed 11-tile title-prompt update, restore zero scroll,
; advance the frame counter, and return. Gameplay never runs here. Baseline
; cost is approximately 590 CPU cycles including the pending check; the bounded
; show path is approximately 788 cycles, primarily the 513/514-cycle OAM DMA.

.export nmi_handler
.import _oam_shadow
.import _screen_title_prompt_text
.import _screen_title_prompt_update
.importzp _nes_frame_counter
.import _screen_level_up_cursor_update
.import _vram_buffer_len
.import _vram_buffer_addr_hi
.import _vram_buffer_addr_lo
.import _vram_buffer_data

OAMADDR   = $2003
PPUSTATUS = $2002
PPUSCROLL = $2005
PPUADDR   = $2006
PPUDATA   = $2007
OAMDMA    = $4014

TITLE_PROMPT_ADDRESS = $220A ; nametable $2000, row 16, column 10
TITLE_PROMPT_LENGTH  = 11
TITLE_PROMPT_SHOW    = 1

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

    lda _screen_title_prompt_update
    beq @check_vram_buffer

    ; Reading status resets the shared $2005/$2006 write latch. The following
    ; bounded transfer is wholly inside VBlank and scroll is restored below.
    lda PPUSTATUS
    lda #>TITLE_PROMPT_ADDRESS
    sta PPUADDR
    lda #<TITLE_PROMPT_ADDRESS
    sta PPUADDR

    lda _screen_title_prompt_update
    cmp #TITLE_PROMPT_SHOW
    bne @hide_prompt

    ldx #$00
@show_prompt:
    lda _screen_title_prompt_text,x
    sta PPUDATA
    inx
    cpx #TITLE_PROMPT_LENGTH
    bne @show_prompt
    jmp @finish_prompt_update

@hide_prompt:
    ldx #TITLE_PROMPT_LENGTH
    lda #$00
@hide_prompt_tile:
    sta PPUDATA
    dex
    bne @hide_prompt_tile

@finish_prompt_update:
    lda #$00
    sta _screen_title_prompt_update

@check_vram_buffer:
    lda _vram_buffer_len
    beq @check_cursor_update
    lda PPUSTATUS
    lda _vram_buffer_addr_hi
    sta PPUADDR
    lda _vram_buffer_addr_lo
    sta PPUADDR
    ldx #$00
@vram_copy:
    lda _vram_buffer_data,x
    sta PPUDATA
    inx
    cpx _vram_buffer_len
    bne @vram_copy
    lda #$00
    sta _vram_buffer_len

@check_cursor_update:
    lda _screen_level_up_cursor_update
    beq @restore_scroll
    sec
    sbc #$01
    tay

    lda PPUSTATUS
    lda #$21
    sta PPUADDR
    lda #$AA
    sta PPUADDR
    ldx #$20
    cpy #$00
    bne :+
    ldx #$3E
:   stx PPUDATA

    lda #$21
    sta PPUADDR
    lda #$CA
    sta PPUADDR
    ldx #$20
    cpy #$01
    bne :+
    ldx #$3E
:   stx PPUDATA

    lda #$21
    sta PPUADDR
    lda #$EA
    sta PPUADDR
    ldx #$20
    cpy #$02
    bne :+
    ldx #$3E
:   stx PPUDATA

    lda #$00
    sta _screen_level_up_cursor_update

@restore_scroll:
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
