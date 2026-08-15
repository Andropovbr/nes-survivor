# Mapa de memória e orçamentos

As medições abaixo são provenientes de `build/nes-survivor.map`, gerado pelo cc65 2.19 após o marco da espada automática. Regenere o mapa após a adição de cada sistema significativo.

## Espaço de endereçamento da CPU e RAM interna

| Intervalo | Bytes | Finalidade atual |
| --- | ---: | --- |
| `$0000-$0001` | 2 | zero page deliberadamente não alocada |
| `$0002-$001D` | 28 | símbolos do projeto e da zero page do cc65 |
| `$001E-$00FF` | 226 | zero page atualmente não alocada |
| `$0100-$01FF` | 256 | stack de hardware do 6502 (reserva para o pior caso) |
| `$0200-$02FF` | 256 | shadow de OAM alinhada à página, 64 sprites x 4 bytes |
| `$0300-$0324` | 37 | dados de runtime inicializados do cc65 |
| `$0325-$0334` | 16 | variáveis globais BSS de C |
| `$0335-$04FF` | 459 | RAM geral livre na região atual do linker |
| `$0500-$07FF` | 768 | stack de parâmetros de software do cc65 |

A RAM interna estática/reservada é de 1.361 de 2.048 bytes, deixando 687 bytes não atribuídos (228 bytes na zero page e 459 bytes gerais). Os intervalos de stack são orçamentos, não o pico de uso medido; marcos futuros devem medir a profundidade real da stack.

O pool de OAM possui exatamente 64 entradas, de quatro bytes cada, e é sempre enviado por DMA como uma página inteira. Cada byte inicia como `$FF`, portanto sprites não utilizados possuem Y fora da tela e não aparecem. A construção futura de OAM deve preservar a prioridade determinística e ocultar todas as entradas não utilizadas.

## Estado mutável atual

| Estado | Bytes | Localização/segmento |
| --- | ---: | --- |
| Contador de frames da NMI | 1 | zero page `$0002` |
| Variável temporária de leitura do controle | 1 | zero page `$0003` |
| Runtime de zero page do cc65 | 26 | zero page `$0004-$001D` |
| Estado do jogo e cursor do renderizador de OAM | 2 | BSS |
| Entrada atual/pressionada/solta | 3 | BSS |
| Estado do RNG | 2 | BSS |
| Posição/orientação/movimento/reprodução do jogador | 7 | BSS |
| Temporizadores de atividade/cooldown da espada | 2 | BSS |

O jogador e o runtime de uma espada não possuem pools. Não há buffers de atualização de nametable, alocações de áudio ou heap. A alocação dinâmica na heap é proibida. A stack nativa do 6502 permanece isolada da OAM, e a stack de software do cc65 cresce dentro de sua região dedicada de três páginas.

O renderizador genérico de OAM não possui nenhum buffer de sprites além da shadow de 256 bytes existente. Ele utiliza um byte em BSS para a próxima entrada, oculta todas as 64 coordenadas Y a cada frame e emite o jogador nas entradas 0-6. Durante os 12 frames ativos do ataque, a espada utiliza as entradas 7-8; nos demais frames, essas entradas e as outras 55 permanecem ocultas. O jogador usa no máximo três sprites em uma scanline, e a espada com um tile de largura adiciona no máximo um na mesma scanline.

## Uso do cartucho

| Região | Conteúdo utilizado | Capacidade | Notas |
| --- | ---: | ---: | --- |
| Cabeçalho iNES | 16 bytes | 16 bytes | mapper 0, NROM-256, espelhamento horizontal |
| PRG-ROM | 3.252 bytes | 32.768 bytes | 9,92% utilizado; 29.516 bytes livres |
| CHR-ROM | 208 bytes atribuídos | 8.192 bytes | índices de tiles `$00-$0C`; 7.984 bytes permanecem em branco |
| Total do arquivo `.nes` | 40.976 bytes | 40.976 bytes | cabeçalho + PRG + CHR |

O uso de PRG inclui 220 bytes de inicialização/paleta, 12 bytes de inicialização de construtores, 2.852 bytes de código/rotinas auxiliares, 125 bytes de RODATA do Soldier e da espada, 37 bytes da imagem de carga de DATA e seis bytes de vetores de interrupção. A RODATA contém 21 registros de metasprite, 3 registros de frame, duas definições e um descritor agregado do Soldier, além dos dois registros de metasprite da espada. Bytes de preenchimento ocupam o restante da imagem NROM fixa, mas não são contabilizados como conteúdo utilizado.

`assets/game.chr` ocupa todo o banco obrigatório de 8 KiB. O Soldier utiliza os índices `$00-$07`, e a espada utiliza dois tiles de 16 bytes em `$08-$09`. O tile importado atribuído mais alto permanece em `$0C`, totalizando 208 bytes atribuídos.

## Orçamento de VBlank

A NMI sempre executa um DMA de OAM e rotinas constantes de controle interno. Sua estimativa no pior caso é de aproximadamente 583 ciclos de CPU, incluindo a entrada da interrupção, ou cerca de 26% do VBlank em NTSC. A paleta do Soldier é carregada apenas durante o reset com a renderização desabilitada; não há transferências de paleta, nametable ou áudio durante a NMI. Qualquer fila de transferência futura deve ser delimitada e adicionada a este orçamento.
