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
segunda pattern table contém 17 glifos não vazios de um bitplane nos códigos
ASCII maiúsculos usados por `PRESENTED BY`, `CODIGO E CARTUCHO`, `NES SURVIVOR`
e `PRESS START`. Isso não usa sprites e não sofre o limite de oito sprites por
scanline.

Uma troca completa desabilita NMI e renderização antes de limpar os 1.024 bytes
da nametable e escrever o texto. Depois aguarda o próximo VBlank, restaura
scroll zero, `PPUCTRL=$90` e `PPUMASK=$1E`.

## Correção do blink e atualização curta de VRAM

A primeira versão desligava e religava `PPUCTRL`/`PPUMASK` em cada meio-ciclo do
blink. A thread principal acordava após a NMI, mas o tempo de input, máquina de
estados e C gerado não garantia que a renderização seria religada ainda dentro
do VBlank. Se a PPU retomasse no quadro visível depois das escritas em `$2006`,
o endereço interno ainda podia refletir `$220A`; antes da restauração normal do
scroll no próximo pre-render scanline, partes do texto eram buscadas em outra
posição. O sintoma era um fragmento próximo ao topo antes do prompt correto.

Desligar rendering periodicamente também era desnecessário para 11 tiles. A
função agora apenas publica um pedido de um byte:

```c
screen_title_prompt_update = visible != 0U
    ? TITLE_PROMPT_UPDATE_SHOW
    : TITLE_PROMPT_UPDATE_HIDE;
```

Na NMI seguinte, depois do DMA de OAM, o handler reinicia o latch compartilhado,
define o endereço fixo e escreve o prompt inteiro durante VBlank:

```asm
lda PPUSTATUS
lda #>TITLE_PROMPT_ADDRESS
sta PPUADDR
lda #<TITLE_PROMPT_ADDRESS
sta PPUADDR
```

O endereço é `$220A`, correspondente à nametable `$2000`, linha 16 e coluna 10.
Mostrar copia os 11 índices ASCII de `screen_title_prompt_text`; ocultar escreve
11 tiles zero. A flag só é limpa depois da transferência completa, e as duas
escritas de `PPUSCROLL` continuam no final da NMI. `PPUCTRL` e `PPUMASK` não são
tocados durante o blink.

O caminho normal da NMI passou de aproximadamente 583 para 590 ciclos por causa
da consulta da flag. O pior caminho, ao mostrar os 11 tiles, é estimado em 788
ciclos, ainda abaixo dos cerca de 2.273 ciclos de VBlank NTSC. São estimativas
do fluxo de instruções, não medições do profiler de ciclos.

## Divisão das pattern tables

A divisão é explícita:

```text
CHR $0000-$0FFF -> Pattern Table 0 -> sprites
CHR $1000-$1FFF -> Pattern Table 1 -> background e fonte
```

`src/chr.s` inclui exatamente `$1000` bytes do asset de sprites e possui asserts
de montagem para que cada metade continue com 4 KiB. Os glifos ficam fisicamente
na segunda metade, nos offsets `0x1000 + codigo_ascii * 16`.

O `PPUCTRL` usado pelo jogo é `$90` (`%10010000`): bit 7 habilita NMI, bit 4
seleciona a Pattern Table 1 para background e o bit 3 limpo mantém sprites na
Pattern Table 0. Os valores ASCII escritos na nametable continuam sendo índices
de 8 bits; quem acrescenta a base `$1000` à busca de background é a PPU.

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
- `screen_set_title_prompt_visible()` e o bloco opcional de `nmi_handler`, que
  mostram pedido na thread principal e consumo delimitado em VBlank;
- o macro `font_tile` em `src/chr.s`, que demonstra os glifos no banco `$1000`;
- os testes de timeout, START mantido e blink em `tests/test_logic.c` e
  `tests/mesen_game_states.lua`.

## Custos medidos e estimados

Medidos no mapa final do linker:

- PRG-ROM total atual: 7.994 bytes; o marco de telas isolado media 7.043 bytes;
- BSS total atual: 124 bytes; o marco de telas isolado media 81 bytes;
- zero page, DATA, OAM e stacks: inalterados;
- OAM: inalterada; as telas iniciais ocultam todas as 64 entradas;
- CHR com significado: 21 tiles de sprite e 17 glifos maiúsculos não vazios.

Medido no Mesen 2.2.1: o stress de 1.750 frames saturou 12 Bats, observou uma
gema e registrou
1.735 atualizações/NMIs de gameplay após a baseline de transição, sem perda. O
teste de telas confirmou textos, OAM oculto, START mantido, vários ciclos de
blink sem estado parcial, ausência de writes em `$2000/$2001` durante o blink,
`PPUCTRL=$90`, glifo na Pattern Table 1 e entrada na run. O PPU Viewer gráfico
ainda deve ser inspecionado manualmente; o teste headless verifica o mesmo estado
por memória e callbacks, mas não substitui a observação humana do quadro.

## Limitações e evoluções relacionadas

- As trocas são cortes diretos e podem deixar a renderização desligada por mais
  de um frame enquanto a nametable completa é limpa.
- O texto é fixo, monocromático e cobre somente os glifos usados agora.
- A duração usa NTSC; PAL/Dendy ainda não possui adaptação.
- Não há fade, áudio, menu nem seleção de personagem.
- Estados que interrompam gameplay exigirão desenho e medição próprios do
  dispatcher, sem assumir que o custo é gratuito.
