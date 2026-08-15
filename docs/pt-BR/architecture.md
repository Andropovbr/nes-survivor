# Arquitetura

## Módulos atuais

- `src/crt0.s` contém o cabeçalho iNES, caminho de reset, inicialização de RAM e PPU, inicialização do runtime do cc65, habilitação da renderização e vetores de interrupção.
- `src/nmi.s` é o tratador de NMI delimitado. Ele faz o upload da página shadow de OAM, restaura o scrolling em zero e avança o contador de frames.
- `src/nes.s` gerencia a alocação de OAM alinhada à página, a primitiva de espera de frame e a rotina de leitura da porta de controle.
- `src/main.c` orquestra a inicialização e o loop principal sincronizado.
- `src/game.c` gerencia a transição explícita de `BOOT` para `RUNNING` e orquestra a atualização do jogador juntamente com a reconstrução determinística da OAM.
- `src/input.c` deriva as máscaras de botões atuais, pressionados e soltos a partir da amostragem direta do hardware.
- `src/rng.c` implementa o estado determinístico de `xorshift16` e suas funções de geração.
- `src/player.c` gerencia o estado mutável e compacto do jogador, movimentação delimitada em 8 direções, orientação horizontal, seleção de animação e política de renderização do jogador.
- `src/animation.c` é um reprodutor de frames reutilizável e orientado a dados. Ele armazena apenas o ID da animação, frame local e temporizador de contagem regressiva; durações geradas controlam a repetição (looping), e apenas a alteração de animação reinicia a reprodução para o frame zero.
- `src/metasprite.c` oculta entradas não utilizadas da OAM e expande registros de tiles relativos com sinal para a shadow de OAM existente. Seu espelhamento horizontal opcional ajusta tanto a geometria quanto o bit de inversão (flip) de hardware.
- `src/soldier_animation_data.c` consolida as exportações de idle e caminhada do Soldier geradas separadamente sob os símbolos `soldier`. Os 21 registros de tiles, 3 frames e 2 definições mantêm seus valores gerados; apenas os offsets agregados e nomes foram alterados.
- `src/weapon_sword.c` mantém os dois bytes de estado da espada automática e o frame gerado exato de dois tiles. A espada é renderizada depois do jogador, à frente da orientação horizontal lembrada, e ataques completamente fora da tela são omitidos nas bordas da arena.
- `include/tuning.h` contém a posição inicial do jogador, velocidade, limites lógicos de 24x24 e capacidades arquiteturais iniciais.

## Limite de nomenclatura entre Player e Soldier

`player` representa a entidade em tempo de execução controlada através do controle 1. `PlayerState`, `PlayerFacing`, a API `player_*` e os limites de posição/movimentação `PLAYER_*` permanecem, portanto, independentes de personagem. `AnimationPlayer` também permanece genérico: é um cursor de reprodução de animação reutilizável, não a identidade do personagem jogável.

`soldier` representa a arte concreta atualmente vinculada a essa entidade em tempo de execução. `soldier_animation_data`, `SOLDIER_ANIMATION_*`, as tabelas internas de `soldier_animation_sprites`/frames/definitions e `soldier_sprite_palette` são específicas do asset. O módulo de player é o único ponto de integração em C que seleciona as definições do Soldier e espelha o metasprite atual quando a orientação é para a esquerda.

O banco de 8 KiB de `assets/game.chr` é vinculado através de `src/chr.s`. Os sprites do Soldier utilizam a pattern table `$0000`, correspondendo aos índices de tiles gerados `$00-$07`; a espada utiliza os índices gerados `$08-$09` na mesma tabela. Os backgrounds utilizam `$1000`, cujo tile zero está em branco, de modo que uma nametable limpa permaneça preta. A inicialização carrega a paleta de sprites de 16 bytes do Soldier em `$3F10-$3F1F` a partir da constante `soldier_sprite_palette` enquanto a renderização e a NMI estão desabilitadas; os registros de metasprite do Soldier selecionam a paleta de sprites 0.

## Limite entre C e Assembly

O C gerencia políticas, transições de estado e lógica pura. O Assembly é restrito à sequência de reset do NES, processamento de interrupções, I/O mapeado em memória, DMA de OAM, espera de frame e leitura serial do controle. Cada ponto de entrada em Assembly chamável a partir do C documenta sua ABI em `include/nes.h` e em sua respectiva implementação.

A lógica de gameplay deve permanecer em C a menos que a inspeção do código gerado ou medições no emulador identifiquem um gargalo concreto. Uma rotina executar a cada frame não é, por si só, motivo para movê-la para Assembly.

## Ciclo de vida do frame

1. O reset desabilita a renderização e as fontes de interrupção, aguarda a estabilização da PPU, limpa todos os 2 KiB da RAM interna e inicializa a stack de software do cc65.
2. Com a renderização desabilitada, a inicialização limpa `$2000-$2FFF`, preenche todas as entradas de paleta com o preto do NES (`$0F`), preenche a shadow de OAM com `$FF`, inicializa o runtime de C e habilita a NMI juntamente com a renderização de background/sprites.
3. A rotina de NMI preserva A/X/Y, realiza um DMA de OAM de 256 bytes a partir de `$0200`, restaura o scrolling para zero, incrementa um contador de frames de 8 bits na zero page, restaura os registradores e retorna. O processamento no pior caso é de aproximadamente 583 ciclos de CPU, incluindo a entrada da interrupção, situando-se confortavelmente dentro dos cerca de 2.273 ciclos do VBlank em NTSC.
4. `nes_wait_frame` captura uma cópia instantânea (snapshot) do contador e aguarda até que a NMI o altere. Uma comparação de 8 bits é atômica no 6502; o estouro de ciclo (wraparound) é seguro porque 256 NMIs não podem ocorrer entre a captura e a comparação.
5. O loop principal lê o controle 1, atualiza a política, a animação do jogador e os temporizadores da espada automática, oculta as entradas antigas da OAM e, em seguida, emite os sete sprites do jogador seguidos pelos dois sprites opcionais da espada. Esse trabalho ocorre fora da NMI e é concluído imediatamente após a sincronização de frames.

Como o DMA de OAM ocorre antes dessa reconstrução no loop principal, uma shadow
recém-construída torna-se visível na NMI seguinte. Os testes de runtime, portanto,
amostram os limites das fases de movimento um frame depois; esse é o pipeline de
renderização intencional de um frame, não uma leitura de controle perdida nem uma
atualização extra de gameplay.

O layout de bits do controle é A, B, Select, Start, Up, Down, Left e Right nos bits 7 a 0. O DMC está desabilitado, portanto o DMA não corrompe a leitura serial do controle. Direções opostas em um mesmo eixo se anulam naquele eixo. Um movimento puramente vertical seleciona a animação de movimento de acordo com a orientação horizontal lembrada. As diagonais atualizam ambos os eixos sem normalização.

O comportamento da OAM é determinístico: todas as 64 entradas são enviadas por DMA a cada NMI; a construção começa ocultando todas as entradas e, em seguida, as chamadas de renderização por ordem de chegada recebem prioridade. O Soldier controlado consome atualmente sete entradas, embora sua área lógica seja de 3x3 tiles, pois os tiles transparentes foram omitidos pelo exportador. A integração do player mantém a mesma âncora de 24 pixels para ambas as orientações e espelha o metasprite atual no momento da renderização quando o Soldier olha para a esquerda, portanto não existe mais um deslocamento separado para movimento à esquerda.

A espada ataca em um período fixo de 60 frames e permanece visível nos primeiros 12 frames de cada período. Seu frame de 8x16 é centralizado verticalmente na área lógica de 24 pixels do jogador, ancorado em `player.x + 24` ao olhar para a direita ou `player.x - 8` ao olhar para a esquerda, e invertido horizontalmente para a esquerda. Uma espada completamente fora da tela é omitida para evitar que coordenadas OAM sem sinal a façam reaparecer na borda oposta. Ainda não há hitbox ou dano.

## Dados de animação e reutilização

`AnimationData` mantém as tabelas imutáveis de sprites, frames e animações separadas do `AnimationPlayer` de três bytes. Ele não possui conhecimento sobre entrada do controle, estado do jogador ou OAM. Da mesma forma, `OamRenderer` aceita quaisquer registros de metasprite gerados e não possui dependência do player. Inimigos, NPCs e itens coletáveis podem, portanto, ter seu próprio estado de reprodução compacto e chamar o mesmo renderizador sem duplicar a lógica de controle ou política de personagens.

As exportações em JSON permanecem apenas como referência de criação e não são interpretadas pela ROM. A regeneração requer a reconsolidação de nomes/offsets em `src/soldier_animation_data.c`; nenhuma estrutura de controle de gameplay contém tiles de frames codificados diretamente (hardcoded). O módulo genérico `player` seleciona atualmente `soldier_animation_data` em seu limite de integração de personagem e depende do espelhamento em runtime para a orientação para a esquerda; nenhum registro de personagens ou sistema de seleção existe no momento.

## RNG determinístico

`xorshift16` utiliza dois bytes de estado e deslocamentos `(7, 9, 8)`. Uma semente (seed) zero é normalizada para um porque zero é o estado absorvente do algoritmo. O gerador é rápido e reproduzível, mas não é criptográfico. Os fluxos de gameplay e cosméticos devem ser separados posteriormente se o consumo compartilhado impedir testes reproduzíveis.

## Limites incrementais para marcos futuros

Sistemas futuros devem ser adicionados apenas quando seus respectivos marcos (milestones) exigirem:

- definições imutáveis de personagens separadas do estado de personagem por partida (run);
- definições imutáveis de armas e slots compactos em tempo de execução para armas automáticas;
- pools de tamanho fixo para inimigos, projéteis e XP com comportamento de saturação documentado;
- definições de arena e de ondas (waves) orientadas a tabelas;
- camadas de elegibilidade, raridade e aplicação para melhorias (upgrades);
- objetivos de desbloqueio e persistência versionada por senhas (passwords).

O conteúdo deve utilizar IDs compactos e índices de array, e não ponteiros de posse ou alocação dinâmica na heap. As tabelas de definição permanecem imutáveis; o estado mutável da partida permanece em pools de tamanho fixo. Adicionar um personagem, arma, inimigo ou fase deve adicionar uma entrada na tabela e introduzir código especializado apenas para comportamentos genuinamente distintos.

Nenhum módulo futuro vazio ou estrutura especulativa de runtime existe no momento. Isso mantém o mapa do linker fidedigno e cada alteração futura passível de revisão.

