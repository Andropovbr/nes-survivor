# Implementação de HUD em Background, Acúmulo de XP e Menu de Level-Up

## Problema

Antes desta implementação, o jogador não tinha retorno visual de pontos de vida (HP), experiência acumulada (XP), nível atual, nem das armas e bônus equipados. Além disso, as gemas de XP dropadas pelos morcegos eram coletadas mas descartadas sem conceder progressão, e não havia transição para escolha de melhorias ou subida de nível.

Adicionar um HUD tradicional usando sprites (OAM) consumiria entre 15 a 30 sprites de hardware, sobrecarregando o limite rígido de 64 sprites do NES e o limite crítico de 8 sprites por scanline, o que provocaria flickering e impediria ondas maiores de inimigos.

## Solução adotada

1. **Separação de Pattern Tables:**
   - **Tabela 0 (`$0000-$0FFF`):** Exclusiva para sprites (Player, Espada, Morcegos, Gemas de XP).
   - **Tabela 1 (`$1000-$1FFF`):** Exclusiva para tiles de background (fontes alfanuméricas, pontuação, caixas de diálogo e elementos do HUD).
   - Configuração de hardware: `PPUCTRL = $90` (`NES_PPUCTRL_NMI_ENABLE | NES_PPUCTRL_BACKGROUND_TABLE_1000`).

2. **Full-Background HUD (Linhas 0 e 1 da Nametable):**
   - **Linha 0:** `"HP:"` + ícone de coração + 6 tiles de barra de vida + `"   XP: "` + 10 tiles de barra de XP + `" LV"` + 2 dígitos decimais do nível.
   - **Linha 1:** `"WPN: "` + 4 slots de armas (o primeiro com ícone de espada, os outros vazios) + `"   BNS: "` + 4 slots de bônus passivos vazios.
   - **Consumo de sprites do HUD:** **0 sprites** durante o gameplay. Todo o orçamento de 64 sprites permanece livre para entidades de jogo.

3. **Buffer de VRAM Limitado e Notificações de Dirty:**
   - Atualizações dinâmicas (barras de HP, XP e dígitos de nível) utilizam notificações explícitas de dirty (`hud_notify_*`). Em frames sem alteração de stats, `hud_update()` retorna em ~8 ciclos.
   - Um buffer sequencial de no máximo 1 transferência por frame (`vram_buffer_len <= 10` bytes) é enfileirado na memória RAM/BSS.
   - Durante a VBlank, o tratador NMI descarrega o pacote via `$2006/$2007` antes de restaurar o scroll zero, custando ~150 a 200 ciclos de CPU (bem dentro dos ~2.273 ciclos da VBlank NTSC).

4. **Progressão Inteira e Coleta de Gemas:**
   - Cada gema coletada em `xp_gem_update` transfere suas unidades (`gem_drop_units`) para `player_add_xp()`.
   - Curva de nível inteira determinística sem uso de ponto flutuante: 5, 12, 22, 35, 52, 75, 105, 145, 200 gemas (depois +60 por nível). Cálculos de preenchimento de barra usam estritamente aritmética de 16 bits para máxima velocidade no 6502.
   - Ao atingir o limiar, ativa `player_level_up_pending()`.

5. **Estado e Modal de Level-Up (`GAME_STATE_LEVEL_UP`):**
   - Pausa determinística: movimentação do jogador, perseguição dos morcegos, ataques de espada, temporizadores e coleta de gemas são congelados.
   - Todos os sprites de gameplay são ocultados da OAM (`screen_hide_all_sprites()`) para evitar sobreposição indesejada no diálogo.
   - Uma janela central de 16x8 tiles é desenhada na Nametable 0 (linhas 10-17) com 3 opções placeholder (Espada +1, Max HP +1, Speed +1).
   - Direcionais Cima/Baixo movem o cursor `>` (com atualização realizada estritamente durante o VBlank no NMI para evitar corte de scanline e decaimento de DRAM da OAM), e o botão A ou START confirma.
   - A confirmação aplica a melhoria (ex: Max HP aumenta vida máxima e atual em 1), deduz a XP necessária, atualiza o HUD, apaga o modal restaurando o fundo e retorna para `GAME_STATE_PLAYING` (ou encadeia novo modal se houver outro nível pendente).

## Fluxo de execução

```text
[Gameplay Ativo]
  -> Inimigo derrotado spawna gema de XP
  -> Jogador toca na gema
  -> xp_gem_update() retorna unidades coletadas
  -> player_add_xp() acumula XP
  -> hud_notify_xp_changed()
  -> Se xp >= next_level_xp:
       -> level_up_init()
       -> screen_show_level_up_modal()
       -> game_flow_enter_level_up()

[Estado LEVEL_UP]
  -> game_update() processa apenas input_pressed() do menu
  -> D-Pad Cima/Baixo: screen_update_level_up_cursor() -> NMI aplica VBlank
  -> Botão A / START:
       -> player_apply_level_up(cursor)
       -> screen_hide_level_up_modal()
       -> hud_notify_*()
       -> hud_update()
       -> Se houver level_up_pending: abre próximo modal
       -> Senão: game_flow_exit_level_up() -> Retorna ao Gameplay
```

## Trechos de código real

### Saída ultrarrápida e matemática inteira em `src/hud.c`
```c
if (dirty_flags == 0U || vram_buffer_len != 0U) {
    return;
}

if ((dirty_flags & DIRTY_XP) != 0U) {
    uint16_t current_xp = player_xp();
    uint16_t current_next_level_xp = player_next_level_xp();
    if (current_next_level_xp != 0U) {
        fill_count = (uint8_t)(((uint16_t)current_xp * HUD_XP_BAR_TILES) / current_next_level_xp);
    } else {
        fill_count = 0U;
    }
    vram_buffer_addr_hi = 0x20U;
    vram_buffer_addr_lo = 0x12U;
    for (i = 0U; i < HUD_XP_BAR_TILES; ++i) {
        vram_buffer_data[i] = (i < fill_count) ? HUD_TILE_BAR_FULL : HUD_TILE_BAR_EMPTY;
    }
    vram_buffer_len = HUD_XP_BAR_TILES;
    dirty_flags &= ~DIRTY_XP;
}
```

### Atualização segura de cursor no VBlank via NMI em `src/nmi.s`
```s
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
    ; Atualiza também linhas 14 e 15 e limpa flag
    lda #$00
    sta _screen_level_up_cursor_update
```

## Restrições e trade-offs do NES

- **Segurança da PPU e VBlank:** Atualizações diretas na PPU durante a renderização ativa corrompem a imagem e o scroll. A movimentação do cursor e do HUD ocorre estritamente dentro da NMI / VBlank, sem forçar blanking no meio do frame.
- **Economia de Sprites:** Construir o HUD inteiramente em background tiles poupa 100% dos 64 sprites para inimigos e efeitos, evitando estouro de scanline (limite de 8 sprites horizontais).
- **Sem Ponto Flutuante:** A progressão e barras utilizam exclusivamente matemática inteira de 16 bits, garantindo 0 frames dropados no teste de estresse de 12 morcegos (`make test-performance`).

## Impacto de recursos e memória (evidência do build map)

- **Zero Page:** 28 bytes utilizados (inalterado, 0 bytes adicionados).
- **OAM Shadow:** 256 bytes (inalterado; HUD usa 0 sprites).
- **RAM Geral (DATA + BSS):** 219 bytes utilizados de 512 bytes (37 DATA + 182 BSS; 293 bytes livres).
- **PRG-ROM:** 10.307 bytes utilizados de 32.768 bytes (31,5% da capacidade, restando 22.461 bytes livres / 68,5% de folga).
- **CHR-ROM:** 8.192 bytes (4 KiB sprites na tabela 0 + 4 KiB background na tabela 1).

## O que observar no Mesen

1. **Top 2 Rows (Linhas 0 e 1):** Ao iniciar o jogo após o Title Screen, o topo exibe a barra de HP verde/preenchida, barra de XP vazia e nível "01". A segunda linha mostra o ícone da espada e os slots vazios.
2. **Coleta de Gemas de XP:** Conforme os morcegos são abatidos e suas gemas recolhidas, a barra de XP preenche gradualmente seus 10 segmentos.
3. **Pausa e Modal de Level-Up:** Ao coletar 5 gemas (nível 1 para 2), a ação congela instantaneamente. Os sprites são ocultados para não cobrir o diálogo. A caixa de diálogo central aparece com os glifos `"|  LEVEL UP!   |"` e opções numeradas `"1.", "2.", "3."`.
4. **Navegação Suave do Cursor:** Pressionar Cima e Baixo no direcional move a seta `>` instantaneamente via VBlank sem cintilação de tela. Pressionar A ou START seleciona a opção, fecha o menu, limpa a tela de diálogo e retoma o jogo com nível "02" e HP/XP recalculados.
5. **Estabilidade de Desempenho:** Teste de estresse com 12 morcegos e ataque ativo (`mesen_bat_stress.lua`) roda com 0 frames pulados (`gameplay_skipped=0`).
