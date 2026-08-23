# Diário de desenvolvimento da branch: main

## 2026-08-23 — Primeira máquina de estados para as telas iniciais

### O que mudou

Foram adicionados os estados explícitos `PRESENTED_BY`, `TITLE` e `PLAYING`,
nametables centralizadas para crédito e título, timeout de 150 frames, blink de
30 frames, inicialização tardia do gameplay, fonte esparsa de background, testes
host-side e cobertura de runtime no Mesen. Scripts de build e documentação
bilíngue de arquitetura, memória e README foram sincronizados.

### Por que foi necessário

A ROM inicializava o gameplay imediatamente e apenas trocava um marcador interno
de `BOOT` para `RUNNING`. Estados pré-run explícitos criam um limite pequeno e
revisável para uma seleção de personagem futura, sem antecipar menus, pause ou
progressão fora deste marco.

### Trecho relevante

```c
case GAME_STATE_PRESENTED_BY:
    if ((pressed_buttons & BUTTON_START) != 0U) {
        enter_title();
    } else if (state_timer > 1U) {
        --state_timer;
    } else {
        enter_title();
    }
    break;
```

A política consome a máscara existente de borda pressionada, então START mantido
não atravessa duas telas. A entrada visual permanece separada da lógica de estado.

### Considerações sobre o NES

Trocas completas desabilitam NMI/renderização, limpam 1.024 bytes, aguardam
VBlank e restauram scroll e configuração fixa da PPU. O blink escreve 11 tiles com a mesma proteção
do latch. O texto usa CHR de background e não consome OAM. O despacho termina em
`game_init()`, de modo que o hot path medido não recebe branch ou chamada C por
frame.

### Desempenho

A primeira integração com despacho durante a run perdeu quatro updates no stress
de 12 Bats e foi removida. Medição final no Mesen: 1.750 frames de vídeo, 12 Bats,
1.735 NMIs/updates de gameplay após a transição e zero perda. Os ciclos das
escritas de VRAM não foram medidos separadamente.

### Impacto em recursos

- PRG-ROM: 6.205 -> 7.099 bytes (`+894`).
- BSS: 78 -> 80 bytes (`+2`).
- Zero page, DATA, shadow de OAM, sprites de hardware e stacks: inalterados.
- CHR: 21 glifos não vazios no banco antes livre `$1000`; tiles de sprite
  `$00-$0D` inalterados.

### Arquivos principais afetados

`src/game.c`, `src/game_flow.c`, `src/screen.c`, `src/chr.s`, headers associados,
scripts de build, testes C/Python/Lua, READMEs, documentos de arquitetura/memória
e `docs/implementation-notes/game-states.md`.

### Validação

- build limpo da ROM: PASS
- testes lógicos host-side no sim65: PASS
- validação estrutural de ROM/CHR/map: PASS
- runtime das telas no Mesen (110 frames): PASS
- runtime player/espada/Bat no Mesen (450 frames): PASS
- stress de 12 Bats no Mesen (1.750 frames): PASS, zero perdas no gameplay
- warnings como erros e `git diff --check`: PASS na validação final

### Limitações / próximos passos

As transições são cortes diretos e podem manter a renderização desligada por
mais de um frame de vídeo. A fonte é fixa e monocromática. O timing assume NTSC.
Um estado futuro que interrompa gameplay ativo precisa de dispatcher próprio e
medido; esta mudança estabelece somente o lifecycle pré-run.

## 2026-08-23 — Drops delimitados de gema de XP e telas em caixa alta

### O que mudou

Cada derrota pela espada cria uma gema 8x8 centralizada no Bat. Um pool fixo de
oito slots controla surgimento, desaparecimento por contato, condensação na
saturação e renderização determinística. O CHR fornecido adiciona o tile `$14`.
Todos os textos visíveis e a fonte esparsa de background agora usam caixa alta
na pattern table 1.

### Por que foi necessário

Inimigos derrotados precisavam deixar pickups visíveis sem antecipar ganho de
XP ou progressão. Pool e condensação limitam RAM/OAM e evitam descartar eventos
silenciosamente quando todos os slots visíveis estão ocupados.

### Trecho relevante

```c
index = sword_hitbox_scan_parity;
sword_hitbox_scan_parity ^= 1U;
for (; index < pool_high_water; index = (uint8_t)(index + 2U)) {
    xp_gem_spawn((uint8_t)(bat_x + 4U), bat_y);
}
```

A espada ativa alterna slots pares e ímpares. Cada inimigo continua sendo
verificado em até dois frames ativos, e a derrota emite a gema antes de liberar
o slot.

### Considerações sobre o NES

O pool não usa heap. Oito gemas visíveis consomem oito entradas de OAM e são
renderizadas depois dos inimigos, preservando a prioridade de player, espada e
ameaças. Pool cheio incrementa a contagem representada pela gema mais próxima.
O contato verifica um slot por frame, limitando o custo com latência máxima de
oito frames. Índices maiúsculos continuam na tabela de background em `$1000`;
o tile `$14` permanece na tabela de sprites em `$0000`.

### Desempenho

Cenário: stress de 1.750 frames no Mesen com 12 Bats.

- baseline: 1.735 NMIs / 1.735 updates / 0 perdidos;
- primeira integração: 1.735 / 1.727 / 8 perdidos;
- colisão escalonada final: 1.735 / 1.735 / 0 perdidos.

Os ciclos de cada rotina não foram medidos separadamente.

### Impacto em recursos

- PRG-ROM: 7.043 -> 7.994 bytes (`+951`);
- BSS: 81 -> 124 bytes (`+43`);
- zero page, DATA, shadow de OAM e reservas de stack: inalterados;
- capacidade de CHR-ROM: 8 KiB, inalterada;
- conteúdo de sprites: um novo tile em `$14`;
- pior caso de sprites de hardware: 33 -> 41 de 64.

### Arquivos principais afetados

`src/xp_gem.c`, `include/xp_gem.h`, `src/enemy.c`, `src/game.c`, `src/screen.c`,
`src/chr.s`, `assets/game.chr`, tuning/build, testes C/Python/Mesen, READMEs,
arquitetura, orçamentos e a nota técnica em português.

### Validação

- build limpo da ROM com warnings como erros: PASS
- testes sim65 de spawn/contato/saturação/OAM: PASS
- validação estrutural de ROM/CHR/map: PASS
- telas iniciais no Mesen (175 frames): PASS
- gameplay no Mesen (450 frames): PASS
- stress de 12 Bats/gemas (1.750 frames): PASS, zero updates perdidos
- `git diff --check`: PASS

### Limitações / próximos passos

A coleta não concede XP e descarta a contagem representada, conforme o escopo.
A gema de um frame não muda visualmente. O contato pode levar oito frames.
Flicker por scanline e coleta no fim da wave permanecem trabalho futuro.
