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
| `$0325-$038B` | 103 | globais BSS de C |
| `$038C-$04FF` | 372 | RAM geral livre |
| `$0500-$07FF` | 768 | stack de parâmetros do cc65 |

A RAM estática/reservada soma 1.448 de 2.048 bytes, deixando 600 bytes livres:
228 na zero page e 372 na RAM geral. Os intervalos de stack são reservas, não
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
| Pool de Bats | 84 | BSS, 12 entradas x 7 bytes |
| Timer/limite de varredura dos Bats | 3 | BSS |

Cada Bat armazena X/Y Q12.4, flag ativa, frame e timer do frame. A alocação
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
| PRG-ROM | 5.787 bytes | 32.768 bytes | 17,66%; 26.981 bytes livres |
| CHR-ROM | 224 bytes atribuídos | 8.192 bytes | tiles `$00-$0D`; 7.968 vazios |
| Arquivo `.nes` | 40.976 bytes | 40.976 bytes | cabeçalho + PRG + CHR |

O PRG inclui 220 bytes de startup, 12 de construtores, 5.349 de código/runtime,
163 de RODATA, 37 de imagem DATA e seis de vetores. O RODATA contém 117 bytes de
animação do Soldier, oito registros da espada e 38 bytes de animação do Bat.

`assets/game.chr` é o banco anexado de 8 KiB: Soldier usa `$00-$07`, a espada
animada `$08-$09` e Bat `$0A-$0D`.

## Orçamento de tempo

A NMI continua limitada a um DMA de OAM e trabalho constante, cerca de 583
ciclos incluindo a entrada da interrupção. Gameplay, colisões e construção de
OAM rodam fora da NMI. Em 450 frames no Mesen com espada animada e dois eventos
de spawn, houve 446 NMIs após o startup e exatamente 446 atualizações de
gameplay, sem perda sincronizada. Um teste de estresse com 12 Bats no emulador
permanece como trabalho futuro.
