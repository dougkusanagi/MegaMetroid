# Pipeline de sprites

As pranchas originais ficam em `res/images/original-references/` (só na máquina local, ignoradas pelo Git). Os scripts em `scripts/` extraem delas PNGs prontos para o `rescomp` do SGDK. O resultado é versionado em `res/images/*-original-sgdk/`.

Requisitos: Python 3 e Pillow (`python -m pip install "Pillow>=9.1"`). Execute os scripts na raiz do projeto.

## Princípios

- **Cores originais.** Os PNGs guardam as cores do SNES como estão na prancha. O `rescomp` converte para o formato do Mega Drive na hora do build, então nunca é preciso editar cor no PNG nem extrair de novo para mudar uma cor.
- **Paleta indexada fixa.** O índice 0 é transparente. Todos os quadros de um mesmo personagem usam a mesma ordem de cores, então um traje ou um ciclo de cor é só uma troca de paleta (`PAL_setPalette`), sem tiles novos.
- **Pivô comum.** Todos os quadros de um personagem têm o mesmo tamanho, e o ponto de referência fica sempre no mesmo pixel. `SPR_setPosition` não precisa de correção por quadro.
- **Esquerda e direita separadas.** A Samus tem arte própria para cada lado (o canhão fica no braço direito). Não use `SPR_setHFlip` nela.
- **Nada entra na ROM sozinho.** O `makefile.gen` do SGDK só compila os `.res` da raiz de `res/`. Os `.res` gerados nas subpastas são catálogos: copie para o `.res` do jogo só o que for usado.

## Samus

```powershell
python scripts/extract_original_samus_sprites.py
```

Lê `res/images/original-references/SNES - Super Metroid - Playable Characters - Samus Aran.png` e regrava `res/images/samus-original-sgdk/`:

| Saída | Conteúdo |
|-------|----------|
| `poses/samus_<pose>_<nome>.png` | Uma tira por pose do jogo. Quadros de 64×64 (8×8 tiles), na ordem da prancha |
| `samus_poses.res`, `manifest_poses.csv` | Um `SPRITE` por pose; o manifesto tem o número da pose no jogo, o nome, as poses que usam os mesmos quadros (`aliases`) e a posição de cada quadro na prancha |
| `palettes/pal_samus_<nome>[_<n>].png` | Uma paleta de 16 cores por linha de amostra da prancha |
| `samus_palettes.res`, `manifest_palettes.csv` | Um `PALETTE` por paleta; o manifesto lista as 16 cores em hexadecimal |
| `effects/samus_fx_<nome>_<nn>.png` | Morte, Crystal Flash e *missile fins* |
| `samus_effects.res`, `manifest_effects.csv` | `SPRITE` dos efeitos de quadro único; o manifesto indica a paleta de cada um e se está pronto para o SGDK |

### Poses

O número da pose (`09`, `1A`…) é o do jogo original, lido das faixas azuis da prancha. A transcrição está em `scripts/data/samus_pose_labels.csv`, com a posição de cada faixa. Faixas empilhadas sem quadros entre elas são poses que compartilham os mesmos quadros: a tira recebe o nome da primeira, e as demais aparecem em `aliases`. Poses sem número na prancha usam `xx`. A coluna `diamond` registra a marca ♦ da prancha.

O quadro de 64×64 é a janela `(8, 8)–(72, 72)` da caixa de 80×80 da prancha, que contém todas as poses. Com a Samus em pé, os pés ficam na linha 59 do quadro e o corpo fica centrado por volta da coluna 32.

Os quadros de demonstração de paleta (o traje de frente em cada cor) não viram tiles, já que essas cores estão nas paletas.

### Paletas

Cada linha de amostra da prancha tem 15 cores (índices 1 a 15), na mesma ordem dos índices dos quadros. O índice 0 recebe a cor de fundo da caixa, só para marcar a transparência.

| Grupo | Linhas |
|-------|--------|
| `power_suit`, `varia_suit`, `gravity_suit` | 1 |
| `<traje>_yellow_cycle`, `_green_cycle`, `_blue_cycle`, `_white_cycle` | 3 |
| `<traje>_long_white_cycle` | 7 |
| `hyper_beam` | 10 |
| `flashback` | 1 |
| `death_tiles_suit`, `death_tiles_suitless`, `death_intro_*`, `death_unused_original` | 1 |
| `death_fading` | 9 |
| `crystal_flash_parts`, `crystal_flash_palette_animation` | 6 |
| `crystal_flash_reference_gray`, `crystal_flash_reference_suit`, `missile_fins` | 1 |

Exemplo de troca de traje sem trocar tiles:

```c
PAL_setPalette(PAL2, pal_samus_varia_suit.data, DMA);
```

### Efeitos

Cada figura avulsa vira um PNG numerado na ordem de leitura. O script escolhe a primeira paleta extraída que contém todas as cores da figura, testando antes as paletas da própria região (`palette` no manifesto). Quando nenhuma serve, a figura usa uma paleta própria (`own`).

Algumas regiões saem inteiras, como referência e sem `SPRITE` (`sgdk_ready = no`):

- `death_tiles_tileset` e `death_tiles_loose`: tiles de fundo da morte, com várias paletas.
- `death_sequence`: o corpo sem traje desenhado junto com as partículas, com duas paletas. No jogo são sprites separados: use `death_suitless_*` com as partículas de `death_shattering_suit`.
- `death_shattering_suit`: as partículas se sobrepõem entre quadros, então os quadros são recortados quando a morte for implementada.


### Folha de animação do jogo

O jogo não carrega as poses avulsas. `scripts/build_player_sheet.py` monta `res/images/player/samus_sheet.png` a partir delas, e gera `inc/player_anim.h` com o enum `PLAYER_ANIM_*`:

- Cada linha é uma animação e cada célula é um quadro de 64×64, lido da esquerda para a direita.
- A lista `ANIMATIONS` do script diz quais poses entram, em ordem: ação, pose para a direita, pose para a esquerda e, opcionalmente, a mira (`U`, `DU`, `D`, `DD`; frente se omitida). Para incluir uma pose nova, acrescente uma entrada e rode o script de novo.
- Uma pose pode ser `'NN'` (a tira inteira) ou `'NN:i'` / `'NN:i-j'` (só esses quadros). As tiras de mira começam com um quadro de transição; a folha usa só o quadro mantido.
- O script também gera `player_muzzle[]`: a boca do canhão no primeiro quadro de cada linha, em pixels a partir da posição do corpo. É o pixel das cores do canhão (índices 5, 7, 8 e 13 da paleta Power Suit) mais distante na direção da mira. O tiro sai desse ponto.
- A esquerda é sempre uma linha própria (`PLAYER_ANIM_<ação>_L` vem logo depois de `_R`). O código soma 1 à linha da direita; não use `SPR_setHFlip`.
- Linhas podem ter quantidades diferentes de quadros. O `rescomp` descarta as células vazias no fim de cada linha.
- Os pés das poses de chão (`GROUNDED`: parado, andando, pouso, agachar, abaixar e levantar) ficam todos na linha 58 da célula, a última do retângulo de colisão (`y + 47`). Cada pose anda inteira para isso, sem mudar a posição relativa entre os quadros da mesma pose. Sem isso a agachada da prancha original ficava 11 px acima do chão. As poses do ar não são alinhadas.
- O `.res` usa tempo 5 por quadro, como a folha antiga. Tempo 0 deixa a animação parada.

```powershell
python scripts/build_player_sheet.py
```

A paleta é `pal_samus_power_suit` (um `PALETTE` no `res/resources.res`). A folha é versionada, porque o build do SGDK não roda Python.

## Outras pranchas

### Tempos de agachar, bola e pousos (2.8.3)

As tiras existentes foram mantidas. Os tempos agora seguem a medição do Mesen:
abaixar/levantar duram 3 ticks, morfar/desmorfar 6 (3 por quadro), pouso normal
7 (5+2 por quadro) e pouso do giro 1. Para as transições de dois quadros e os
pousos, o render seleciona o quadro pelo contador da simulação, sem depender
dos 5 ticks padrão do SGDK. Desmorfar no chão termina na pose agachada.

Sem tiles, sprites ou paletas adicionais. O contador antigo de duração de ↓
foi removido: menos 4 bytes de RAM no núcleo do 68000. A hitbox original por
estado ainda será tratada na 2.8.4. Medidas e testes em `docs/FISICA.md`.

### Virada (3.2.7)

A folha inclui `TURN` (`25/26`, pés alinhados) e `TURN_AIR` (`2F/30`).
Os nomes das poses indicam o lado **inicial** da Samus. O núcleo mantém esse
lado e um contador visual de seis ticks; o render mostra os quadros 0, 1 e 2
por dois ticks cada, sem loop do SGDK. A frenagem continua usando `turn` e
`turned`, com as velocidades já medidas. Mira/tiro têm prioridade visual,
preservando a origem do canhão; bola, giro e chute não exibem a virada normal.

Medida reproduzível: cenários `turn`, `turn_moving` e `air_turn_rest` de
`scripts/sm_reference.ps1`, colunas `pose`, `anim_frame` e `anim_timer`.
No chão o original usa `25`, no ar `2F`, ambos por 2+2+2 ticks.

Custo: quatro linhas/12 quadros na folha, dois inteiros adicionais no núcleo
(8 bytes de RAM no 68000). Os novos quadros têm até 18 tiles, abaixo do máximo
já reservado pela Samus; sem sprite ou paleta adicional em VRAM. Ciclos de CPU
ainda não medidos em hardware. `build_player_sheet.py` valida os pixels gerados;
o teste da ROM verifica os três quadros nas quatro tiras pelo BlastEm via API.

### Pulo normal: sequência de quadros (correção de 18/10/2026)

O pulo normal usa a tira `4D/4E` na subida e na descida. `29/2A` é a queda
ao sair de uma borda. No Mesen, `jump_stand` mantém a pose `4D` por 91 ticks:
quadro 0 por 3 ticks, quadro 1 por 43, quadros 2, 3 e 4 por 4 ticks cada,
quadro 5 até pousar (33 ticks nessa medição). Esses tempos dependem da altura
do salto: o quadro 1 aguarda a descida, e o 5 aguarda o chão.

O núcleo controla `jump_frame` e `jump_frame_ticks`; o render escolhe esse
quadro, sem o loop automático do SGDK. A saída `4B/4C` continua com o tempo
de transição já existente no porte. Nenhum tile novo: os seis quadros estavam
na folha. Custo adicional: dois inteiros (8 bytes de RAM no 68000), um contador
atualizado por tick; ciclos de CPU ainda não medidos em hardware.

Para reproduzir a medida: `./scripts/sm_reference.ps1 -Scenario jump_stand
-Out tests/out/sm_jump_animation.csv`. O CSV inclui `anim_frame` e `anim_timer`,
lidos de `$0A96` e `$0A94`, conforme o
[mapa de RAM do disassembly](https://github.com/InsaneFirebat/sm_disassembly/blob/master/src/memory.asm).
`normal_jump_animation()` verifica os tempos na subida/descida, em ambos os
lados e com pulo curto; a ROM confere tira e quadros renderizados nos dois lados
pelo teste da API GDB do BlastEm.

### Power Beam em jogo (3.2.6)

`python scripts/build_beam_sheet.py` gera `res/images/player/beam.png` (8 direções,
na ordem do manifesto) e `beam_impact.png` (os cinco primeiros quadros laranja de
`weapons_impact_r00`; os dois últimos são azuis). A paleta compartilhada ocupa PAL3:
as 15 cores da tira do tiro, incluindo transparência, mais branco no índice 15.
Nenhuma cor é quantizada pelo script. Os PNGs gerados são versionados.

O impacto fica no primeiro pixel bloqueado pela varredura, por 10 ticks (2 por
quadro), inclusive quando há parede entre o corpo e a boca do canhão. Sair da tela
ou esgotar a vida do tiro não gera impacto. Cada uma das três entradas do pool
fica ocupada até seu impacto terminar. O render reserva 4 tiles por entrada
(12 tiles / 384 bytes de VRAM, contra 3 tiles antes), sem realocar durante a troca
de definição; usa até 3 sprites de hardware. O núcleo acrescenta 36 bytes de RAM
no 68000 (três inteiros de 32 bits por entrada); a cópia da definição do tiro
reserva os mesmos 4 tiles do impacto. A varredura existente mantém seu limite
de 12 passos por tick por tiro; não houve medição de ciclos no hardware.

```powershell
python scripts/extract_sheet_sprites.py            # todas
python scripts/extract_sheet_sprites.py weapons    # só uma
```

Cada prancha vira `res/images/<prancha>-original-sgdk/`, com um `.res` (catálogo) e um `manifest.csv`. As regiões de cada prancha ficam em `SHEETS`, no início do script. Os nomes seguem a posição na prancha (`_rNN` é a linha, `_N` é o pedaço de uma linha). Os nomes definitivos ficam para a etapa que usar cada recurso.

| Prancha | Pasta | Conteúdo |
|---------|-------|----------|
| Weapons | `weapons-original-sgdk` | Míssil e super míssil em 8 direções; tiro laranja em 8 direções (provavelmente o Power Beam sem carga, conferir no jogo); bomba, segunda bomba e fumaça; feixes fino e grosso (3 quadros por direção); impactos e explosões; grapple beam |
| Objects, Items & Doors | `objects-original-sgdk` | Tampas de porta por cor (vertical e horizontal, como tiles); itens e upgrades; estátuas Chozo, estações, elevador e portas grandes |
| Doors | `doors-original-sgdk` | Portas com batente e abertura da tampa (azul, rosa, verde, laranja, cinza) e portas de Ceres |
| Samus's Starship | `starship-original-sgdk` | Nave como `IMAGE` (192×80): luzes como uma imagem e 8 paletas (`starship_lights_pal_NN`), escotilha abrindo em 10 imagens |
| Status Bar | `status-bar-original-sgdk` | Tiles do HUD: texto ENERGY, ícones de item (apagado/aceso), AUTO, dígitos e tiles do minimapa em 3 cores. O preto vira o índice 0 |
| Enemies | `enemies-original-sgdk` | Uma tira por linha de inimigo nas três colunas da prancha. A linha de cada inimigo de Crateria (Zoomer, Skree, Ripper, Geemer) é identificada na etapa 5.2 |

Como na Samus, as cores são as originais e o índice 0 é transparente. Cada tira usa uma paleta só. Quando uma linha precisa de mais de 15 cores (quadros de dano piscando, por exemplo), ela é dividida em tiras consecutivas.

Ficam como referência em RGB, sem entrada no `.res` (`sgdk_ready = no` no manifesto):

- Recortes com mais de 15 cores, como as portas de Ceres em `doors` e alguns itens. No SNES esses gráficos são tiles de fundo com uma paleta por tile; no Mega Drive, as cores precisam ser separadas por tile entre as paletas disponíveis quando o recurso for usado.
- Quadros maiores que 128×128, que o SGDK não monta como sprite (limite de 16 sprites de hardware por quadro).
- `starship_tiles_and_palettes`: amostras de paleta e tiles da escotilha da prancha da nave.

Ainda não extraídos de forma geral: tilesets e mapas das demais salas (fase 6), chefes, cenas, título e fontes. Cada uma entra em `SHEETS` quando a etapa correspondente começar.


## Landing Site: integração visual 6.0

`python scripts/build_landing_site.py` regenera os recursos versionados em
`res/images/maps/landing_site/`, usando Pillow e numpy. A origem é o recorte
sem perdas da prancha nova de mapas de Crateria arquivado em
`tests/reference/super_metroid/landing_site/foreground.png`; não usa a imagem
editada anterior. O manifesto registra a prancha, checksum e recorte
(4615,1,2304,1248). O azul da prancha é transparência, não a cor do céu.

O fundo de 256×512 foi decodificado do BG2/VRAM e CGRAM originais arquivados,
com CHR 4bpp, atributos, espelhamentos e tilemap vertical duplo. É uma amostra
do save disponível, não uma referência de todas as condições meteorológicas.
O scroll vertical usa o período de 512 px; outros níveis podem conservar fundo
fixo (`background_scroll_height = 0`). Não há chuva nesta entrega.

As portas usam o primeiro quadro fechado de `doors_door_r00`, espelhado na
parede direita: (0,608), (0,1120), (2272,352), (2272,1120). A nave usa
`starship_hatch_00` em (1057,1101), com precisão visual de um pixel contra a
captura do save. Toda a arte é composta no MAP de BG_A; o scroll do SGDK mantém
os objetos visíveis ao entrar e sair da tela. Nave/portas são estáticas e não
mudam colisão, spawn ou progressão. A base de blocos vem do próprio mapa novo;
blocos ainda não podem ser destruídos.

**Paletas.** PAL0: terreno; PAL1: céu e tons de porta; PAL2: Power Suit sem
alteração; PAL3: nove cores distintas do feixe no VDP, mais seis tons de nave.
Cada tile escolhe uma única paleta por erro de cor ponderado, sem dithering ou
redimensionamento. Assim, a forma dos pixels é preservada, mas há aproximações
nas cores de nave/portas. As nuvens mantêm tons neutros em vez de ficarem
avermelhadas pelo truncamento isolado de canais. As 15 cores SNES da prancha do
feixe já se reduziam a nove no VDP; a nova indexação mantém exatamente essas
cores e a transparência. `build_beam_sheet.py` reutiliza essa PAL3 e não repõe
a paleta antiga ao regenerar os tiros.

**Custo.** Tiles de cenário: 311 de foreground e 126 de background na build
validada, 437 tiles / 13.984 bytes de VRAM. Quatro bancos de 16 cores no total,
nenhum sprite adicional de hardware ou entidade alocada. A composição e escolha
de paletas são feitas no host; por frame permanece o scroll de dois MAPs. A ROM
continua alinhada em 1.441.792 bytes. A colisão conserva sua grade de 44.928 bytes
em ROM. O novo campo do nível usa dois bytes por definição. O readback de vídeo
reserva cerca de 8,4 KB apenas na ROM de smoke; não entra na ROM normal.

**Validação.** `scripts/test.ps1` confere dimensões, uma paleta por tile e as
cores efetivas do feixe/impacto, além das regressões do núcleo.
`python scripts/smoke_test.py --capture-scenes` reconstrói seis vistas a partir
da VRAM, CRAM, scroll e SAT lidos pela própria ROM no BlastEm, via GDB. Confere
os pixels dos planos, bancos de paleta e scroll contra os recursos compilados;
não é uma comparação automática com todas as cenas possíveis do SNES.


### 6.0.1 — Continuidade do scroll e medição dos saltos

O módulo `% 512` aplicado antes de `MAP_scrollTo` era incorreto: o próprio MAP
já repete os blocos do recurso. Cruzar 1024→1023 fazia o parâmetro do fundo saltar
0→511, solicitando um redraw completo. Agora o fundo recebe Y contínuo da câmera;
o hardware e o MAP cuidam da repetição. A arte não foi retocada por causa dessa
mudança.

No BlastEm NTSC, em 110 quadros medidos por salto, após aquecimento e com pelo
menos 24 px de movimento vertical de câmera (1024→988 na execução validada),
o resultado foi:

| Medida | Normal antes/depois | Giratório antes/depois |
|---|---|---|
| VInts perdidos | 2 → 0 | 2 → 0 |
| Maior fila DMA (bytes) | 4.368 → 912 | 4.872 → 1.032 |
| Maior carga média de 8 quadros (%) | 42 → 28 | 46 → 32 |

O smoke falha se esses trajetos perderem VInts, ultrapassarem 2 KB de DMA ou se
não executarem o salto com o deslocamento de câmera esperado. Os valores são
desse cenário, não um benchmark de todas as salas/inimigos nem garantia para
qualquer emulador ou hardware. O port não recebe instrumentação na ROM padrão.

`tests/reference/super_metroid/landing_sky/` conserva quatro probes adicionais
do original (câmeras Y=976, 1024, 688 e 43), incluindo scanlines, VRAM, CGRAM e
screenshots. Na câmera alta Y=43, a faixa de horizonte e o retorno às nuvens
também aparecem no original; a repetição visual não foi removida. As posições
high/upper foram fixadas artificialmente apenas para estabilizar a câmera, sem
alterar os registradores de render. O original aplica scroll horizontal por
faixas/HDMA e chuva que ainda não existem no port; o céu não é visualmente idêntico.

### Mira diagonal parada e agachada (3.3.1)

Y/Z selecionam diagonal acima/abaixo sem movimento. As poses de quadro único
05–08 (em pé) e 71–74 (agachada) usam o quadro 0; as oito linhas são alinhadas ao
chão pelo gerador, que calcula a boca do canhão para os tiros diagonais.
A folha passa de 64 para 72 linhas, mantendo streaming de um quadro por vez,
sem novos sprites simultâneos. O input acrescenta um inteiro a `MmInput`
(4 bytes no alvo), sem alocação dinâmica. Y+Z cancelam o modificador.

### Tampa de porta animada (6.6.3)

`python scripts/build_door_cap.py` gera `res/images/doors/door_cap.png` a partir dos quadros
0–4 de `res/images/doors-original-sgdk/doors_door_r00.png`. A folha tem 5 quadros de 16×64 px
(80×64), recortados da faixa x=16..31 de cada quadro: é a única parte que muda entre a porta
fechada e o batente aberto. O quadro 0 é a porta fechada; 1–3 retraem a tampa em três formas
de 6 ticks; 4 mostra o batente aberto, com os 4 px do lado da sala em preto.

O cenário mantém a porta fechada composta em BG_A; o sprite é sempre opaco e cobre a faixa.
Como o índice 0 é transparente no Mega Drive, os pixels vazios usam o índice 1 (preto), e não
0. A paleta é a PAL1 do Landing Site, porque é o único banco com os três azuis da porta:
índices 9/10/11 (azuis), 3/6/8/12/13 (cinzas) e 14 (rosa/realce). O branco da folha original
(índice 12) cai no 14, o slot mais próximo da PAL1. O gerador recusa qualquer índice fora
desse mapa, para não converter por engano as portas vermelhas/rosas da mesma folha.

Custo: cada porta usa 2 sprites de hardware (16×64 = 2×16×32) e 16 tiles; oito portas reservam
16 sprites e 128 tiles de VRAM. As tampas entram na lista antes de Samus e dos feixes, ficam
com `SPR_setAlwaysAtBottom` e profundidades explícitas: portas em `SPR_MAX_DEPTH`, Samus em
`SPR_MAX_DEPTH - 1`, feixes em `SPR_MAX_DEPTH - 2`. Portas verticais são ocultadas até existir
sala construída com uma. A PAL1 da Crateria 2 não tem as cores de porta e sua área de porta é
preta; a tampa reaproveita a paleta do Landing Site e não causa regressão visível ali.

Validação: o smoke no BlastEm confere visibilidade, quadro (`mm_lock_frame`), espelhamento,
posição com a câmera e ordem de profundidade nos quadros 1–4, e que sprites de portas
inexistentes permanecem ocultos.
