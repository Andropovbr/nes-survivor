# Mapa de memória e orçamentos

As medições vêm de `build/nes-survivor.map`, gerado pelo cc65 2.19 após adicionar
a primeira máquina explícita de estados para as telas iniciais.

## Espaço de endereçamento da CPU e RAM interna

| Intervalo | Bytes | Finalidade atual |
| --- | ---: | --- |
| `$0000-$0001` | 2 | zero page deliberadamente não alocada |
| `$0002-$001D` | 28 | símbolos do projeto e do cc65 |
| `$001E-$00FF` | 226 | zero page livre |
| `$0100-$01FF` | 256 | reserva da stack de hardware do 6502 |
| `$0200-$02FF` | 256 | shadow de OAM, 64 sprites x 4 bytes |
| `$0300-$0324` | 37 | dados inicializados do cc65 |
| `$0325-$0375` | 81 | globais BSS de C |
| `$0376-$04FF` | 394 | RAM geral livre |
| `$0500-$07FF` | 768 | stack de parâmetros do cc65 |

A RAM estática/reservada soma 1.426 de 2.048 bytes, deixando 622 bytes livres:
228 na zero page e 394 na RAM geral. Os intervalos de stack são reservas, não
medições de pico.

## Estado mutável e pools

| Estado | Bytes | Localização/segmento |
| --- | ---: | --- |
| Contador NMI/temporário do controle | 2 | zero page `$0002-$0003` |
| Runtime de zero page do cc65 | 26 | zero page `$0004-$001D` |
| Cursor de OAM | 1 | BSS |
| Estado inicial, countdown e flag de blink | 3 | BSS |
| Pedido pendente de prompt para a NMI | 1 | BSS |
| Entrada atual/pressionada/solta | 3 | BSS |
| Estado do RNG | 2 | BSS |
| Posição/orientação/animação do player | 7 | BSS |
| Timers de atividade/cooldown da espada | 2 | BSS |
| Pool de Bats | 48 | BSS, 12 entradas x 4 bytes |
| Estado compartilhado de spawn/movimento/animação/separação | 14 | BSS |

Cada Bat armazena X/Y em pixels, uma flag ativa e a orientação horizontal. Um acumulador Q4 compartilhado
gera passos inteiros, e um frame/timer compartilhado anima todos em sincronia. A alocação
reutiliza o primeiro slot inativo abaixo de um limite de varredura que encolhe.
Com 12 slots ocupados, o spawn vencido permanece pendente e tenta novamente;
nenhuma memória é sobrescrita e nenhum Bat agendado é perdido silenciosamente.
Os oito bytes de separação guardam um cursor rotativo de pares e um resultado
pendente limitado a um eixo; não existe estado de separação por Bat.

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
| PRG-ROM | 7.043 bytes | 32.768 bytes | 21,49%; 25.725 bytes livres |
| CHR-ROM | 560 bytes de tiles com significado | 8.192 bytes | 14 tiles de sprite + 21 glifos não vazios |
| Arquivo `.nes` | 40.976 bytes | 40.976 bytes | cabeçalho + PRG + CHR |

O PRG inclui 220 bytes de startup, 12 de construtores, 6.549 de código/runtime,
219 de RODATA, 37 de imagem DATA e seis de vetores. A branch da máquina acrescenta 838
bytes de PRG e três bytes de BSS em relação à base; zero page, DATA,
OAM e stacks permanecem inalterados.

Os primeiros 4 KiB de `assets/game.chr` fornecem sprites: Soldier usa `$00-$07`,
a espada animada `$08-$09` e Bat `$0A-$0D`. `src/chr.s` fornece 21 glifos ASCII
não vazios na tabela de background `$1000`, nos próprios códigos dos caracteres;
o tile de espaço reservado e todos os patterns restantes ficam vazios.

## Orçamento de tempo

A NMI continua limitada a um DMA de OAM e trabalho constante, cerca de 590
ciclos incluindo a entrada quando não existe update de VRAM pendente. Mostrar
os 11 tiles do prompt usa aproximadamente 788 ciclos totais; ocultá-los custa
menos. Gameplay, colisões e construção de OAM rodam fora da NMI.

Antes da otimização, o teste de 850 frames perdeu 171 atualizações após o terceiro
Bat. O teste atual de 1.750 frames compensa as telas iniciais, satura os 12 slots
e observa 1.735 atualizações de gameplay sincronizadas por NMI após sua baseline
pós-transição, sem perdas. A animação é compartilhada e a renderização usa um
caminho C especializado para o par de sprites.

Um passe inicial com os 66 pares de separação perdeu 249 atualizações no mesmo
teste de estresse. Limites de 12 e depois quatro pares ainda perderam 107 e 28
atualizações. A versão final inspeciona um par em um frame sem passo Q4 e aplica
o resultado guardado no passo seguinte. O cenário atual com estados ainda
atinge os 12 Bats sem perder atualizações de gameplay.

Ao adicionar orientação por Bat, a primeira versão repetia o branch de render
para os dois tiles e perdeu quatro atualizações com 12 Bats e espada ativa. A
emissão do registro completo de dois sprites com um único branch restaurou o
resultado de estresse sem perdas com flip horizontal habilitado.

As trocas completas de nametable desabilitam intencionalmente NMI e renderização
durante a escrita de 1.024 bytes e do texto fixo. O runtime de 450 frames no
Mesen observou 435 NMIs; a diferença cobre a estabilização do reset e as trocas
delimitadas. O blink agora ocorre em uma transferência delimitada na NMI sem
alternar a renderização; os ciclos são estimativas pelo código gerado, não uma
medição do profiler de ciclos do emulador.
