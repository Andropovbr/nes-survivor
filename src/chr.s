.segment "CHARS"

; The existing sprite pattern table occupies $0000-$0FFF. The background
; pattern table at $1000 stores only the ASCII glyphs used by the initial
; screens; unused tiles remain zero-filled by the linker.
sprite_pattern_table_start:
.incbin "game.chr", 0, $1000
.assert * - sprite_pattern_table_start = $1000, error, "sprite pattern table must occupy CHR $0000-$0FFF"

background_pattern_table_start:

.macro font_tile row0, row1, row2, row3, row4, row5, row6, row7
    .byte row0, row1, row2, row3, row4, row5, row6, row7
    .res 8, $00
.endmacro

.res 32 * 16, $00           ; ASCII $00-$1F
font_tile $00,$00,$00,$00,$00,$00,$00,$00 ; space ($20)
.res 34 * 16, $00           ; ASCII $21-$42
font_tile $38,$44,$40,$40,$40,$44,$38,$00 ; C ($43)
.res 1 * 16, $00            ; D
font_tile $7C,$40,$40,$78,$40,$40,$7C,$00 ; E ($45)
.res 8 * 16, $00            ; F-M
font_tile $44,$64,$54,$4C,$44,$44,$44,$00 ; N ($4E)
.res 1 * 16, $00            ; O
font_tile $78,$44,$44,$78,$40,$40,$40,$00 ; P ($50)
.res 2 * 16, $00            ; Q-R
font_tile $3C,$40,$40,$38,$04,$04,$78,$00 ; S ($53)
.res 13 * 16, $00           ; T-`
font_tile $00,$00,$38,$04,$3C,$44,$3C,$00 ; a ($61)
font_tile $40,$40,$78,$44,$44,$44,$78,$00 ; b ($62)
font_tile $00,$00,$38,$40,$40,$40,$38,$00 ; c ($63)
font_tile $04,$04,$3C,$44,$44,$44,$3C,$00 ; d ($64)
font_tile $00,$00,$38,$44,$7C,$40,$38,$00 ; e ($65)
.res 1 * 16, $00            ; f
font_tile $00,$00,$3C,$44,$3C,$04,$38,$00 ; g ($67)
font_tile $40,$40,$78,$44,$44,$44,$44,$00 ; h ($68)
font_tile $10,$00,$30,$10,$10,$10,$38,$00 ; i ($69)
.res 4 * 16, $00            ; j-m
font_tile $00,$00,$78,$44,$44,$44,$44,$00 ; n ($6E)
font_tile $00,$00,$38,$44,$44,$44,$38,$00 ; o ($6F)
.res 2 * 16, $00            ; p-q
font_tile $00,$00,$58,$60,$40,$40,$40,$00 ; r ($72)
font_tile $00,$00,$3C,$40,$38,$04,$78,$00 ; s ($73)
font_tile $20,$20,$78,$20,$20,$24,$18,$00 ; t ($74)
font_tile $00,$00,$44,$44,$44,$4C,$34,$00 ; u ($75)
font_tile $00,$00,$44,$44,$44,$28,$10,$00 ; v ($76)
.res 2 * 16, $00            ; w-x
font_tile $00,$00,$44,$44,$3C,$04,$38,$00 ; y ($79)
.res (256 - 122) * 16, $00  ; remaining background tiles
.assert * - background_pattern_table_start = $1000, error, "background pattern table must occupy CHR $1000-$1FFF"
