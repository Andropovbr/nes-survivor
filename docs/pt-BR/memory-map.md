# Mapa de memória e orçamentos

As medições vêm de `build/nes-survivor.map`, gerado pelo cc65 2.19 após integrar
a espada animada ao marco do primeiro inimigo Bat.

## Espaço de endereçamento da CPU e RAM interna

| Intervalo | Bytes | Finalidade atual |
| --- | ---: | --- |
| `$0000-$0001` | 2 | zero page deliberadamente não alocada |
| `$0002-$001D` | 28 | símbolos do projeto e do cc65 |
| `$001E-$00FF` | 226 | zero page livre |
| `$0100-$01FF` | 256 | reserva da stack de hardware do 6502 |
| `$0200-$02FF` | 256 | shadow de OAM, 64 sprites x 4 bytes |
| `$0300-$0324` | 37 | dados inicializados do cc65 |
| `$0325-$035E` | 58 | globais BSS de C |
| `$035F-$04FF` | 417 | RAM geral livre |
| `$0500-$07FF` | 768 | stack de parâmetros do cc65 |

A RAM estática/reservada soma 1.403 de 2.048 bytes, deixando 645 bytes livres:
228 na zero page e 417 na RAM geral. Os intervalos de stack são reservas, não
medições de pico.

## Estado mutável e pools

| Estado | Bytes | Localização/segmento |
| --- | ---: | --- |
| Contador NMI/temporário do controle | 2 | zero page `$0002-$0003` |
| Runtime de zero page do cc65 | 26 | zero page `$0004-$001D` |
| Estado do jogo e cursor de OAM | 2 | BSS |
| Entrada atual/pressionada/solta | 3 | BSS |
| Estado do RNG | 2 | BSS |
| Posição/orientação/animação do player | 7 | BSS |
| Timers de atividade/cooldown da espada | 2 | BSS |
| Pool de Bats | 36 | BSS, 12 entradas x 3 bytes |
| Estado compartilhado de spawn/movimento/animação | 6 | BSS |

Cada Bat armazena X/Y em pixels e uma flag ativa. Um acumulador Q4 compartilhado
gera passos inteiros, e um frame/timer compartilhado anima todos em sincronia. A alocação
reutiliza o primeiro slot inativo abaixo de um limite de varredura que encolhe.
Com 12 slots ocupados, o spawn vencido permanece pendente e tenta novamente;
nenhuma memória é sobrescrita e nenhum Bat agendado é perdido silenciosamente.

## Orçamento de OAM

A prioridade é determinística. Soldier usa 0-6. Durante os 12 frames ativos, a
espada usa 7-8; fora do ataque, os Bats começam em 7. Até 12 Bats usam dois
sprites cada, totalizando no pior caso 33/64 e deixando 31 ocultos. Bats
sobrepostos podem exceder oito sprites por scanline. Player e espada ativa
mantêm prioridade, mas ainda não há rotação de flicker dos inimigos.

## Uso do cartucho

| Região | Conteúdo utilizado | Capacidade | Notas |
| --- | ---: | ---: | --- |
| Cabeçalho iNES | 16 bytes | 16 bytes | mapper 0, NROM-256 |
| PRG-ROM | 5.126 bytes | 32.768 bytes | 15,64%; 27.642 bytes livres |
| CHR-ROM | 224 bytes atribuídos | 8.192 bytes | tiles `$00-$0D`; 7.968 vazios |
| Arquivo `.nes` | 40.976 bytes | 40.976 bytes | cabeçalho + PRG + CHR |

O PRG inclui 220 bytes de startup, 12 de construtores, 4.688 de código/runtime,
163 de RODATA, 37 de imagem DATA e seis de vetores. O RODATA contém 117 bytes de
animação do Soldier, oito registros da espada e 38 bytes de animação do Bat.

`assets/game.chr` é o banco anexado de 8 KiB: Soldier usa `$00-$07`, a espada
animada `$08-$09` e Bat `$0A-$0D`.

## Orçamento de tempo

A NMI continua limitada a um DMA de OAM e trabalho constante, cerca de 583
ciclos incluindo a entrada da interrupção. Gameplay, colisões e construção de
OAM rodam fora da NMI. A inicialização oculta as 64 entradas uma vez; cada frame
seguinte oculta somente as entradas utilizadas anteriormente.

Antes da otimização, o teste de 850 frames perdeu 171 atualizações após o terceiro
Bat. O teste final de 1.700 frames saturou os 12 slots e observou 1.696 NMIs após
o startup com exatamente 1.696 atualizações: nenhuma perda. A animação é
compartilhada e a renderização usa um caminho C especializado para o par de
sprites; Assembly não foi necessário.
