# Dano por contato, invulnerabilidade e game over

## Problema e comportamento observável

Antes deste marco, Bats podiam atravessar a área lógica do Soldier sem afetar a
run. Não existiam HP, intervalo entre acertos, estado final nem áudio. A mudança
introduz cinco HP por padrão, dano unitário no contato, 30 frames de
invulnerabilidade, tela `GAME OVER`, retorno ao título com START e um impacto
curto no APU.

Os valores ajustáveis ficam em `tuning.h`:

```c
#ifndef PLAYER_INITIAL_HP
#define PLAYER_INITIAL_HP        5U
#endif
#define PLAYER_HIT_COOLDOWN_FRAMES 30U
```

O `#ifndef` permite que apenas a ROM de instrumentação do benchmark use 255 HP.
A ROM normal e os testes funcionais sempre compilam com o padrão cinco.

## Estado do jogador e fluxo do dano

`PlayerState` ganhou dois bytes: `hp` e `hit_cooldown`. `player_init()` restaura
os dois no começo de toda run. `player_update()` reduz o contador uma vez por
update de gameplay e devolve se o jogador já pode sofrer novo dano. O acerto é
saturado e não aceita repetição durante o cooldown:

```c
if (player.hp == 0U || player.hit_cooldown != 0U) {
    return 0U;
}

--player.hp;
player.hit_cooldown = PLAYER_HIT_COOLDOWN_FRAMES;
```

Como o contador começa em 30 e é reduzido no início dos updates seguintes, uma
nova colisão pode produzir dano exatamente 30 updates depois. HP zero não sofre
underflow.

## AABB e custo delimitado

As coordenadas do metasprite continuam representando seu canto superior
esquerdo, mas a área visual 24x24 não é usada inteira para dano. A hurtbox mede
16x16, começa oito pixels abaixo do topo e acompanha o corpo na orientação:

```c
return player.x + (player.facing == PLAYER_FACING_LEFT
    ? PLAYER_HITBOX_LEFT_X_OFFSET_PIXELS
    : PLAYER_HITBOX_RIGHT_X_OFFSET_PIXELS);
```

Olhando à direita ela ocupa `x+8..x+23`; olhando à esquerda, `x..x+15`. Isso
remove a área vazia das costas e a largura visual causada pela bainha. A AABB do
Bat continua 16x8. Para não criar uma varredura de 12 entradas junto da colisão
da espada, `enemy_overlaps_player()` testa um slot usado por oportunidade e
avança um cursor rotativo. Com o limite atual, o reconhecimento pode levar até
12 oportunidades.

Uma primeira implementação varria todos os Bats e perdeu 21 updates no stress.
Limitar a um slot reduziu a perda a quatro, ainda concentrados nos frames ativos
da espada. A forma final usa `collision_phase`: quando jogador vulnerável e
espada ativa coincidem, um frame processa contato e o seguinte processa espada.
Fora dessa coincidência cada consulta roda normalmente.

```c
if (player_vulnerable != 0U &&
    (sword_active == 0U || collision_phase == 0U)) {
    /* consulta um slot de contato */
}
if (sword_active != 0U &&
    (player_vulnerable == 0U || collision_phase != 0U)) {
    enemy_apply_sword_hitbox(&sword_hitbox);
}
```

O trade-off é latência delimitada em vez de um pico O(n). Durante a
coincidência, a espada visita cada paridade ao menos a cada quatro frames ativos.

## Estado de game over e START

Ao aceitar o quinto dano, `game.c` toca o efeito, entra em
`GAME_STATE_GAME_OVER`, limpa sprites e escreve `GAME OVER` na nametable. O loop
principal continua sincronizado, mas o dispatcher frio só atualiza a máquina de
estados. Uma nova borda de START chama a mesma entrada de título usada no boot;
outro START inicia uma run nova e `player_init()` restaura cinco HP.

A shadow de OAM é ocultada durante a troca. Como o DMA acontece na NMI seguinte,
a OAM física reflete a ocultação um frame depois; o teste do Mesen considera
explicitamente esse pipeline.

## Correção do texto GAME OVER

A string em C já era `GAME OVER`, mas `src/chr.s` reservava J-M como tiles
vazios. Portanto a nametable continha `$4D` corretamente, enquanto a pattern
table desenhava espaço no lugar de `M`. A reserva agora cobre somente J-L e o
tile `$4D` recebeu o glifo:

```asm
.res 3 * 16, $00
font_tile $44,$6C,$54,$54,$44,$44,$44,$00 ; M ($4D)
```

O validador estrutural e o teste no Mesen verificam que esse tile não está vazio.

## Efeito sonoro no APU

`nes_play_player_hit_sfx` é uma rotina Assembly curta e chamável por C. Ela não
usa RAM nem zero page, não é reentrante e roda somente na thread principal:

```asm
lda #%00001000
sta $4015
lda #%00011100
sta $400C
lda #%00000100
sta $400E
lda #%00010000
sta $400F
```

O canal de ruído produz um impacto seco. O índice de length counter encerra o
som em aproximadamente dez frames NTSC, sem update por frame e sem trabalho na
NMI. Como ainda não existe música, a rotina assume propriedade exclusiva do
canal; um futuro mixer deverá arbitrar `$4015` e os registradores de ruído.

## Custos medidos

Mapa do linker da ROM padrão, cc65 2.19:

- PRG-ROM: 7.994 -> 8.403 bytes (`+409`);
- BSS: 124 -> 128 bytes (`+4`);
- zero page, DATA, OAM shadow e stacks: inalterados;
- CHR significativo: +16 bytes para corrigir o glifo `M` de `GAME OVER`;
- sprites de hardware: inalterados, pior caso 41/64.

Stress no Mesen com ROM de instrumentação de 255 HP: 1.750 frames, 12 Bats,
duas gemas, 1.735 NMIs e 1.735 updates após a compensação das transições, zero
updates perdidos. O override muda somente o imediato do HP inicial; código e
limites de entidades são os mesmos da ROM padrão.

## Validação e observação no Mesen

- `sim65`: HP inicial, cinco danos, saturação em zero, bloqueio e liberação após
  30 updates, bordas AABB, slot inativo e transições de estado;
- validação estrutural: NROM, vetores, CHR, OAM e zero page;
- Mesen: cinco writes em `$400F`, valores de APU esperados, `GAME OVER`, glifo
  `M` não vazio, OAM oculta após o DMA e START retornando ao título;
- stress Mesen: pool de 12 Bats sem update perdido.

Para inspeção manual, observar `$400C/$400E/$400F`, `player.hp`, o contador de
cooldown e a transição da nametable. A validação headless executou a ROM e
confirmou memória/callbacks; não foi feita inspeção auditiva humana do timbre.

## Limitações e evoluções

- ainda não existe HUD de HP nem feedback visual de invulnerabilidade;
- contato pode levar até 12 oportunidades para ser reconhecido com o pool cheio;
- o SFX não convive ainda com música ou mixer;
- não há knockback nem dano diferenciado por tipo de inimigo;
- o timing continua baseado em NTSC.
