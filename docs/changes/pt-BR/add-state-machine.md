# Diário de desenvolvimento da branch: add-state-machine

## 2026-08-23 — Blink da title screen movido para a NMI

### O que mudou

A atualização periódica de `Press Start` não desliga nem religa mais a
renderização pelo C. `screen.c` publica um byte de modo mostrar/ocultar, e a NMI
seguinte escreve exatamente 11 tiles em `$220A`, limpa o pedido e restaura scroll
zero. Máscaras de PPUCTRL, asserts do layout CHR e verificações no Mesen agora
garantem explicitamente sprites na Pattern Table 0 e background na Table 1.

### Por que foi necessário

O caminho antigo assumia que o C acordado depois da NMI terminaria antes do fim
do VBlank. Ao religar a renderização depois de alterar `$2006/$2007`, a PPU podia
retomar em um scanline visível com endereço interno transitório. Isso produzia
um fragmento breve do prompt em posição errada. A transferência fixa no VBlank
corrige a causa de timing e latch, em vez de esconder o artefato.

### Trecho relevante

```c
if (visible != 0U) {
    screen_title_prompt_update = TITLE_PROMPT_UPDATE_SHOW;
} else {
    screen_title_prompt_update = TITLE_PROMPT_UPDATE_HIDE;
}
```

```asm
lda PPUSTATUS
lda #>TITLE_PROMPT_ADDRESS
sta PPUADDR
lda #<TITLE_PROMPT_ADDRESS
sta PPUADDR
```

A atribuição do C é atômica no 6502. A NMI controla toda a transferência, lê
status para reiniciar o latch compartilhado de `$2005/$2006` e só limpa o pedido
depois dos 11 bytes.

### Considerações sobre o NES

`PPUCTRL=$90`: bit 7 habilita NMI, bit 4 seleciona a Pattern Table 1 de background
em `$1000` e o bit 3 limpo seleciona a Pattern Table 0 de sprites em `$0000`.
Entradas da nametable continuam sendo índices de 8 bits. `chr.s` assegura por
assert que cada metade física de CHR possui exatamente 4 KiB. A transferência é
fixa; nenhum command buffer genérico de VRAM foi criado.

### Desempenho

Estimativa pelo fluxo de instruções:

- NMI normal: aproximadamente 590 ciclos, antes aproximadamente 583;
- NMI que mostra o prompt: aproximadamente 788 ciclos totais;
- orçamento de VBlank NTSC: aproximadamente 2.273 ciclos.

O stress de 1.750 frames no Mesen atingiu 12 Bats com 1.735 NMIs e 1.735 updates
de gameplay após a baseline de transição: zero perdas.

### Impacto em recursos

- PRG-ROM: 7.099 -> 7.043 bytes (`-56`).
- BSS: 80 -> 81 bytes (`+1`) para o modo pendente.
- Zero page, DATA, OAM, sprites de hardware e conteúdo/tamanho de CHR: inalterados.
- A fonte permanece fisicamente em `CHR $1000-$1FFF`; os asserts não mudam bytes.

### Arquivos principais afetados

`src/screen.c`, `src/nmi.s`, `src/crt0.s`, `src/chr.s`, `include/nes.h`,
`tests/test_logic.c`, `tests/mesen_game_states.lua`, documentos de arquitetura e
memória e `docs/implementation-notes/game-states.md`.

### Validação

- build limpo da ROM: PASS
- testes sim65 e asserts de compilação de PPUCTRL: PASS
- validação estrutural de ROM/CHR/map: PASS
- estados no Mesen, 175 frames e vários ciclos de blink: PASS
- nenhum estado parcial do prompt na nametable amostrada: PASS
- nenhum write em `$2000/$2001` durante blink: PASS
- `PPUCTRL=$90` e glifo `P` na Pattern Table 1 observados: PASS
- runtime player/espada/Bat no Mesen, 450 frames: PASS
- stress de 12 Bats no Mesen, 1.750 frames: PASS, zero perdas

### Limitações / próximos passos

O Mesen headless valida memória da PPU e writes de registradores, mas não substitui
observação humana do quadro. Antes do merge, observar manualmente vários ciclos
e confirmar as duas metades no PPU Viewer. Transições completas continuam
desligando rendering intencionalmente; somente o update periódico pequeno foi
movido para a NMI.
