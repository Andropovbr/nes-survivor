# Registro de Desenvolvimento da Branch: hud-xp-levelup

## 2026-09-06 — Adição de HUD em Background, Progressão de XP e Menu de Level-Up

### O que mudou

Implementação de um HUD completo desenhado na Nametable 0 de background (linhas 0 e 1), acúmulo de XP a partir das gemas coletadas com curva de progressão inteira e modal determinístico de 3 escolhas de level-up no estado `GAME_STATE_LEVEL_UP`:

1. **Tabela de Padrões e Tiles de Background:**
   - Preenchida a Pattern Table 1 (`$1000-$1FFF`) em `src/chr.s` com glifos de fonte (maiúsculas ASCII, dígitos `0` a `9`, pontuações `+`, `-`, `:`, `>`, `|`) e tiles de HUD (coração, moldura de slot, espada, barra vazia e barra cheia).
   - Mantida a Pattern Table 0 (`$0000-$0FFF`) para sprites do jogador, espada, inimigos e gemas.

2. **Full-Background HUD (Linhas 0 e 1):**
   - Linha 0: Rótulo HP, ícone de coração, barra de vida de 6 segmentos, rótulo XP, barra de XP de 10 segmentos e contador de nível de 2 dígitos.
   - Linha 1: 4 slots de arma (espada pré-equipada no slot 0) e 4 slots de bônus passivo.
   - Consome 0 sprites de hardware, preservando todos os 64 sprites para entidades de gameplay.

3. **Buffer de VRAM Limitado e Entrega em VBlank:**
   - Buffer sequencial de disparo único (`_vram_buffer_len <= 10` bytes) em `src/nes.s` e `src/hud.c`.
   - Transferência segura durante a VBlank em `src/nmi.s` antes da restauração do scroll, consumindo menos de 200 ciclos.

4. **Curva de Progressão de XP:**
   - Retorno de `xp_gem_update` integrado diretamente com `player_add_xp()`.
   - Tabela pré-computada de limiares inteiros (`level_thresholds: 5, 12, 22, 35, 52, 75, 105, 145, 200`) evitando operações pesadas de ponto flutuante via software.

5. **Sistema de Escolha de Level-Up:**
   - Adicionado `GAME_STATE_LEVEL_UP` em `game_flow`.
   - Pausa determinística que congela jogador, movimentação de inimigos, checagem de colisão, temporizadores de ataque e coleta de gemas.
   - Modal central de 16x8 tiles com 3 escolhas navegadas com Cima/Baixo no direcional e confirmadas com `BUTTON_A` ou `BUTTON_START`.
   - Confirmação aplica o efeito, deduz a XP, avança o nível, limpa o overlay e retoma o jogo suavemente.

### Trecho de código relevante

```c
/* Enfileiramento limitado no buffer de VRAM em src/hud.c */
if ((dirty_flags & DIRTY_XP) != 0U) {
    fill_count = (uint8_t)(((uint32_t)cached_xp * HUD_XP_BAR_TILES) / cached_next_level_xp);
    vram_buffer_addr_hi = 0x20U;
    vram_buffer_addr_lo = 0x12U;
    for (i = 0U; i < HUD_XP_BAR_TILES; ++i) {
        vram_buffer_data[i] = (i < fill_count) ? HUD_TILE_BAR_FULL : HUD_TILE_BAR_EMPTY;
    }
    vram_buffer_len = HUD_XP_BAR_TILES;
    dirty_flags &= ~DIRTY_XP;
}
```

```s
; Execução da transferência na VBlank em src/nmi.s
@check_vram_buffer:
    lda _vram_buffer_len
    beq @restore_scroll
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
```

### Considerações de hardware NES

- **0 Sprites para HUD:** Manter todos os elementos do HUD no background previne cintilação e preserva o limite de 8 sprites por scanline para o gameplay.
- **Segurança de NMI/VBlank:** Escritas na PPU isoladas estritamente na VBlank com no máximo 10 bytes por frame, bem abaixo do limite de 2.273 ciclos da VBlank NTSC.
- **Aritmética Inteira:** Toda a matemática usa tipos inteiros de largura fixa (`uint8_t`, `uint16_t`, `uint32_t`).

### Impacto em recursos e memória

- **Zero Page:** 28 bytes utilizados (0 bytes de variação).
- **RAM / BSS:** 188 bytes utilizados (+60 bytes de variação, 287 bytes livres).
- **PRG-ROM:** 10.597 bytes utilizados (+2.194 bytes de variação, 22.171 bytes livres / 67,7% de folga).
- **CHR-ROM:** 8.192 bytes (Tabela 0 para sprites, Tabela 1 para background).
- **OAM Shadow:** 256 bytes (0 sprites alocados para o HUD).

### Validação exata realizada

- `make clean; make test`: Compilação limpa sem avisos com `--warnings-as-errors`, todos os testes unitários de lógica no `sim65` passaram.
- `python tests/validate_rom.py`: Cabeçalho, mapeamento e integridade de labels da ROM verificados com sucesso.
- `make test-runtime`: Testes automatizados no emulador Mesen aprovados:
  - `tests/mesen_game_states.lua`: Passou (175 frames).
  - `tests/mesen_player.lua`: Passou (450 frames).
  - `tests/mesen_player_damage.lua`: Passou (546 frames).
