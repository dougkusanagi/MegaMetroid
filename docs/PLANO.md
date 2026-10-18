# Plano de portabilidade: Super Metroid → Mega Drive (MegaMetroid)

Plano para continuar o port de Super Metroid para Mega Drive / Genesis usando o [SGDK](https://github.com/Stephane-D/SGDK).
Cada etapa é pequena, testável no emulador e termina em **commit + push** (ver `AGENTS.md`).

Última revisão: 2026-10-12.

> **Nota (revisão da refatoração `codex/refactor-game-core`)**: o código dessa branch foi incorporado a esta. Avaliação: o núcleo (`src/core/`) é C portátil sem SGDK, em ponto fixo Q24.8, com colisão varrida por pixel, pool estático de projéteis, `src/game.c` como única camada SGDK e testes de regressão em `tests/` (`scripts/test.ps1`, exige MSVC/gcc/clang no host). Isso adianta as etapas 0.3, 0.4, 1.1, 1.2, 2.4, 4.1 e 4.2 e a câmera por zonas. As etapas ✅ acima foram validadas por compilação; o teste de comportamento no emulador ainda deve ser repetido a cada etapa. Não vale aproveitar: `dev` (versão anterior da mesma refatoração, com `player.c`/`physics.c` maiores), `0.54` (reescrita abandonada com arquivos vazios) e as cópias `better-org` / `working-v-col`.

## 1. Estado atual

**Já funciona**
- Cenário de Crateria (fg + bg) como `MAP` do SGDK, com scroll de câmera seguindo a Samus (`updateCamera`).
- Sprite da Samus (5x6 tiles) com animações parada / andando / pulando e flip horizontal.
- Colisão com tiles 8x8 (AABB horizontal + vertical) contra um mapa de colisão de 288x156 tiles.
- Rampas (tile 2 = direita, tile 3 = esquerda) em estado inicial — último commit "Improve slope collision handling".
- Pulo (botão C) com gravidade e SFX PCM via XGM.
- Duas salas definidas via `level_def` (`level_crateria_1`, `map_crateria_2`), mas só a primeira é carregada.

**Limitações / dívida técnica identificadas**
- `src/main.c` (~580 linhas) concentra boot, input, física, colisão, câmera e animação.
- `main.c` consulta `map_collision` (definido em `src/map.c`, declarado em `inc/map.h`) em vez do `map_collision_crateria_1` do `level_def`; o campo `level_def.map_collision` é um `u8*` que não é usado.
- `control` é uma variável global definida dentro de `inc/types.h` (risco de definição múltipla ao crescer o número de `.c`).
- Direcional ↑/↓ move a Samus livremente (debug) em `playerUpdate`; o pulo é disparado só no evento de input (`handleInput`): sem pulo variável.
- `stopSoundJump()` chama a si mesma (recursão infinita) — bug a corrigir.
- Sem máquina de estados do player, tiros, inimigos, itens, portas, HUD, save ou música.
- `out/` (artefatos de build) está versionado e o `.gitignore` está vazio.
- `src/better-org` e `src/working-v-col` são cópias antigas de `main.c`, fora do build.
- A variável de ambiente `GDK` precisa apontar para o SGDK para compilar (o shell dos agentes pode não tê-la definida).

## 2. Princípios

1. **Orçamento do hardware**: 64 KB de VRAM, 64 KB de RAM, 80 sprites (20 por scanline), 4 paletas de 16 cores, ROM até 4 MB sem mapper. Toda feature registra seu custo (tiles de VRAM, RAM, ciclos por frame).
2. **Fidelidade pragmática**: física e comportamento inspirados no Super Metroid, adaptados ao Mega Drive (tiles 8x8, 60 fps NTSC).
3. **Dados fora do código**: salas, colisão, inimigos e itens vêm de exportações em `res/` / `tools/`, não de arrays gigantes editados à mão.
4. **Incremental**: cada etapa deixa o ROM rodando.
5. **Commit + push a cada etapa testada e funcionando** (regra do `AGENTS.md`).

## 3. Roteiro por etapas

**Teste** = como validar (emulador, por exemplo BlastEm ou Gens KMod; `KLog` para debug).

### Fase 0 — Base e higiene

| # | Etapa | Entrega | Teste |
|---|-------|---------|-------|
| 0.1 ✅ | Ambiente de build documentado | `docs/BUILD.md`: `GDK`, comando de build (`make -f %GDK%\makefile.gen`), emulador usado | `out/rom.bin` gerado a partir de clone limpo |
| 0.2 ✅ | `.gitignore` e limpeza | Ignorar `out/`, `git rm -r --cached out`; remover `src/better-org` e `src/working-v-col` | Build gera `rom.bin`; `git status` limpo após build |
| 0.3 ✅ | Corrigir bugs conhecidos | `stopSoundJump` (recursão), `control` movido para `.c` + `extern`, warnings | Build sem warnings; som de pulo toca e para |
| 0.4 ✅ | Usar `level_def` de verdade | Colisão lida pelo `level_def` (`game_init` aponta `MmWorld` para `level->map_collision`); `src/map.c` ficou só com `level_get` | Colisão igual à atual em Crateria 1 |

### Fase 1 — Refatoração do núcleo

| # | Etapa | Entrega | Teste |
|---|-------|---------|-------|
| 1.1 ✅ | Dividir `main.c` | `player.c`, `camera.c`, `collision.c`, `input.c`, `level.c` + headers em `inc/` | Comportamento idêntico no trecho inicial |
| 1.2 ✅ | Input por polling | `JOY_readJoypad` por frame (pressionado / segurado / solto), sem estado em `handleInput` | Andar, pular e soltar direção sem "travar" |
| 1.3 ✅ | Loop e tempo fixos | Loop único (`update` → `draw` → `SYS_doVBlankProcess`) e contador de frames | 60 fps estáveis |
| 1.4 ✅ | Debug overlay | Posição, velocidade, `is_on_floor`, tile sob os pés (ligado por `#define`) | Valores coerentes ao andar |

### Fase 2 — Movimento da Samus

| # | Etapa | Entrega | Teste |
|---|-------|---------|-------|
| 2.1 ✅ | Máquina de estados | Parada, andando, pulando, caindo, agachada, bola, spin | Transições corretas em log |
| 2.2 ✅ | Física tabelada | Gravidade/aceleração/velocidades em subpixel (`fix16`); medidas em `docs/FISICA.md` | Altura e alcance do pulo medidos |
| 2.3 ✅ | Pulo variável | Soltar o botão corta o pulo; coyote time curto | Pulo curto vs. longo |
| 2.4 ✅ | Remover voo por d-pad | ↑/↓ passam a mirar / agachar | Samus não "voa" mais |
| 2.5 ✅ | Rampas robustas | Altura por coluna (45°, depois 22,5°); subir/descer sem quicar | Percorrer todas as rampas de Crateria 1 sem travar ou flutuar |
| 2.6 ✅ | Agachar e bola | Hitbox menor, animações, passagens estreitas | Entrar em túnel de 1 tile |
| 2.7 ✅ | Spin jump / wall jump | Pulo giratório; wall jump depois | Subir poço com wall jump |
| 2.8 | Física medida no original | Corrigir as diferenças da tabela de `docs/FISICA.md` (medidas no Mesen 2), em sub-etapas 2.8.1–2.8.5 | Valores do original fixados em `tests/core_test.c` |

**2.8.1 Pulo e gravidade** ✅. Saída 4,875 px/tick, gravidade 0,109375, queda até 5,03, como no
original: o pulo parado sobe 111 px e fica 92 ticks fora do chão. A saída fica 2 ticks parada (1 no
giro), soltar C zera a subida na hora e o ápice tem um tick em 0. Sem coyote time (o original não
pula depois de sair da borda). `mm_move` deixou de arredondar `y` quando a velocidade vertical é 0.
Teste: `physics()`, `variable_jump()` e `ledge_jump()` em `tests/core_test.c`; o smoke da ROM segura
C nos pulos e passa no BlastEm.

**2.8.2 Velocidades horizontais** ✅. Caminhada acelera 0,1875 por tick até 2,75 (11 ticks), soltar o
direcional desliza a 0,5 por tick, virar leva 7 ticks (com o empurrão de 1 px do original), a corrida
soma 1/16 por tick (+2 em 32 ticks), o ar tem 1,25 (pulo), 1,375 (giro) e 1,0/1,75 (queda), inverter
no ar freia e treme como no original, a bola rola a 3,25 e o pulo de parede deixa o direcional livre.
Só gira quem já se movia ao pular (também no deslize). Teste: `movement()`, `air_steering()` e os
demais em `tests/core_test.c`, com as sequências px a px medidas no Mesen (`scripts/sm_reference.lua`);
o smoke da ROM passa no BlastEm.

**2.8.3 Agachar, bola e pousos**. Agachada fica ao soltar ↓, ↑ na bola volta a agachada, bola não
pula, tempos das transições e dos pousos do original.

**2.8.4 Hitboxes**. Larguras e alturas do original por estado.

**2.8.5 Feixe**. Velocidade inicial 4 px/tick com aceleração de 1/16.

### Fase 3 — Sprites e animação

| # | Etapa | Entrega | Teste |
|---|-------|---------|-------|
| 3.1 ✅ | Pipeline de sprite | Pranchas originais → PNG indexado → `.res` (`scripts/extract_*.py`), convenções em `docs/SPRITES.md`. Extração feita e todos os `.res` compilam no `rescomp`; a Samus carrega no jogo desde a 3.2.1 | Sprite novo carrega seguindo o doc |
| 3.2 ⏭️ | Animações atuais com os sprites originais | Trocar a folha Dread (`player_sprite`) pelas poses de `res/images/samus-original-sgdk/poses` em tudo o que o código já faz. Dividida nas sub-etapas 3.2.1–3.2.7 abaixo, cada uma com commit próprio | Ver a tabela da 3.2 |
| 3.3 | Animações completas | Virar, mirar (cima/baixo/diagonal), agachar, bola, spin | Todas visíveis em sequência |
| 3.4 | Hitbox por estado | `collision_size` varia por estado | Sem prender na parede ao trocar de estado |
| 3.5 | Orçamento de VRAM | Alocação automática de VRAM / streaming de tiles do sprite | `SPR_showVRAMUsage` dentro do orçamento |
| 3.6 | Paletas da Samus | Varia/Gravity e ciclos de cor (charge, speed booster, screw attack, dano, Hyper Beam) por troca de paleta, sem tiles novos | Ciclos visíveis sem trocar tiles |

#### Detalhe da 3.2: o que o código já faz e que pose usar

Levantamento de `src/game.c` (`render`) e `src/core/world.c` / `weapon.c` em 2026-10-12. Antes da 3.2.1 existiam 3 animações (`ANIM_STAND`, `ANIM_WALK`, `ANIM_JUMP`) da folha Dread 40×48, com `SPR_setHFlip` para a esquerda. O tiro congelava a Samus no quadro `shoot_frame` de `ANIM_STAND` por 10 ticks. Poses no formato direita/esquerda; os números são os do jogo (`manifest_poses.csv`).

| Situação no código | Como está hoje | Poses novas (dir./esq.) | Sub-etapa |
|---|---|---|---|
| Parado (`STAND`) | `ANIM_STAND` | `01`/`02` | 3.2.1 |
| Andando / correndo (`WALK`, `run_extra`) | `ANIM_WALK`, timer encurtado na corrida | `09`/`0A` (10 quadros), mesmo encurtamento | 3.2.1 |
| Pulo parado (sem girar) | quadro 0 de `ANIM_STAND` | `4B`/`4C` na saída, `4D`/`4E` subindo, `29`/`2A` caindo, `A4`/`A5` no pouso | 3.2.2 |
| Pulo andando (`spin_jumping`) | `ANIM_JUMP` | `19`/`1A` (9 quadros); pouso `A6`/`A7` | 3.2.2 |
| Pulo de parede | `ANIM_JUMP` | `83`/`84`, seguido de `19`/`1A` | 3.2.2 |
| Som do pulo | `jump_sfx` em todo pulo, cortado no pouso | Só no pulo giratório e no de parede; o pulo parado fica sem o som de giro | 3.2.2 |
| Abaixar (`CROUCH`, hitbox já menor) | sem sprite: aparece em pé | `35`/`36` ao abaixar, `27`/`28` agachado, `3B`/`3C` ao levantar | 3.2.3 |
| Bola (`BALL`) | usa os quadros do giro | `37`/`38` ao entrar, `1D`/`1F` no chão (anima só com `vx` ≠ 0), `31`/`32` no ar (mesmos quadros na prancha), `3D`/`3E` ao sair | 3.2.4 |
| Mirar para cima parado | só no instante do tiro (`shoot_frame` 1) | `03`/`04`, enquanto ↑ estiver segurado | 3.2.5 |
| Atirar andando | congela na pose parada | `0B`/`0C` (canhão estendido, mantém o ciclo das pernas) | 3.2.5 |
| Mirar na diagonal andando (↑/↓ + direção) | congela na pose parada | `0F`/`10` (acima), `11`/`12` (abaixo) | 3.2.5 |
| Atirar agachado / mirar para cima agachado | pose parada; tiro sai na altura de quem está em pé | `27`/`28` com o tiro mais baixo; `85`/`86` para cima | 3.2.5 |
| Atirar ou mirar no pulo parado | pose parada | `13`/`14` (frente), `15`/(falta `16`), `17`/`18` (baixo), `69`/`6A` (diagonal acima), `6B`/`6C` (diagonal abaixo) | 3.2.5 |
| Atirar ou mirar caindo | pose parada | `67`/`68` (frente), `2B`/`2C` (cima), `2D`/`2E` (baixo), `6D`/`6E`, `6F`/`70` (diagonais) | 3.2.5 |
| Atirar no pulo giratório | corta o giro (`spin_jumping = 0`) e usa a pose parada | Mesmo corte; cai nas poses de pulo/queda com mira acima | 3.2.5 |
| Origem do tiro | fixa em `(x + 20, y + 22)` | Tabela por pose com a boca do canhão no quadro 64×64 | 3.2.5 |
| Sprite do tiro | tile 8×8 desenhado à mão (`src/beam_sprite.c`) | `weapons_power_beam` (8 direções; conferir no jogo que é o Power Beam) e `weapons_impact_*` ao bater na parede | 3.2.6 |
| Virar de lado | flip instantâneo | `25`/`26` parado; `2F`/`30` no ar (as faixas `43`/`44` agachado e `87`/`88` caindo usam os mesmos quadros) | 3.2.7 |

**3.2.1 Base** ✅. Um script monta uma folha única do jogador a partir de uma lista de poses (todas têm 64×64, então cada linha da folha é uma animação) e gera o enum das animações. Também:
- A paleta passa a ser `pal_samus_power_suit` e sai o `SPR_setHFlip`.
- O sprite é desenhado em `(x − 12, y − 11)`, para que os pés (linha 58, a última com pixel, cai na linha `y + 47`, a última da hitbox `{8, 8, 32, 48}`). Câmera, hitbox e física não mudam.
- O `player_sprite` (folha Dread) sai do `resources.res`.

Feito: `scripts/build_player_sheet.py` gera `res/images/player/samus_sheet.png` e `inc/player_anim.h` (ver `docs/SPRITES.md`). Ficam provisórios até as sub-etapas seguintes: tiro e mira congelam no quadro parado (a 3.2.5 traz as poses de mira), e o pulo parado usa a pose parada (a 3.2.2 separa o pulo).

Teste: `tests/rom_smoke.c` (`-SmokeTest`, entrada roteirizada) passa com a folha nova, e as capturas do BlastEm mostram a Samus andando para os dois lados, sem troca de cor. Parada à direita também conferida; parada à esquerda só pela captura final.

**3.2.2 Pulos** ✅. O pulo parado tem pose própria em cada fase: saída (`4B`/`4C`, 5 ticks), subida (`4D`/`4E`), queda (`29`/`2A`) e pouso (`A4`/`A5`, 10 ticks, que termina ao começar a andar). O giratório gira em `19`/`1A` e pousa em `A6`/`A7` (15 ticks). O chute de parede mostra `83`/`84` (10 ticks) e depois gira. O som de giro toca só nos pulos que giram. As poses são calculadas no núcleo (`MmPose` e `pose_timer` em `src/core/world.c`), e o `render` só escolhe a linha da folha. O tiro no ar continua congelado no quadro parado até a 3.2.5. Teste: `poses()` em `tests/core_test.c`; `tests/rom_smoke.c` confere os dois pulos na ROM, e as capturas do BlastEm mostram o pulo parado e o giratório.

**3.2.3 Agachar** ✅. Pose de abaixar (`35`/`36`, 5 ticks), agachada em loop (`27`/`28`) e levantar (`3B`/`3C`, 5 ticks, que termina ao começar a andar). O teto que mantém a Samus agachada continua valendo. Tiro agachado segue no quadro parado até a 3.2.5. Teste: `crouch_poses()` em `tests/core_test.c`; `tests/rom_smoke.c` agacha e levanta na ROM, e a captura do BlastEm mostra as três poses.

Correção depois do teste no emulador: a agachada flutuava 11 px (e abaixar/levantar 3 px), porque a prancha original põe essas poses mais altas na caixa. `scripts/build_player_sheet.py` agora alinha os pés de todas as poses de chão na linha 58 (`GROUND_ROW`). Um toque de ↓ mantém a Samus agachada depois de soltar (`MM_TAP_TICKS`, 12 ticks); segurar mais que isso levanta ao soltar. Teste: `crouch_tap()` em `tests/core_test.c` e o trecho de agachar em `tests/rom_smoke.c`.

**3.2.4 Bola** ✅. Entrada por dois toques de ↓, no chão ou no ar (`MM_DOUBLE_TAP_TICKS`, 20 ticks entre os toques; um toque não morfa e apontar para baixo com o tiro não conta). Como no Super Metroid, ↓ com a Samus agachada também morfa, sem limite de tempo: antes, quem agachava com um toque e apertava ↓ de novo depois da janela não virava bola. Poses `MM_POSE_MORPH` (`37`/`38`, 10 ticks), `MM_POSE_BALL` (`1D`/`1F`) e `MM_POSE_UNMORPH` (`3D`/`3E`, 10 ticks); a bola só gira com `vx` ≠ 0, a 2 ticks por quadro. Os quadros da bola são alinhados um a um na linha 58 (`ALIGN_EACH_FRAME` em `scripts/build_player_sheet.py`), porque a extração deixava a bola subir e descer 1 px. A bola não atira e, se pular, continua bola. Teste: `double_tap()` e `ball_poses()` em `tests/core_test.c`; `tests/rom_smoke.c` confere as três poses na ROM, e a captura do BlastEm mostra curvar, rolar e descurvar.

**Correção do pulo de parede (2026-10-13).** Depois do teste no emulador: o pulo de parede saía com o direcional *contra* a parede e em qualquer pulo. Agora exige, como no Super Metroid, pulo giratório, parede encostada (até `MM_WALL_REACH` = 8 px) e direcional para o lado oposto; o impulso vai para o lado apertado, e a pose `83`/`84` passa a olhar para onde a Samus vai. Teste: `wall_jump_rules()`, `wall_climb()` e `poses()` em `tests/core_test.c`.

**3.2.5 Mira e tiro** ✅. O núcleo calcula `MmAim` a cada tick (frente, cima, diagonal acima, baixo no ar, diagonal abaixo), enquanto o direcional estiver segurado, com ou sem tiro. ↓ + direção no chão anda mirando na diagonal abaixo; ↓ sozinho no chão continua agachando. A pose de tiro (`shoot_pose_timer`) fica enquanto A estiver segurado e 10 ticks depois. `player_row()` em `src/game.c` escolhe a linha pela pose, mira, tiro e movimento: 15 ações novas (30 linhas), da tabela acima. Mirar ou atirar substitui as poses curtas (pouso, levantar), mas não a bola, o giro e o chute de parede. As linhas de andar compartilham o ciclo das pernas: trocar entre elas mantém o quadro. As tiras de mira da prancha começam com um quadro de transição, então a folha usa só o quadro mantido (`'03:1'` em `ANIMATIONS`); a pose `16` são os quadros 2–3 da tira `15`. `scripts/build_player_sheet.py` acha a boca do canhão em cada linha (o pixel cinza-verde mais distante na direção da mira) e gera `player_muzzle[]` em `inc/player_anim.h`; `mm_weapon_tick` recebe essa posição e traça do centro do corpo até ela, sem nascer dentro de parede. Teste: `aiming()` e `beam_from_muzzle()` em `tests/core_test.c` (que agora compila `weapon.c`); `tests/rom_smoke.c` passa pelas linhas de mira para cima, tiro andando, diagonal andando e diagonal no pulo, e as capturas do BlastEm mostram essas poses.

**3.2.6 Sprite do tiro** ✅. O tile manual foi substituído pelos oito quadros de
`weapons_power_beam`, escolhidos pela direção da velocidade. Os cinco quadros
laranja de `weapons_impact_r00` aparecem no primeiro contato com a parede,
por 10 ticks; colisão entre corpo e canhão também gera impacto. Cada entrada
do pool reserva quatro tiles e permanece ocupada até o impacto terminar.
`scripts/build_beam_sheet.py` gera as duas tiras com PAL3 compartilhada, sem
perda de cores (ver custo em `docs/SPRITES.md`). Teste: regressões do núcleo
com gcc, `beam_impacts()` para contato varrido/expiração/saída da tela, build
SGDK e `SMOKE PASS` no BlastEm, incluindo os cinco quadros do impacto na ROM.

**3.2.7 Virar.** Animação curta de virada antes de trocar a pose de lado.

**Fora da 3.2.** Ficam para a 3.3:
- Mira diagonal parado (`05`–`08`) e agachado (`71`–`74`): no SNES vem de L/R, e o controle de 3 botões não tem botões livres. **Decidido:** no controle de 6 botões, L = Z (mirar para baixo) e R = Y (mirar para cima); X fica livre para cancelar item. Ainda não implementado no código.
- Moonwalk, pouso atirando (`E6`/`E7`), pouco HP, space jump e screw attack.

**Lacunas da prancha.**
- A pose `16` (pulo parado, esquerda, mirando para cima) não tem faixa própria: são os quadros 2–3 da tira `15`.
- A faixa da `69` diz "Facing left - aiming up-right", provável erro de digitação ("Facing right").

> **Prioridade (2026-10-12)**: a 3.2 vem antes de qualquer outra implementação.

#### Extração (revisão de 2026-10-12)

A primeira extração gerava uma paleta por PNG (365 paletas em 983 quadros), cortava o pivô, declarava um `SPRITE` por quadro, numerava os quadros sem a pose, descartava as paletas e quantizava recortes com várias paletas. O extrator foi refeito (ver `docs/SPRITES.md`):

- Uma tira por pose do jogo (165 poses, 792 quadros), com o número e o nome da pose transcritos das faixas da prancha (`scripts/data/samus_pose_labels.csv`).
- Quadros de 64×64 com pivô comum e esquerda/direita com arte própria (sem `SPR_setHFlip` na Samus).
- Paleta indexada fixa (Power Suit) com as cores originais do SNES; o `rescomp` converte no build. Trajes e ciclos são troca de paleta: 101 paletas extraídas.
- Morte, Crystal Flash e *missile fins* como efeitos com a paleta correspondente. Composições com várias paletas ficam como referência.
- `res/images/*-original-sgdk/` é versionado; só as pranchas originais ficam fora do Git.
- `scripts/extract_sheet_sprites.py` extrai armas, objetos/itens, portas, nave, HUD e inimigos.

Para a 3.2: o tamanho de quadro é 64×64 (8×8 tiles); com a Samus em pé, os pés ficam na linha 59. Se a hitbox atual (`collision_size`) assumir o sprite 40×48 da folha Dread, o deslocamento do sprite em relação à hitbox muda.

### Fase 4 — Tiros e armas

| # | Etapa | Entrega | Teste |
|---|-------|---------|-------|
| 4.1 ✅ | Pool de projéteis | Array estático (sem `malloc`) com posição, direção, vida | 5+ tiros sem queda de fps |
| 4.2 ✅ | Tiro do canhão | Botão A (B ficou para a corrida), 8 direções, cadência, colisão com tiles | Tiro some na parede |
| 4.3 | Mísseis e super mísseis | Munição, seleção, contador | Sem munição não atira |
| 4.4 | Beams | Charge, ice, wave como flags de equipamento | Cada beam altera sprite/dano |

### Fase 5 — Inimigos e combate

| # | Etapa | Entrega | Teste |
|---|-------|---------|-------|
| 5.1 | Entidade de inimigo | Struct + tabela de comportamentos (ponteiro de função) | Inimigo andando e colidindo com o chão |
| 5.2 | Inimigos de Crateria | Zoomer, Skree, Ripper, Geemer (nesta ordem) | Cada um com IA e animação |
| 5.3 | Dano e vida | HP, invencibilidade, knockback, energia da Samus | Morrer e reiniciar a sala |
| 5.4 | Drops | Energia/míssil pequenos | Pegar aumenta contadores |
| 5.5 | Limite de sprites | Culling fora da tela; ≤80 sprites e ≤20 por scanline | Sala cheia sem flicker excessivo |

### Fase 6 — Salas, portas e mundo

| # | Etapa | Entrega | Teste |
|---|-------|---------|-------|
| 6.1 | Formato de sala | `room_def`: tilemap, colisão, spawn, inimigos, portas, tamanho | Crateria 1 e 2 carregadas via `room_def` |
| 6.2 | Ferramenta de exportação | Script (Tiled/Aseprite → `.c`/`.res`) para colisão e entidades | Regenerar sala sem editar array à mão |
| 6.3 | Transição de sala | Portas, fade/scroll, trocar tiles e paletas | Crateria 1 → 2 e volta |
| 6.4 | Streaming do mapa | Verificar limites de ROM/RAM; compressão/tiles de colisão 16x16 | Sem estouro em salas grandes |
| 6.5 | Mapa do mundo | Tabela de salas e conexões de Crateria | Percorrer a região inteira |
| 6.6 | Portas e blocos | Portas azul/vermelha/verde, blocos destrutíveis | Abrir com o tiro correto |

### Fase 7 — Itens, progressão e HUD

| # | Etapa | Entrega | Teste |
|---|-------|---------|-------|
| 7.1 | HUD | Energia, mísseis, mini-mapa mínimo (plano WINDOW) | HUD estável durante o scroll |
| 7.2 | Itens coletáveis | Tanques de energia/míssil | Coletar persiste na sala |
| 7.3 | Upgrades | Morph Ball, Bombas, Varia, High Jump | Habilidades liberam trechos do mapa |
| 7.4 | Save em SRAM | Itens, posição e flags | Salvar, resetar, continuar |

### Fase 8 — Áudio e polimento

| # | Etapa | Entrega | Teste |
|---|-------|---------|-------|
| 8.1 ✅ | Música | Tema de Crateria (XGM/XGM2) | Toca sem engasgar |
| 8.2 ⏭️ | SFX | Tiro, dano, item, porta | Canais PCM não se atropelam |
| 8.3 | Título / game over | Estados de jogo | Fluxo título → jogo → game over |
| 8.4 | Desempenho | Medir `SYS_getCPULoad` no pior caso | CPU < 100% |
| 8.5 | Release | ROM final, README com controles, tag de versão | Roda em BlastEm e em hardware real |

**8.1 Música (2026-10-13).** A trilha veio em MP3 (`res/sound/musics/`, local, fora do Git). O Mega Drive não decodifica MP3, e um MP3 não vira VGM/XGM (FM) automaticamente, então a música toca como PCM de 8 bits, 13,3 kHz mono, no canal 1 do XGM2. `scripts/convert_music.py` (precisa de ffmpeg e numpy) corta cada faixa em introdução, tocada uma vez, e corpo do loop, repetido pelo driver; os dois ficam em múltiplos de 256 amostras, para o `rescomp` não inserir silêncio no ponto do loop. O ponto de loop sai de uma autocorrelação (`--find-loop`). Landing Site: faixa 03, "Arrival on Crateria", com 19,2 s de introdução e loop de 66,7 s. Custo: 1,1 MB de ROM (a ROM foi de 256 KB para 1,4 MB). Com 4 MB, cabem só mais umas duas ou três faixas desse tamanho: para o jogo inteiro será preciso tocar a meia taxa (6,65 kHz), encurtar loops ou, melhor, refazer as músicas em FM (VGM/XGM). Teste: `tests/rom_smoke.c` confere o canal 1 tocando; no overlay de depuração (`MUSIC INTRO`/`LOOP`), a troca para o loop aparece entre 15 s e 24 s.

**8.2, primeira parte (2026-10-13).** Os efeitos originais estão em `res/sound/Sound-Effects/` (WAV 44,1 kHz estéreo; o `rescomp` converte para PCM de 13,3 kHz no build). Ligados ao que já existe: tiro (`beam_shot`, canal 3, no lugar do bipe do PSG), giro (`samus_flip`, canal 2, em loop enquanto a Samus gira, no lugar do antigo `jump.wav`) e pouso (`samus_land`, canal 2, que o giro libera ao pousar). O canal 1 fica para a música (8.1). Os outros efeitos (porta, dano, inimigo, item, mísseis, nave) entram com as etapas que os usam. Teste: `tests/rom_smoke.c` confere que o giro e o tiro tocam PCM na ROM.

## 4. Riscos e decisões em aberto

**Correção validada — pulo em rampas (2026-10-18).** A colisão de subida agora
precede a horizontal: o primeiro tick no ar deixava os pés na altura da rampa,
que era tratada como parede e zerava velocidade e corrida. `slope_spin_takeoff()`
cobre duas orientações, 64 posições subpixel e caminhada/corrida máxima. As
regressões do núcleo e a ROM passaram; `scripts/smoke_test.py` lê o resultado
pela API GDB do BlastEm, sem controle de interface. Ver [CHANGELOG.md](../CHANGELOG.md).

- **Tamanho dos mapas**: Crateria 1 tem 288x156 tiles de 8 px (2304x1248). Avaliar tiles de colisão 16x16 (4x menos dados) ou RLE para o restante do jogo.
- **VRAM**: cada sala precisa de tiles únicos suficientes junto com sprites; planejar tileset compartilhado por região.
- **Propriedade intelectual**: Super Metroid é da Nintendo. O projeto é de estudo/fã e não distribui ROMs. As pranchas originais ficam fora do Git; os recortes convertidos para o SGDK (`res/images/*-original-sgdk/`) são versionados por decisão do mantenedor.
- **Fidelidade da física**: medido no jogo original em 2026-10-13 (`scripts/sm_reference.ps1`, Mesen 2). A tabela de [FISICA.md](FISICA.md#medidas-no-super-metroid-original-mesen-2) lista o que bate e o que difere; falta decidir quais diferenças corrigir.
- **Sprite da Samus por partes** (medido em 2026-10-12): o `rescomp` só reaproveita quadros inteiros (iguais ou espelhados), e cada quadro guarda os próprios tiles na ROM. Com isso, as 165 poses ocupam cerca de 286 KB, e um banco de tiles compartilhado cairia para cerca de 78 KB. Separar tronco e pernas na linha 40 do quadro (como o Super Metroid faz) economiza 34% na corrida com mira: as pernas são iguais nas 4 miras. Na VRAM não muda nada, porque só o quadro atual é carregado. Decisão: usar quadros inteiros na 3.2 e reavaliar na 3.5 se a ROM apertar.
- **ROM > 4 MB**: exigiria mapper SSF.

## 5. Definição de "pronto" de uma etapa

1. Compila sem erros novos e roda no emulador.
2. O teste da tabela foi executado e passou.
3. `README.md` e `docs/` atualizados se a mudança afetar uso, controles ou formato de dados.
4. Etapa marcada como concluída neste plano.
5. **Commit** com mensagem clara e **push** para o remoto (regra em `AGENTS.md`).
