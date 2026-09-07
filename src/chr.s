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

.res 16 * 16, $00           ; $00-$0F
font_tile $66,$FF,$FF,$FF,$7E,$3C,$18,$00 ; heart ($10)
font_tile $FF,$81,$81,$81,$81,$81,$81,$FF ; slot empty ($11)
font_tile $18,$18,$18,$18,$7E,$18,$18,$3C ; sword ($12)
font_tile $00,$FF,$00,$00,$00,$00,$FF,$00 ; bar empty ($13)
font_tile $00,$FF,$FF,$FF,$FF,$FF,$FF,$00 ; bar filled ($14)
.res 11 * 16, $00           ; $15-$1F

font_tile $00,$00,$00,$00,$00,$00,$00,$00 ; space ($20)
.res 10 * 16, $00           ; ASCII $21-$2A
font_tile $00,$10,$10,$7C,$10,$10,$00,$00 ; + ($2B)
.res 1 * 16, $00            ; ASCII $2C
font_tile $00,$00,$00,$7C,$00,$00,$00,$00 ; - ($2D)
.res 2 * 16, $00            ; ASCII $2E-$2F
font_tile $38,$44,$4C,$54,$64,$44,$38,$00 ; 0 ($30)
font_tile $10,$30,$10,$10,$10,$10,$38,$00 ; 1 ($31)
font_tile $38,$44,$04,$08,$10,$20,$7C,$00 ; 2 ($32)
font_tile $38,$44,$04,$18,$04,$44,$38,$00 ; 3 ($33)
font_tile $08,$18,$28,$48,$7C,$08,$08,$00 ; 4 ($34)
font_tile $7C,$40,$78,$04,$04,$44,$38,$00 ; 5 ($35)
font_tile $18,$20,$40,$78,$44,$44,$38,$00 ; 6 ($36)
font_tile $7C,$04,$08,$10,$20,$20,$20,$00 ; 7 ($37)
font_tile $38,$44,$44,$38,$44,$44,$38,$00 ; 8 ($38)
font_tile $38,$44,$44,$3C,$04,$08,$30,$00 ; 9 ($39)
font_tile $00,$18,$18,$00,$18,$18,$00,$00 ; : ($3A)
.res 3 * 16, $00            ; ASCII $3B-$3D
font_tile $40,$60,$30,$18,$30,$60,$40,$00 ; > ($3E)
.res 2 * 16, $00            ; ASCII $3F-$40
font_tile $38,$44,$44,$7C,$44,$44,$44,$00 ; A ($41)
font_tile $78,$44,$44,$78,$44,$44,$78,$00 ; B ($42)
font_tile $38,$44,$40,$40,$40,$44,$38,$00 ; C ($43)
font_tile $78,$44,$44,$44,$44,$44,$78,$00 ; D ($44)
font_tile $7C,$40,$40,$78,$40,$40,$7C,$00 ; E ($45)
font_tile $7C,$40,$40,$78,$40,$40,$40,$00 ; F ($46)
font_tile $38,$44,$40,$4C,$44,$44,$38,$00 ; G ($47)
font_tile $44,$44,$44,$7C,$44,$44,$44,$00 ; H ($48)
font_tile $38,$10,$10,$10,$10,$10,$38,$00 ; I ($49)
font_tile $04,$04,$04,$04,$04,$44,$38,$00 ; J ($4A)
font_tile $44,$48,$50,$60,$50,$48,$44,$00 ; K ($4B)
font_tile $40,$40,$40,$40,$40,$40,$7C,$00 ; L ($4C)
font_tile $44,$6C,$54,$54,$44,$44,$44,$00 ; M ($4D)
font_tile $44,$64,$54,$4C,$44,$44,$44,$00 ; N ($4E)
font_tile $38,$44,$44,$44,$44,$44,$38,$00 ; O ($4F)
font_tile $78,$44,$44,$78,$40,$40,$40,$00 ; P ($50)
font_tile $38,$44,$44,$44,$54,$48,$34,$00 ; Q ($51)
font_tile $78,$44,$44,$78,$50,$48,$44,$00 ; R ($52)
font_tile $3C,$40,$40,$38,$04,$04,$78,$00 ; S ($53)
font_tile $7C,$10,$10,$10,$10,$10,$10,$00 ; T ($54)
font_tile $44,$44,$44,$44,$44,$44,$38,$00 ; U ($55)
font_tile $44,$44,$44,$44,$44,$28,$10,$00 ; V ($56)
font_tile $44,$44,$44,$54,$54,$6C,$44,$00 ; W ($57)
font_tile $44,$44,$28,$10,$28,$44,$44,$00 ; X ($58)
font_tile $44,$44,$28,$10,$10,$10,$10,$00 ; Y ($59)
font_tile $7C,$04,$08,$10,$20,$40,$7C,$00 ; Z ($5A)
.res 33 * 16, $00           ; ASCII $5B-$7B
font_tile $10,$10,$10,$10,$10,$10,$10,$10 ; | ($7C)
.res 131 * 16, $00          ; remaining background tiles ($7D-$FF)
.assert * - background_pattern_table_start = $1000, error, "background pattern table must occupy CHR $1000-$1FFF"
