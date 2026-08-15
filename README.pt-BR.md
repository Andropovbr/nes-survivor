[English](README.md) | [Português (Brasil)](README.pt-BR.md)

# NES Survivor

O NES Survivor é um jogo de ação no estilo survivor em arena fixa, projetado para os limites de hardware do NES original. O projeto utiliza C para os sistemas de alto nível e Assembly 6502 focado para a inicialização do hardware e rotinas delimitadas de baixo nível.

## Status atual

A ROM agora inicializa em uma arena preta fixa com o Soldier, o primeiro personagem jogável, centralizado na tela. O jogador movimenta o Soldier em todas as oito direções do D-pad, mantém a última direção horizontal para a qual estava virado e utiliza uma animação gerada de um frame para idle e dois frames para movimento. As durações das animações, offsets com sinal de metasprites, índices de tiles e atributos de OAM são provenientes dos dados consolidados do png2chr-studio.

O Soldier ataca automaticamente com uma espada à frente da direção para a qual está olhando, uma vez a cada 60 frames. O frame gerado da espada permanece visível por 12 frames; a arma é apenas visual até a introdução dos sistemas de inimigos e colisão.

A base NROM continua realizando DMA de OAM delimitado na NMI e executa a lógica de controle, jogador, animação e construção de OAM no loop principal sincronizado em C. O módulo `player` representa qualquer personagem que o controle 1 assuma; símbolos concretos de gráficos e animação possuem o prefixo `soldier`.

## Requisitos

- Toolchain cc65 2.19 ou compatível (`cc65`, `ca65`, `ld65`, `cl65`, `sim65`)
- GNU Make para os comandos principais
- Python 3 para os testes de validação de cartucho
- Mesen 2 é recomendado para inspeção em tempo de execução (runtime)

Nenhum caminho de ferramenta é fixo no código (hard-coded). O build utiliza os executáveis disponíveis no `PATH`.

## Build e testes

```sh
make
make test
make test-runtime
make clean
```

A ROM é gerada em `build/nes-survivor.nes`; o mapa do linker e os labels são gerados ao lado dela. `make test` executa os testes de lógica em C através do `sim65` do cc65 e valida o cartucho iNES gerado com Python. `make test-runtime` é opcional e executa a ROM por 150 frames com o executor de testes Lua headless do Mesen 2.

No Windows, o `make` usa `python` para criar e limpar o diretório de build de
forma portável. Em sistemas Unix-like, utiliza `python3`. Sobrescreva `PYTHON`
ou `MESEN` apenas quando os executáveis não estiverem no `PATH`, por exemplo:

```powershell
make test-runtime MESEN="F:/Emuladores/NES/Mesen.exe"
```

Em sistemas Windows sem GNU Make, o fluxo de trabalho equivalente e verificado é:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1 build
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1 test
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1 runtime
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1 clean
```

`assets/game.chr` é o banco de 8 KiB de CHR-ROM sob controle de versão utilizado pelo build.

## Controles

O controle 1 é lido a cada frame. O D-pad movimenta um pixel por eixo por frame de jogo, incluindo diagonais. Esquerda e Direita atualizam a orientação horizontal; Cima e Baixo isolados a preservam. Soltar o D-pad retorna ao estado idle preservando a orientação. A espada ataca automaticamente e não exige botão. A, B, Select e Start são lidos, mas ainda não realizam nenhuma ação de gameplay.

## Plataforma alvo e limitações de hardware

- NROM-256 / Mapper 0, 32 KiB PRG-ROM e 8 KiB CHR-ROM
- Espelhamento horizontal de nametable (horizontal nametable mirroring)
- Premissa de temporização NTSC (60 frames por segundo)
- Uma tela fixa com scrolling travado em zero
- Sem áudio e sem adaptação de temporização para PAL/Dendy no momento
- Apenas um personagem e uma arma de espada visual; sem dano, inimigos, ondas (waves), HUD ou colisão no momento
- As diagonais intencionalmente utilizam a velocidade total de um pixel em ambos os eixos
- Não é necessária rotação para sprite flicker no momento; o jogador consome 7 slots de OAM e a espada utiliza mais 2 enquanto está visível

Os detalhes de arquitetura e frames estão em [docs/pt-BR/architecture.md](docs/pt-BR/architecture.md).
O uso de memória medido está em [docs/pt-BR/memory-map.md](docs/pt-BR/memory-map.md).
