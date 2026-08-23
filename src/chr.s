.segment "CHARS"

; The existing sprite pattern table occupies $0000-$0FFF. The background
; pattern table at $1000 stores only the uppercase ASCII glyphs used by initial
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
.res 32 * 16, $00           ; ASCII $21-$40
font_tile $38,$44,$44,$7C,$44,$44,$44,$00 ; A ($41)
font_tile $78,$44,$44,$78,$44,$44,$78,$00 ; B ($42)
font_tile $38,$44,$40,$40,$40,$44,$38,$00 ; C ($43)
font_tile $78,$44,$44,$44,$44,$44,$78,$00 ; D ($44)
font_tile $7C,$40,$40,$78,$40,$40,$7C,$00 ; E ($45)
.res 1 * 16, $00            ; F
font_tile $38,$44,$40,$4C,$44,$44,$38,$00 ; G ($47)
font_tile $44,$44,$44,$7C,$44,$44,$44,$00 ; H ($48)
font_tile $38,$10,$10,$10,$10,$10,$38,$00 ; I ($49)
.res 3 * 16, $00            ; J-L
font_tile $44,$6C,$54,$54,$44,$44,$44,$00 ; M ($4D)
font_tile $44,$64,$54,$4C,$44,$44,$44,$00 ; N ($4E)
font_tile $38,$44,$44,$44,$44,$44,$38,$00 ; O ($4F)
font_tile $78,$44,$44,$78,$40,$40,$40,$00 ; P ($50)
.res 1 * 16, $00            ; Q
font_tile $78,$44,$44,$78,$50,$48,$44,$00 ; R ($52)
font_tile $3C,$40,$40,$38,$04,$04,$78,$00 ; S ($53)
font_tile $7C,$10,$10,$10,$10,$10,$10,$00 ; T ($54)
font_tile $44,$44,$44,$44,$44,$44,$38,$00 ; U ($55)
font_tile $44,$44,$44,$44,$44,$28,$10,$00 ; V ($56)
.res 2 * 16, $00            ; W-X
font_tile $44,$44,$28,$10,$10,$10,$10,$00 ; Y ($59)
.res (256 - 90) * 16, $00   ; remaining background tiles
.assert * - background_pattern_table_start = $1000, error, "background pattern table must occupy CHR $1000-$1FFF"
