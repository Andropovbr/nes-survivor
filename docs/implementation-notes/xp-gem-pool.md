# Pool e coleta visual das gemas de XP

## Problema observado

O Bat desaparecia ao tocar a hitbox da espada, sem deixar um objeto no ponto da
derrota. A nova arte reservou o tile `$14` da pattern table de sprites para uma
gema 8x8. Este marco precisava mostrar o drop e removê-lo ao contato com o
player, sem antecipar nível, barra ou ganho de XP.

Também era necessário limitar memória e sprites. Uma lista crescente não cabe
no NES, e descartar um drop quando o pool enche contraria a regra do projeto de
não perder XP por saturação.

## Solução e fluxo

`xp_gem` mantém oito slots fixos. A morte centraliza a gema no Bat 16x8 antes de
liberar o inimigo:

```c
xp_gem_spawn(
    (uint8_t)(bat_x +
              (BAT_WIDTH_PIXELS - XP_GEM_WIDTH_PIXELS) / 2U),
    bat_y);
enemy_active[index] = 0U;
```

Um slot guarda X, Y, atividade e `gem_drop_units`. Essa contagem não é XP do
player; ela registra quantos drops foram condensados no sprite. Quando existem
oito gemas visíveis, o novo drop incrementa a gema mais próxima pela distância
de Chebyshev, calculada apenas com diferenças e comparação:

```c
return delta_x > delta_y ? delta_x : delta_y;
```

Isso evita heap, multiplicação e uma busca recorrente. Ao tocar o player, a
gema e sua contagem desaparecem. Nenhuma variável de XP, nível ou progressão é
alterada neste marco.

## Coleta escalonada

Verificar oito AABBs por frame causou regressão no cenário de 12 Bats. O cursor
`collection_scan_index` testa somente um slot por frame. A latência máxima com
o pool cheio é de oito frames (aproximadamente 133 ms em NTSC), mantendo custo
limitado e previsível.

A carga ainda excedia o frame nos momentos de espada ativa. A colisão dos Bats
passou então a alternar índices pares e ímpares:

```c
index = sword_hitbox_scan_parity;
sword_hitbox_scan_parity ^= 1U;
for (; index < pool_high_water; index = (uint8_t)(index + 2U)) {
    /* mesma AABB e mesmo efeito de derrota */
}
```

Cada Bat continua sendo verificado em no máximo dois frames ativos; a espada
permanece ativa por 12 frames. A solução ficou em C porque a redução algorítmica
recuperou o orçamento e não houve justificativa medida para Assembly.

## Restrições do NES e prioridades

- pool: 8 gemas;
- RAM: 40 bytes nos slots e 2 bytes compartilhados;
- sprite: 1 por gema, tile `$14`, paleta 3;
- pior caso de OAM: 7 player + 2 espada + 24 Bats + 8 gemas = 41/64;
- ordem: player, espada, inimigos e gemas;
- comportamento cheio: condensar na gema ativa aproximada mais próxima;
- coleta no fim da wave: ainda não existe wave transition.

A prioridade mantém jogador e ameaças antes dos drops. Ainda pode haver flicker
se muitos objetos ocuparem a mesma scanline, pois o PPU mostra no máximo oito
sprites por linha.

## Desempenho medido

Cenário: teste Mesen de 1.750 frames, movimentação determinística e saturação de
12 Bats.

- baseline antes das gemas: 1.735 NMIs, 1.735 updates, 0 perdidos;
- primeira integração: 1.735 NMIs, 1.727 updates, 8 perdidos;
- coleta escalonada isoladamente: 1.735 NMIs, 1.727 updates, 8 perdidos;
- colisão par/ímpar final: 1.735 NMIs, 1.735 updates, 0 perdidos.

Não foi feita medição individual de ciclos das rotinas. Não há alegação de
percentual de ganho. O stress final também observa ao menos uma gema no OAM.

## Memória e ROM medidas

Dados do linker map final:

- BSS: 81 -> 124 bytes (`+43`);
- PRG-ROM usado: 7.043 -> 7.994 bytes (`+951`);
- zero page: 28 bytes, inalterada;
- DATA: 37 bytes, inalterada;
- CHR-ROM: 8 KiB, inalterada como capacidade;
- tiles de sprite com conteúdo: 20 -> 21 no asset novo; a gema usa `$14`.

## O que observar no Mesen ou hardware

1. Derrotar um Bat com a espada e observar uma gema 8x8 centralizada na posição.
2. Encostar o metasprite do player e confirmar o desaparecimento em até oito
   frames.
3. No PPU Viewer, confirmar sprites na pattern table 0 e a fonte maiúscula na
   pattern table 1.
4. No OAM Viewer, confirmar tile `$14`, atributo de paleta 3 e prioridade após
   os inimigos.
5. Com muitos Bats/gemas na mesma linha, observar possível flicker do limite de
   oito sprites por scanline, sem corrupção da OAM.

## Limitações e evoluções

- a gema possui um único frame no JSON atual; não há mudança visual entre frames;
- o contato pode levar até oito frames para ser processado;
- a contagem condensada é descartada na coleta porque ganho de XP foi
  explicitamente adiado;
- ainda faltam XP acumulado, requisitos de nível, HUD, level-up e coleta ao fim
  da wave;
- uma futura integração de XP deve consumir `gem_drop_units` antes de liberar o
  slot e testar overflow conforme o limite definitivo da run.
