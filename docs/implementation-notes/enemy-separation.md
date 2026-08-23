# Separação entre morcegos

## Problema

Antes desta mudança, cada morcego comparava sua posição diretamente com a do
player e avançava um pixel em X, Y ou nos dois eixos quando o acumulador Q4
emitia um passo de movimento. Todos perseguiam essencialmente o mesmo ponto.

Esse comportamento era barato e previsível, mas fazia vários morcegos
convergirem para coordenadas muito parecidas. Como não existia nenhuma reação à
posição dos outros inimigos, eles podiam permanecer visualmente empilhados.

## Solução adotada

A solução mantém a perseguição original e acrescenta uma separação suave entre
pares próximos. A proximidade é uma caixa retangular calculada com diferenças
inteiras de X e Y. Não há distância euclidiana, raiz quadrada, ponto flutuante,
divisão, física ou estrutura espacial.

Para limitar o pico de CPU no 6502, a versão atual não compara todos os pares no
mesmo frame. Um cursor percorre os pares possíveis ao longo do tempo. Em cada
frame sem passo de posição Q4, apenas um par é inspecionado. Se o par estiver
próximo, uma posição de separação fica pendente e é aplicada no próximo passo
Q4.

Somente um eixo é substituído pela separação. O outro continua perseguindo o
player, permitindo que o morcego se afaste lateralmente enquanto ainda avança
na direção do alvo.

## Como funciona

1. O acumulador compartilhado soma `BAT_MOVEMENT_SPEED_SUBPIXELS` a cada update.
2. Quando ainda não existe um passo inteiro, o cursor seleciona um par de slots.
3. Slots inativos são ignorados.
4. As diferenças absolutas de X e Y são comparadas com os limites de tuning.
5. Um par próximo escolhe apenas um eixo de separação.
6. As duas posições resultantes são limitadas às bordas da arena e guardadas.
7. No próximo passo Q4, todos os morcegos executam a perseguição normal.
8. Se ainda for válido, o resultado pendente sobrescreve somente o eixo de
   separação dos dois morcegos.
9. Uma separação horizontal também atualiza a orientação visual dos morcegos.

Uma nova proximidade encontrada antes do próximo passo pode substituir o
resultado pendente anterior. Portanto, a implementação não soma forças de
vários vizinhos: ela aplica no máximo uma interação de par por passo inteiro.

## Trechos importantes do código

### Detecção barata de proximidade

Arquivo: `src/enemy.c`, função `enemy_update`.

```c
delta_x = enemy_x_positions[index] < enemy_x_positions[other]
              ? (uint8_t)(enemy_x_positions[other] -
                          enemy_x_positions[index])
              : (uint8_t)(enemy_x_positions[index] -
                          enemy_x_positions[other]);
delta_y = enemy_y_positions[index] < enemy_y_positions[other]
              ? (uint8_t)(enemy_y_positions[other] -
                          enemy_y_positions[index])
              : (uint8_t)(enemy_y_positions[index] -
                          enemy_y_positions[other]);
```

As posições são bytes e as subtrações só acontecem na ordem que produz um
resultado não negativo. Isso obtém valores absolutos sem função matemática ou
conversão para tipos maiores.

```c
if (delta_x < BAT_SEPARATION_X_PIXELS &&
    delta_y < BAT_SEPARATION_Y_PIXELS) {
    pending_separation = 1U;
    pending_separation_index = index;
    pending_separation_other = other;
    pending_separation_on_x = (uint8_t)(
        (delta_x == 0U && delta_y == 0U) ||
        delta_x >= delta_y);
}
```

O par só é considerado próximo quando está dentro dos dois limites. O eixo X é
escolhido em uma sobreposição exata ou quando `delta_x >= delta_y`; nos demais
casos a separação usa Y.

### Desempate determinístico e limites da arena

Arquivo: `src/enemy.c`, função `enemy_update`.

```c
if (enemy_x_positions[index] <= enemy_x_positions[other]) {
    pending_separation_index_position =
        enemy_x_positions[index] > BAT_MIN_X
            ? (uint8_t)(enemy_x_positions[index] - 1U)
            : BAT_MIN_X;
    pending_separation_other_position =
        enemy_x_positions[other] < BAT_MAX_X
            ? (uint8_t)(enemy_x_positions[other] + 1U)
            : BAT_MAX_X;
}
```

O cursor sempre forma o par com `index < other`. Quando ambos têm o mesmo X e
Y, a regra anterior escolhe X e este `<=` faz o menor índice tentar ir para a
esquerda e o maior para a direita. A regra é estável e não consome RNG. Os
testes contra `BAT_MIN_X` e `BAT_MAX_X` impedem underflow e overflow. O eixo Y
usa a mesma forma de saturação.

### Perseguição e separação coexistindo

Arquivo: `src/enemy.c`, função `enemy_update`.

```c
if (enemy_x_positions[index] < target_x) {
    ++enemy_x_positions[index];
    enemy_facing_right[index] = 1U;
} else if (enemy_x_positions[index] > target_x) {
    --enemy_x_positions[index];
    enemy_facing_right[index] = 0U;
}
if (enemy_y_positions[index] < target_y) {
    ++enemy_y_positions[index];
} else if (enemy_y_positions[index] > target_y) {
    --enemy_y_positions[index];
}
```

Esse continua sendo o movimento principal. Depois do loop, se a separação
pendente for horizontal, somente X é sobrescrito; se for vertical, somente Y é
sobrescrito. Assim, o eixo não selecionado conserva o passo de perseguição e o
movimento pode continuar diagonal.

### Trabalho distribuído entre frames

Arquivo: `src/enemy.c`, função `enemy_update`.

```c
if (separation_pair_other >= pool_high_water) {
    ++separation_pair_index;
    separation_pair_other =
        (uint8_t)(separation_pair_index + 1U);
    if (separation_pair_other >= pool_high_water) {
        separation_pair_index = 0U;
        separation_pair_other = 1U;
    }
}
index = separation_pair_index;
other = separation_pair_other;
++separation_pair_other;
```

O cursor enumera pares com `index < other`, avança `other` e passa para o próximo
`index` ao atingir o high-water mark. Esse bloco só roda dentro do caminho
`move_positions == 0U`: a comparação acontece em um frame sem movimento inteiro,
e a aplicação acontece no passo Q4 seguinte.

## Parâmetros de tuning

Arquivo: `include/tuning.h`.

```c
#define BAT_POSITION_SUBPIXELS_PER_PIXEL 16U
#define BAT_MOVEMENT_SPEED_SUBPIXELS      6U
#define BAT_SEPARATION_X_PIXELS          12U
#define BAT_SEPARATION_Y_PIXELS           6U
```

Os limites 12x6 usam as coordenadas do canto superior esquerdo dos morcegos. O
Bat atual ocupa 16x8 pixels, então os valores detectam uma sobreposição visual
considerável sem funcionar como colisão rígida. A separação usa o mesmo passo
inteiro emitido pelo movimento Q4; não existe uma segunda velocidade.

## Decisões e trade-offs

- A solução usa o pool e os índices existentes, sem alocação dinâmica.
- A caixa retangular é menos precisa que uma distância circular, mas muito mais
  barata no 6502 e suficiente para reduzir empilhamentos persistentes.
- A escolha por um único eixo mantém o código simples e deixa o outro eixo livre
  para perseguir o player.
- Não existe steering acumulado. Em grupos densos, diferentes pares são
  corrigidos gradualmente.
- Não foi criada grade espacial porque o limite atual é de apenas 12 inimigos e
  a medição mostrou que distribuir comparações simples já atende ao frame.
- A separação não altera OAM, quantidade de sprites ou a política de render.

## Performance e memória

Com `n` slots usados, existem `n * (n - 1) / 2` pares. Portanto, uma varredura
completa continua sendo O(n²), mas esse custo é distribuído. Com os 12 slots
usados existem 66 pares. Pela cadência Q4 atual, o ciclo completo pode levar
cerca de 106 frames de gameplay.

Medições registradas durante o desenvolvimento:

- comparar os 66 pares no mesmo passe perdeu 249 updates no teste de estresse;
- limitar o passe a 12 pares perdeu 107 updates;
- limitar a quatro pares perdeu 28 updates;
- a versão atual, com uma comparação em frame sem passo Q4, atingiu 12 Bats com
  1.696 NMIs e 1.696 updates: zero updates perdidos.

Esses números são medições do teste `tests/mesen_bat_stress.lua`, não estimativas
de ciclos. A lógica roda fora da NMI.

A separação acrescenta oito bytes de BSS compartilhado: dois bytes para o
cursor e seis para o resultado pendente. Não existe custo de separação por
morcego. O pool atual tem outros 12 bytes de orientação horizontal, adicionados
posteriormente pelo comportamento de facing e não pelo algoritmo de separação.

## O que observar no jogo

No Mesen, vale demonstrar:

- vários morcegos convergindo para um player parado;
- a redução gradual de pilhas, em vez de uma colisão rígida instantânea;
- um morcego afastando-se em X enquanto continua perseguindo em Y, ou o inverso;
- o desempate de morcegos exatamente sobrepostos;
- o comportamento próximo das quatro bordas da arena;
- grupos com 12 morcegos e espada ativa, verificando estabilidade do frame;
- sobreposições momentâneas, que são esperadas nesta versão.

Também é importante observar a limitação de oito sprites por scanline do NES.
A separação tende a reduzir coincidências, mas não substitui flicker management
ou uma futura política de OAM cycling.

## Possíveis evoluções

- Ajustar os limites 12x6 após comparação visual no Mesen ou em hardware.
- Revisar quantos pares são processados somente se medições futuras mostrarem
  folga ou necessidade de resposta mais rápida.
- Considerar uma abordagem espacial apenas se o limite de inimigos crescer e a
  varredura distribuída deixar de ser suficiente.

Não há motivo medido, no estado atual, para introduzir steering, física ou
Assembly nessa rotina.
