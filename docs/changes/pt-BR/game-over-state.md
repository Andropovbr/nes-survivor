# Diário da branch: game-over-state

## 2026-08-23 — Dano por contato, áudio de acerto e game over

### O que mudou

Foram adicionados cinco HP ajustáveis, cooldown de 30 frames após dano, hurtbox
16x16 inferior e ajustada à orientação, contato delimitado com Bats,
estado/tela `GAME_OVER`, glifo `M` visível, retorno ao título com START e um
impacto curto no canal de ruído. Testes e documentação também foram atualizados.

### Por que foi necessário

Inimigos atravessavam o jogador sem consequência; a run não tinha condição de
falha nem feedback de impacto. Esta mudança fecha o menor ciclo completo de dano
sem incluir HUD, knockback, música ou progressão fora do escopo.

### Trecho relevante

```c
if (player.hp == 0U || player.hit_cooldown != 0U) {
    return 0U;
}
--player.hp;
player.hit_cooldown = PLAYER_HIT_COOLDOWN_FRAMES;
```

A guarda impede underflow e repetição durante a invulnerabilidade. O contador é
atualizado uma vez por update; novo dano fica elegível após 30 updates completos.

```c
return (uint8_t)(player.x +
    (player.facing == PLAYER_FACING_LEFT ? 0U : 8U));
```

A hurtbox 16x16 acompanha a parte inferior do corpo: `y + 8`, com a origem
horizontal deslocada para a frente. Pixels vazios/bainha nas costas são somente
visuais. A fonte CHR também define o tile `$4D` em vez de deixá-lo vazio, então
a string já correta `GAME OVER` passa a desenhar o `M`.

```c
if (player_vulnerable != 0U &&
    (sword_active == 0U || collision_phase == 0U)) {
    /* verifica um slot de contato */
}
```

O cursor rotativo limita a AABB a um slot. Quando contato e espada coincidem,
uma fase determinística alterna os trabalhos em vez de somar os picos.

### Considerações sobre o NES

A política permanece em C. O novo Assembly é somente a ABI pequena do APU: sem
parâmetros/retorno, destrói A/flags, não usa RAM/ZP e roda na thread principal.
O length counter encerra o ruído sem trabalho na NMI. OAM e CHR não mudam. A
tela de game over usa o caminho seguro com rendering desligado; a OAM física é
ocultada no DMA seguinte.

### Desempenho

Medido no Mesen com a build de instrumentação de 255 HP:

- varredura completa de contato: 21 updates perdidos;
- um slot sem escalonamento: 4 updates perdidos;
- versão final escalonada: 0 updates perdidos;
- cenário final: 1.750 frames, 12 Bats, 2 gemas e 1.735 NMIs/updates.

O override altera somente o imediato do HP inicial para a run de carga não
terminar; a ROM entregue usa cinco. Não houve medição isolada de ciclos.

### Impacto em recursos

- PRG-ROM: 7.994 -> 8.403 bytes (`+409`);
- BSS: 124 -> 128 bytes (`+4`: HP, cooldown, cursor e fase);
- RAM geral livre: 351 -> 347 bytes;
- zero page, DATA, shadow de OAM e stacks: inalterados;
- CHR significativo: +16 bytes para o glifo `M` ausente (18 glifos no total);
- sprites: pior caso inalterado em 41/64.

### Arquivos principais afetados

`include/tuning.h`, interfaces e fontes de player/inimigo/fluxo/tela/NES,
`src/chr.s`, testes e scripts de build, READMEs, arquitetura/mapas e a nota.

### Validação

- comandos explícitos de descoberta do Make: indisponível neste Windows;
- build limpo PowerShell: PASS;
- testes lógicos no sim65: PASS;
- validação estrutural de ROM/CHR/mapa: PASS;
- telas e gameplay no Mesen: PASS;
- cenário de cinco danos/APU/game over/START: PASS (game over no frame 549);
- stress de 12 Bats no Mesen: PASS, zero updates perdidos;
- warnings como erros em compilador/assembler/linker: PASS.

### Limitações / próximos passos

Ainda não há HUD, piscar de invulnerabilidade, knockback, mixer de música ou
dano por tipo. A hurtbox 16x16 exclui intencionalmente os oito pixels superiores
e os oito pixels das costas. Com pool cheio, o contato pode levar até 12 oportunidades. O
teste headless verificou registradores do APU, mas não avaliou subjetivamente o
timbre. PAL/Dendy continua sem adaptação.
