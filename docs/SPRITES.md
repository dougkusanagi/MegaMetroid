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


## Outras pranchas

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

Ainda não extraídas: tilesets e mapas das salas (fase 6), chefes, cenas, título e fontes. Cada uma entra em `SHEETS` quando a etapa correspondente começar.
