# Arquitetura

## Módulos atuais

- `src/crt0.s` contém o cabeçalho iNES, caminho de reset, inicialização de RAM e PPU, inicialização do runtime do cc65, habilitação da renderização e vetores de interrupção.
- `src/nmi.s` é o tratador de NMI delimitado. Ele faz o upload da página shadow de OAM, restaura o scrolling em zero e avança o contador de frames.
- `src/nes.s` gerencia a alocação de OAM alinhada à página, a primitiva de espera de frame e a rotina de leitura da porta de controle.
- `src/main.c` orquestra a inicialização e o loop principal sincronizado.
- `src/game.c` coordena o ciclo das telas iniciais, inicializa o gameplay somente na entrada de `PLAYING` e então orquestra player, espada, colisão com inimigos, coleta de gemas e reconstrução determinística da OAM sem custo de despacho no hot path.
- `src/game_flow.c` mantém os estados explícitos `PRESENTED_BY`, `TITLE` e `PLAYING` e seus timers determinísticos em frames.
- `src/screen.c` realiza as pequenas escritas de nametable, paleta e texto usadas pelo crédito e pela title screen.
- `src/input.c` deriva as máscaras de botões atuais, pressionados e soltos a partir da amostragem direta do hardware.
- `src/rng.c` implementa o estado determinístico de `xorshift16` e suas funções de geração.
- `src/player.c` gerencia o estado mutável e compacto do jogador, movimentação delimitada em 8 direções, orientação horizontal, seleção de animação e política de renderização do jogador.
- `src/animation.c` é um reprodutor de frames reutilizável e orientado a dados. Ele armazena apenas o ID da animação, frame local e temporizador de contagem regressiva; durações geradas controlam a repetição (looping), e apenas a alteração de animação reinicia a reprodução para o frame zero.
- `src/metasprite.c` oculta entradas não utilizadas da OAM e expande registros de tiles relativos com sinal para a shadow de OAM existente. Seu espelhamento horizontal opcional ajusta tanto a geometria quanto o bit de inversão (flip) de hardware.
- `src/soldier_animation_data.c` consolida as exportações de idle e caminhada do Soldier geradas separadamente sob os símbolos `soldier`. Os 21 registros de tiles, 3 frames e 2 definições mantêm seus valores gerados; apenas os offsets agregados e nomes foram alterados.
- `src/weapon_sword.c` mantém os dois bytes de estado da espada automática e o frame gerado exato de dois tiles. A espada é renderizada depois do jogador, expõe a hitbox ativa correspondente e omite ataques totalmente fora da tela.
- `src/enemy.c` gerencia o pool fixo de 12 Bats, spawn nas bordas, temporização Q4 compartilhada de perseguição e animação, colisão com espada e renderização determinística.
- `src/bat_animation_data.c` adapta os frames e tiles anexados do Bat ao formato imutável compartilhado.
- `src/xp_gem.c` mantém o pool fixo de oito gemas, condensação na saturação, contato escalonado com o player e renderização de um sprite.
- `include/tuning.h` contém geometria e velocidade de player/espada/Bat, tempos de ataque e spawn, limites da arena e capacidades fixas.

## Limite de nomenclatura entre Player e Soldier

`player` representa a entidade em tempo de execução controlada através do controle 1. `PlayerState`, `PlayerFacing`, a API `player_*` e os limites de posição/movimentação `PLAYER_*` permanecem, portanto, independentes de personagem. `AnimationPlayer` também permanece genérico: é um cursor de reprodução de animação reutilizável, não a identidade do personagem jogável.

`soldier` representa a arte concreta atualmente vinculada a essa entidade em tempo de execução. `soldier_animation_data`, `SOLDIER_ANIMATION_*`, as tabelas internas de `soldier_animation_sprites`/frames/definitions e `soldier_sprite_palette` são específicas do asset. O módulo de player é o único ponto de integração em C que seleciona as definições do Soldier e espelha o metasprite atual quando a orientação é para a esquerda.

Os primeiros 4 KiB de `assets/game.chr` são vinculados através de `src/chr.s`. Soldier usa `$00-$07`, a espada animada `$08-$09`, Bat `$0A-$0D` e a gema usa `$14` na pattern table `$0000`. A tabela de background em `$1000` contém somente os glifos ASCII maiúsculos esparsos de um bitplane exigidos pelas telas iniciais; o tile zero permanece vazio. O startup carrega as paletas de sprites, e o módulo de tela carrega a paleta mínima preta/branca de background com renderização e NMI desabilitadas. Soldier e espada usam a paleta 0, Bat usa a 1 e a gema usa a 3.

## Limite entre C e Assembly

O C gerencia políticas, transições de estado e lógica pura. O Assembly é restrito à sequência de reset do NES, processamento de interrupções, I/O mapeado em memória, DMA de OAM, espera de frame e leitura serial do controle. Cada ponto de entrada em Assembly chamável a partir do C documenta sua ABI em `include/nes.h` e em sua respectiva implementação.

A lógica de gameplay deve permanecer em C a menos que a inspeção do código gerado ou medições no emulador identifiquem um gargalo concreto. Uma rotina executar a cada frame não é, por si só, motivo para movê-la para Assembly.

## Ciclo de vida do frame

1. O reset desabilita a renderização e as fontes de interrupção, aguarda a estabilização da PPU, limpa todos os 2 KiB da RAM interna e inicializa a stack de software do cc65.
2. Com a renderização desabilitada, a inicialização limpa `$2000-$2FFF`, preenche todas as entradas de paleta com o preto do NES (`$0F`), preenche a shadow de OAM com `$FF`, inicializa o runtime de C e habilita a NMI juntamente com a renderização de background/sprites.
3. A rotina de NMI preserva A/X/Y, realiza um DMA de OAM de 256 bytes a partir de `$0200`, escreve opcionalmente os 11 tiles fixos do prompt, restaura o scrolling para zero, incrementa um contador de frames de 8 bits na zero page, restaura os registradores e retorna. O caminho normal usa aproximadamente 590 ciclos; mostrar o prompt usa aproximadamente 788 ciclos, ainda dentro dos cerca de 2.273 ciclos do VBlank em NTSC.
4. `nes_wait_frame` captura uma cópia instantânea (snapshot) do contador e aguarda até que a NMI o altere. Uma comparação de 8 bits é atômica no 6502; o estouro de ciclo (wraparound) é seguro porque 256 NMIs não podem ocorrer entre a captura e a comparação.
5. Durante a inicialização, `game.c` avança a máquina de crédito/title com amostras de input sincronizadas pela NMI. Trocas completas desabilitam temporariamente NMI e renderização, limpam 1.024 bytes, escrevem o texto fixo, aguardam VBlank e restauram scroll zero antes de reabilitar a renderização.
6. Depois da entrada em `PLAYING`, a inicialização retorna para `main`. O loop principal atualiza player, espada automática e perseguição dos Bats, aplica a hitbox apenas durante frames ativos, verifica uma gema contra o player e reconstrói a OAM na ordem player, espada opcional, pool estável de inimigos e pool estável de gemas. O trabalho permanece fora da NMI.

## Estados iniciais do jogo

`game_flow` começa em `GAME_STATE_PRESENTED_BY`, muda para `GAME_STATE_TITLE` após 150 atualizações NTSC ou uma borda de START e muda para `GAME_STATE_PLAYING` somente com outra borda de START. O prompt começa visível e alterna a cada 30 atualizações. As bordas vêm da máscara existente `current & ~previous`, portanto START mantido não produz outra borda na title screen.

O loop dos estados iniciais fica em `game_init()`. Os pools de gameplay e a primeira imagem de OAM só são criados na entrada de `PLAYING`; então `game_init()` retorna e o hot path já medido continua sem despacho recorrente de estado. Isso preserva o orçamento com 12 Bats. Um novo estado pré-run é uma extensão pequena de `game_flow` e do coordenador frio. Pause, level-up ou game over que interrompam uma run exigirão um dispatcher medido; essa arquitetura não foi antecipada antes do marco correspondente.

A atualização de 11 tiles é solicitada pelo C e consumida pela NMI seguinte. A NMI lê `PPUSTATUS` para reiniciar o latch compartilhado de `$2005/$2006`, aponta `PPUADDR` para `$220A`, escreve o prompt inteiro visível ou vazio e restaura o scroll. Ela não alterna `PPUCTRL` nem `PPUMASK`, portanto a renderização permanece estável. Trocas completas continuam sendo cortes diretos e podem ocupar vários frames de vídeo com renderização desligada.

O valor fixo de `PPUCTRL` é `$90`: bit 7 habilita NMI, bit 4 seleciona a pattern table 1 do background (`CHR $1000-$1FFF`) e o bit 3 limpo seleciona a pattern table 0 de sprites (`CHR $0000-$0FFF`). Os bytes da nametable continuam sendo índices de tile de 8 bits; o bit da PPU escolhe em qual metade de 4 KiB eles serão buscados.

Como o DMA de OAM ocorre antes dessa reconstrução no loop principal, uma shadow
recém-construída torna-se visível na NMI seguinte. Os testes de runtime, portanto,
amostram os limites das fases de movimento um frame depois; esse é o pipeline de
renderização intencional de um frame, não uma leitura de controle perdida nem uma
atualização extra de gameplay.

O layout de bits do controle é A, B, Select, Start, Up, Down, Left e Right nos bits 7 a 0. O DMC está desabilitado, portanto o DMA não corrompe a leitura serial do controle. Direções opostas em um mesmo eixo se anulam naquele eixo. Um movimento puramente vertical seleciona a animação de movimento de acordo com a orientação horizontal lembrada. As diagonais atualizam ambos os eixos sem normalização.

O comportamento da OAM é determinístico: todas as 64 entradas são enviadas por DMA a cada NMI. A inicialização oculta todas as entradas uma vez; cada construção seguinte oculta somente as entradas usadas no frame anterior e, em seguida, as chamadas de renderização por ordem de chegada recebem prioridade. O Soldier controlado consome atualmente sete entradas, embora sua área lógica seja de 3x3 tiles, pois os tiles transparentes foram omitidos pelo exportador. A integração do player mantém a mesma âncora de 24 pixels para ambas as orientações e espelha o metasprite atual no momento da renderização quando o Soldier olha para a esquerda, portanto não existe mais um deslocamento separado para movimento à esquerda.

A espada ataca em um período fixo de 60 frames e permanece ativa nos primeiros 12 frames de cada período. Seu frame de 8x16 é centralizado verticalmente na área lógica de 24 pixels do jogador, ancorado em `player.x + 24` ao olhar para a direita ou `player.x - 8` ao olhar para a esquerda, e invertido horizontalmente para a esquerda. Uma espada completamente fora da tela é omitida para evitar que coordenadas OAM sem sinal a façam reaparecer na borda oposta. Sua hitbox usa o mesmo estado ativo, âncora e regras de borda; Bats sobrepostos são removidos.

## Pool de Bats e spawn

O primeiro Bat vence o timer após 120 frames de gameplay. Spawns seguintes bem-sucedidos reiniciam o timer em 120 frames (dois segundos). As posições usam RNG determinístico e uma das quatro bordas. Com 12 slots ativos, o spawn vencido tenta novamente em outro frame sem sobrescrever memória.

As posições usam coordenadas de pixel armazenadas em bytes. Um acumulador Q4 compartilhado avança seis subpixels por atualização e emite um passo de um pixel ao atingir 16 subpixels, preservando a velocidade média de 0,375 pixel por eixo por atualização. A velocidade fica abaixo do passo de um pixel do player e segue a convenção diagonal sem normalização. O estado cosmético da animação também é compartilhado por todos os Bats. Um limite de varredura que encolhe mantém o custo proporcional aos slots usados.

Cada Bat armazena um byte de orientação horizontal. O spawn inicializa esse
valor pela posição X do Bat em relação ao alvo atual; depois, perseguição ou
separação horizontal o atualizam, enquanto movimento puramente vertical o
preserva. A arte original olha para a direita; para olhar à esquerda, o render
troca os tiles esquerdo/direito do metasprite fixo de dois sprites e alterna o
bit de flip horizontal de cada tile.
O caminho especializado usa uma ramificação de orientação por Bat e mantém as
mesmas duas entradas de OAM.

A separação usa diferenças entre as coordenadas do canto superior esquerdo e
uma caixa de proximidade ajustável de 12x6 pixels. Um par ativo rotativo é
inspecionado em cada frame sem passo de posição Q4. Quando está próximo, o
resultado guardado substitui a perseguição apenas no eixo de maior separação no
próximo passo; a perseguição continua no outro eixo. Os dois Bats se movem em
direções opostas, sobreposições exatas usam a ordem dos índices do pool e os
limites da arena saturam o resultado. O cursor visita todos os pares de slots
usados sem concentrar um pico O(n²) em um frame. Com 12 slots usados, uma
varredura completa dos 66 pares leva até cerca de 106 frames de gameplay; esta
primeira versão prioriza deliberadamente CPU previsível em vez de resposta
rígida imediata.

A colisão compara as AABBs 16x8 dos Bats com a AABB 8x16 da espada somente durante frames ativos. Índices pares e ímpares alternam, portanto cada Bat é verificado ao menos a cada dois frames ativos, reduzindo pela metade o pico da colisão. Um acerto cria uma gema centralizada antes de liberar o slot do Bat. HP e dano no player não foram implementados.

A inspeção da saída do cc65 e dos contadores de frame no Mesen identificou a indexação repetida de structs com 16 bits, o estado de animação por inimigo e as chamadas genéricas de metasprite como caminho crítico. O pool agora usa arrays compactos de bytes, temporização compartilhada e um renderizador limitado aos dois sprites do Bat, ainda em C. O teste atual de 1.750 frames compensa as telas iniciais, alcança os 12 slots e registra 1.735 NMIs/updates de gameplay após a baseline pós-transição, sem perda. Nenhuma rotina em Assembly foi necessária.

## Dados de animação e reutilização

`AnimationData` mantém as tabelas imutáveis de sprites, frames e animações separadas do `AnimationPlayer` de três bytes. Ele não possui conhecimento sobre entrada do controle, estado do jogador ou OAM. Da mesma forma, `OamRenderer` aceita quaisquer registros de metasprite gerados e não possui dependência do player. Inimigos, NPCs e itens coletáveis podem, portanto, ter seu próprio estado de reprodução compacto e chamar o mesmo renderizador sem duplicar a lógica de controle ou política de personagens.

As exportações em JSON permanecem apenas como referência de criação e não são interpretadas pela ROM. A regeneração requer a reconsolidação de nomes/offsets em `src/soldier_animation_data.c`; nenhuma estrutura de controle de gameplay contém tiles de frames codificados diretamente (hardcoded). O módulo genérico `player` seleciona atualmente `soldier_animation_data` em seu limite de integração de personagem e depende do espelhamento em runtime para a orientação para a esquerda; nenhum registro de personagens ou sistema de seleção existe no momento.

## RNG determinístico

`xorshift16` utiliza dois bytes de estado e deslocamentos `(7, 9, 8)`. Uma semente (seed) zero é normalizada para um porque zero é o estado absorvente do algoritmo. O gerador é rápido e reproduzível, mas não é criptográfico. Os fluxos de gameplay e cosméticos devem ser separados posteriormente se o consumo compartilhado impedir testes reproduzíveis.

## Limites incrementais para marcos futuros

Sistemas futuros devem ser adicionados apenas quando seus respectivos marcos (milestones) exigirem:

- definições imutáveis de personagens separadas do estado de personagem por partida (run);
- definições imutáveis de armas e slots compactos em tempo de execução para armas automáticas;
- pool fixo de projéteis e integração de progressão para os pools existentes de inimigos e gemas;
- definições de arena e de ondas (waves) orientadas a tabelas;
- camadas de elegibilidade, raridade e aplicação para melhorias (upgrades);
- objetivos de desbloqueio e persistência versionada por senhas (passwords).

O conteúdo deve utilizar IDs compactos e índices de array, e não ponteiros de posse ou alocação dinâmica na heap. As tabelas de definição permanecem imutáveis; o estado mutável da partida permanece em pools de tamanho fixo. Adicionar um personagem, arma, inimigo ou fase deve adicionar uma entrada na tabela e introduzir código especializado apenas para comportamentos genuinamente distintos.

Nenhum módulo futuro vazio ou estrutura especulativa de runtime existe no momento. Isso mantém o mapa do linker fidedigno e cada alteração futura passível de revisão.

## Pool de gemas de XP

O pool possui oito slots fixos. Cada slot guarda X/Y em bytes, flag ativa e uma
contagem de drops representados de 16 bits: cinco bytes por slot, 40 no total.
Dois bytes compartilhados guardam limite de varredura e cursor de coleta. A
alocação reutiliza o primeiro slot inativo. Com todos ocupados, o drop incrementa
a contagem da gema mais próxima por distância de Chebyshev, sem heap,
multiplicação ou busca recorrente de proximidade.

Uma gema ativa é testada por frame contra a AABB lógica 24x24 do player; com o
pool cheio, o desaparecimento pode levar no máximo oito frames após o contato.
A coleta zera a contagem, mas ainda não concede XP. As gemas são renderizadas
depois dos inimigos para preservar a prioridade de player, espada e ameaças;
cada uma usa um sprite, tile CHR `$14` e paleta 3.

