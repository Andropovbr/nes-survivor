# Primeira máquina de estados do jogo

## Problema anterior

Antes desta mudança, `main()` chamava `game_init()` e a ROM criava player,
espada e pool de Bats imediatamente. O único estado era uma passagem interna de
`BOOT` para `RUNNING`; não existia espaço explícito para crédito ou title screen.

A máquina de estados foi introduzida agora para que a entrada da run tenha um
fluxo previsível sem espalhar condições pelo loop de gameplay:

```text
PRESENTED_BY -> TITLE -> PLAYING
```

## Estados e transições

`game_flow.c` contém somente a política temporal e pode ser exercitado no
`sim65`. A inicialização parte de `GAME_STATE_PRESENTED_BY`:

```c
current_state = GAME_STATE_PRESENTED_BY;
state_timer = PRESENTED_BY_DURATION_FRAMES;
```

O timer vale 150 atualizações sincronizadas, ou 2,5 segundos na referência NTSC
de 60 Hz. Ele conta até a transição para `TITLE`; uma borda de START realiza a
mesma transição antes do timeout. A title screen entra com o prompt visível e
um countdown de 30 frames. Ao vencer, a flag alterna e o countdown é recarregado:

```c
title_prompt_visible = (uint8_t)(title_prompt_visible == 0U);
state_timer = TITLE_BLINK_HALF_PERIOD_FRAMES;
```

Somente outra borda de START muda o estado para `PLAYING`. Nesse ponto
`screen_show_gameplay()` limpa a nametable, e `gameplay_init()` cria player,
espada, pool de inimigos e primeira imagem da shadow de OAM. Esses sistemas não
existem durante as telas iniciais.

## START recém-pressionado

O input já mantinha as máscaras atual, pressionada e solta. A máquina recebe
`input_pressed()`, cuja implementação usa:

```c
pressed_buttons = (uint8_t)(sample & (uint8_t)~previous);
```

Assim, segurar START para pular o crédito não cria uma segunda borda na title
screen. É necessário soltar e pressionar novamente. O teste host-side combina
amostras reais do módulo de input com `game_flow_update()` para cobrir esse caso.

## Nametable, fonte e segurança da PPU

`screen.c` fornece apenas as quatro operações necessárias: montar crédito,
montar title, alternar o prompt e limpar para gameplay. Os textos são arrays
ASCII e seus valores são escritos diretamente como índices de tiles. As colunas
foram escolhidas a partir da largura de 32 tiles para centralização visual.

Os primeiros 4 KiB de CHR continuam sendo o banco de sprites existente. A
segunda pattern table contém 21 glifos não vazios de um bitplane nos códigos
ASCII usados por `Presented by`, `Codigo e Cartucho`, `NES Survivor` e
`Press Start`. Isso não usa sprites e não sofre o limite de oito sprites por
scanline.

Uma troca completa desabilita NMI e renderização antes de limpar os 1.024 bytes
da nametable e escrever o texto. Depois aguarda o próximo VBlank, restaura
scroll zero, `PPUCTRL=$90` e `PPUMASK=$1E`. O blink modifica somente os 11 tiles do prompt no limite
sincronizado do frame, mas usa a mesma proteção para não deixar corrida no latch
compartilhado de `PPUADDR`/`PPUSCROLL`.

## Preservação do hot path

Uma primeira integração consultava o estado antes de toda atualização de
gameplay. Mesmo esse custo pequeno voltou a produzir frames perdidos no cenário
de 12 Bats. A forma final executa os estados iniciais dentro de `game_init()`;
ao entrar em `PLAYING`, a função retorna e `main()` usa o loop de gameplay já
medido, sem despacho recorrente.

Essa decisão permite acrescentar seleção de personagem e outros estados pré-run
no coordenador frio. Pause, level-up e game over, por interromperem uma run,
precisarão de um dispatcher de runtime medido em um marco futuro. Nenhum desses
estados foi antecipado aqui.

## Trechos úteis para vídeo

Os pontos mais representativos são:

- o `switch` de `game_flow_update()`, que mostra as três políticas explícitas;
- o cálculo de borda em `input_apply_sample()`;
- `initial_screens_update()`, onde a mudança de estado dispara a entrada de tela;
- `screen_show_title()`, que mostra a sequência PPU segura;
- o macro `font_tile` em `src/chr.s`, que demonstra os glifos no banco `$1000`;
- os testes de timeout, START mantido e blink em `tests/test_logic.c` e
  `tests/mesen_game_states.lua`.

## Custos medidos e estimados

Medidos no mapa final do linker:

- PRG-ROM: 7.099 bytes, aumento de 894 bytes;
- BSS: 80 bytes, aumento líquido de 2 bytes;
- zero page, DATA, OAM e stacks: inalterados;
- OAM: inalterada; as telas iniciais ocultam todas as 64 entradas;
- CHR com significado: 14 tiles de sprite e 21 glifos não vazios.

Medido no Mesen 2.2.1: o stress de 1.750 frames saturou 12 Bats e registrou
1.735 atualizações/NMIs de gameplay após a baseline de transição, sem perda. O
teste de telas confirmou textos, OAM oculto, START mantido, duas fases do blink
e entrada na run. O custo em ciclos das escritas de VRAM não foi medido.

## Limitações e evoluções relacionadas

- As trocas são cortes diretos e podem deixar a renderização desligada por mais
  de um frame enquanto a nametable completa é limpa.
- O texto é fixo, monocromático e cobre somente os glifos usados agora.
- A duração usa NTSC; PAL/Dendy ainda não possui adaptação.
- Não há fade, áudio, menu nem seleção de personagem.
- Estados que interrompam gameplay exigirão desenho e medição próprios do
  dispatcher, sem assumir que o custo é gratuito.
