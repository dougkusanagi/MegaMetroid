# Plano de portabilidade: Super Metroid → Mega Drive (MegaMetroid)

Plano para continuar o port de Super Metroid para Mega Drive / Genesis usando o [SGDK](https://github.com/Stephane-D/SGDK).
Cada etapa é pequena, testável no emulador e termina em **commit + push** (ver `AGENTS.md`).

Última revisão: 2026-10-18.

> **Nota (revisão da refatoração `codex/refactor-game-core`)**: o código dessa branch foi incorporado a esta. Avaliação: o núcleo (`src/core/`) é C portátil sem SGDK, em ponto fixo Q24.8, com colisão varrida por pixel, pool estático de projéteis, `src/game.c` como única camada SGDK e testes de regressão em `tests/` (`scripts/test.ps1`, exige MSVC/gcc/clang no host). Isso adianta as etapas 0.3, 0.4, 1.1, 1.2, 2.4, 4.1 e 4.2 e a câmera por zonas. As etapas ✅ acima foram validadas por compilação; o teste de comportamento no emulador ainda deve ser repetido a cada etapa. Não vale aproveitar: `dev` (versão anterior da mesma refatoração, com `player.c`/`physics.c` maiores), `0.54` (reescrita abandonada com arquivos vazios) e as cópias `better-org` / `working-v-col`.

## 1. Estado atual

**Já funciona**
- Cenário de Crateria (fg + bg) como `MAP` do SGDK, com scroll de câmera seguindo a Samus (`updateCamera`).
- Sprite da Samus (5x6 tiles) com animações parada / andando / pulando e flip horizontal.
- Colisão com tiles 8x8 (AABB horizontal + vertical) nas salas de Crateria 1 (288x156) e 2 (160x160).
- Rampas com tile 2 para a direita e tile 3 para a esquerda, incluindo subida e descida.
- Pulo (botão C) com gravidade e SFX PCM via XGM.
- Duas salas têm `room_def`; Crateria 1 é a inicial e ambas carregam pelo mesmo caminho de `game_init`.

**Limitações / dívida técnica atual**
- A ROM normal começa em Crateria 1. Crateria 2 já carrega como sala independente, mas não há transição entre as salas (6.3).
- As quatro portas visuais da sala 1 têm posições e cores, mas seus destinos e interação ainda não foram definidos (6.3/6.6).
- Listas de inimigos estão vazias; comportamento e instâncias entram nas fases 5 e 6.2.
- A exportação de mapa e colisão ainda não é automática (6.2); mapas maiores pedem avaliação de RAM, ROM e streaming (6.4).
- A nave segue decorativa, sem plataforma, save/recarga ou escotilha interativa (fases 6/7).

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
| 0.5 ✅ | Smoke visível no monitor secundário | Janela sem ativação; `--hidden` e fallback oculto | `SMOKE PASS` via GDB no `DISPLAY2` |

**0.5 Smoke visível no monitor secundário** ✅ (2026-10-18). BlastEm é
posicionado no monitor secundário sem ativação. `--hidden` força a execução
oculta; sem monitor secundário o teste permanece oculto. Validado no
`DISPLAY2` com `SMOKE PASS` via GDB.

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
| 2.8 ✅ | Física medida no original | Correções 2.8.1–2.8.5 entregues; limites de fidelidade em `docs/FISICA.md` | Valores do original fixados em `tests/core_test.c` |

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

**2.8.3 Agachar, bola e pousos** ✅. As ações já existiam; esta etapa corrige a
fidelidade ao original, medida no Mesen: agachada permanece após soltar ↓,
independentemente da duração. ↑ ou C desfaz a bola sem impulso de pulo; no chão
termina agachada, e um novo ↑ levanta. A expansão respeita o espaço para agachar,
sem exigir que caiba em pé, e não permite pular agachada para dentro de um teto.
Transições: abaixar/levantar 3 ticks, morfar/desmorfar 6 (3 por quadro).
Pousos: normal 7 (5+2 por quadro), giro 1. Testes de referência no núcleo, nos
dois lados e sob teto baixo; teste da ROM no BlastEm pela API GDB. Medições e
limites restantes em `docs/FISICA.md`; hitboxes originais ficam para 2.8.4.

**2.8.4 Hitboxes** ✅. Perfil de colisão da Samus baseado nos raios arquivados:
10×42 em pé, 10×38 no pulo/queda, 10×24 no giro, 10×32 agachada, 10×14 em bola
e 10×20 mirando diretamente para baixo no ar. Pés são preservados no chão e
o centro nas mudanças aéreas, com deslocamento de 9 px ao morfar no ar observado
na referência. Expansões bloqueadas mantêm a caixa anterior até haver espaço;
pousos sob teto tentam a caixa agachada. A desmorfagem usa 10×32 durante sua
transição, antes de voltar à caixa do estado seguinte. `mm_samus_init` ativa o
perfil no jogo; caixas personalizadas continuam disponíveis para testes genéricos.
Teste: expectativas geradas das capturas versionadas, dois lados, passagens,
expansões bloqueadas, 72 superfícies de rampa e 80 mil ticks nos mapas reais;
ROM confere as seis dimensões pelo BlastEm via API. Sem nova execução do Mesen.
Limites dos quadros transitórios e custo em `docs/FISICA.md`.

**2.8.5 Feixe ✅ (2026-10-18).** Power Beam começa a 4 px/tick nos eixos
cardinais e 683/256 px por eixo na diagonal. Acelera 1/16 por eixo ativo antes
do movimento, inclusive no quadro do disparo (primeiro avanço: 4,0625 px ou
699/256 px por eixo). Mantidos os intervalos 15/25, três slots e impactos por
varredura. Seis novas capturas originais arquivadas; testes comparam trajetórias
e velocidades contra RAM salva e verificam direções, cadência e paredes finas.
ROM validada no BlastEm via API. Componentes herdadas do movimento da Samus
e da câmera do original ainda não são reproduzidas; detalhes em `docs/FISICA.md`.

### Fase 3 — Sprites e animação

| # | Etapa | Entrega | Teste |
|---|-------|---------|-------|
| 3.1 ✅ | Pipeline de sprite | Pranchas originais → PNG indexado → `.res` (`scripts/extract_*.py`), convenções em `docs/SPRITES.md`. Extração feita e todos os `.res` compilam no `rescomp`; a Samus carrega no jogo desde a 3.2.1 | Sprite novo carrega seguindo o doc |
| 3.2 ✅ | Animações atuais com os sprites originais | Folha Dread substituída pelas poses de `res/images/samus-original-sgdk/poses`. Sub-etapas 3.2.1–3.2.7 concluídas, com commits próprios | Ver a tabela da 3.2 |
| 3.3 | Animações completas | Virar, mirar (cima/baixo/diagonal), agachar, bola, spin | Todas visíveis em sequência |
| 3.4 ✅ | Hitbox por estado | Perfil original por estado, entregue na 2.8.4 | Trocas junto a pisos, tetos e paredes sem penetrar tiles |
| 3.5 | Orçamento de VRAM | Alocação automática de VRAM / streaming de tiles do sprite | `SPR_showVRAMUsage` dentro do orçamento |
| 3.6 | Paletas da Samus | Varia/Gravity e ciclos de cor (charge, speed booster, screw attack, dano, Hyper Beam) por troca de paleta, sem tiles novos | Ciclos visíveis sem trocar tiles |

#### Detalhe da 3.2: o que o código já faz e que pose usar

Os relatos das sub-etapas abaixo preservam o histórico da importação dos
sprites. Tempos de agachar/bola/pousos e retenção da agachada foram refinados
na 2.8.3 em 18/10/2026; os valores atuais estão nessa etapa e em `docs/FISICA.md`.

Levantamento de `src/game.c` (`render`) e `src/core/world.c` / `weapon.c` em 2026-10-12. Antes da 3.2.1 existiam 3 animações (`ANIM_STAND`, `ANIM_WALK`, `ANIM_JUMP`) da folha Dread 40×48, com `SPR_setHFlip` para a esquerda. O tiro congelava a Samus no quadro `shoot_frame` de `ANIM_STAND` por 10 ticks. Poses no formato direita/esquerda; os números são os do jogo (`manifest_poses.csv`).

| Situação no código | Como está hoje | Poses novas (dir./esq.) | Sub-etapa |
|---|---|---|---|
| Parado (`STAND`) | `ANIM_STAND` | `01`/`02` | 3.2.1 |
| Andando / correndo (`WALK`, `run_extra`) | `ANIM_WALK`, timer encurtado na corrida | `09`/`0A` (10 quadros), mesmo encurtamento | 3.2.1 |
| Pulo parado (sem girar) | quadro 0 de `ANIM_STAND` | `4B`/`4C` na saída, `4D`/`4E` subindo e descendo (quadros por fase), `29`/`2A` só ao cair de borda, `A4`/`A5` no pouso | 3.2.2 |
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

**3.2.2 Pulos** ✅. O pulo parado tem pose própria em cada fase: saída (`4B`/`4C`, 5 ticks), subida e descida (`4D`/`4E`, com quadros controlados pela fase do salto desde 18/10/2026; `29`/`2A` ficam para a queda de borda) e pouso (`A4`/`A5`, 10 ticks, que termina ao começar a andar). O giratório gira em `19`/`1A` e pousa em `A6`/`A7` (15 ticks). O chute de parede mostra `83`/`84` (10 ticks) e depois gira. O som de giro toca só nos pulos que giram. As poses são calculadas no núcleo (`MmPose` e `pose_timer` em `src/core/world.c`), e o `render` só escolhe a linha da folha. O tiro no ar continua congelado no quadro parado até a 3.2.5. Teste: `poses()` em `tests/core_test.c`; `tests/rom_smoke.c` confere os dois pulos na ROM, e as capturas do BlastEm mostram o pulo parado e o giratório.

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

**3.2.7 Virar** ✅. Três quadros de dois ticks, medidos no Mesen (`turn`,
`turn_moving`, `air_turn_rest`): `25/26` no chão e `2F/30` no ar, escolhidos
pelo lado inicial. O núcleo guarda a duração visual separada da frenagem;
velocidades e transições físicas não mudam. O render retorna à pose normal
ao terminar e mantém prioridade para mira/tiro, bola, giro e chute de parede.
Teste: `turn_animation()` no núcleo; a ROM confere as quatro tiras e seus três
quadros, com `SMOKE PASS` via API GDB do BlastEm. Custo em `docs/SPRITES.md`.

**Fora da 3.2.** Ficam para a 3.3:
- Mira diagonal parado (`05`–`08`) e agachado (`71`–`74`): no SNES vem de L/R, e o controle de 3 botões não tem botões livres. **Decidido:** no controle de 6 botões, L = Z (mirar para baixo) e R = Y (mirar para cima); X fica livre para cancelar item. Ainda não implementado no código.
- Revisar saída e duração da pose do wall jump: a auditoria 6.0.1 confirmou o limite lateral, mas não equivalência dos tempos visuais.
- Moonwalk, pouso atirando (`E6`/`E7`), pouco HP, space jump e screw attack.

**Lacunas da prancha.**
- A pose `16` (pulo parado, esquerda, mirando para cima) não tem faixa própria: são os quadros 2–3 da tira `15`.
- A faixa da `69` diz "Facing left - aiming up-right", provável erro de digitação ("Facing right").

> **Prioridade de 2026-10-12 atendida em 2026-10-18:** a 3.2 está concluída.
> Próxima pendência após 2.8.5: 3.3 (auditar e completar animações e mira).

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
| 6.0 ✅ | Integração visual do Landing Site e da nave | Cenário da referência, céu original, quatro portas azuis fechadas, blocos e nave estática | Seis vistas da ROM verificadas pela VRAM/CRAM no BlastEm |
| 6.0.1 ✅ | Revisão do scroll e relatos de física | Y contínuo no MAP; métricas de CPU/DMA/VInt e novas referências do original | Sem quadros perdidos nos dois saltos medidos; seis vistas validadas |
| 6.1 ✅ | Formato de sala | `room_def`: tilemaps, colisão, spawn, inimigos, portas, zonas de câmera e tamanho | Crateria 1 e 2 inicializadas pelo carregador comum |
| 6.2 ✅ | Ferramenta de exportação | Script (Tiled → `.c`/`.res`) para colisão, spawn, câmera, portas e inimigos | Regenerar sala sem editar array à mão |
| 6.3 ✅ | Transição de sala | Portas, fade, trocar tiles, mapas, paletas e colisão | Crateria 1 → 2 e volta |
| 6.4 ✅ | Limites de ROM/RAM/VRAM | Medir os orçamentos, recusar sala que não cabe na VRAM, provar que a troca não vaza heap; formato da colisão decidido | Sem estouro em salas grandes |
| 6.5 ✅ | Mapa do mundo | Tabela de salas e conexões de Crateria, extraída do original e embutida na ROM | Percorrer a região inteira (no grafo; salas jogáveis conforme forem construídas) |
| 6.6 | Portas e blocos | Portas azul/vermelha/verde, blocos destrutíveis | Abrir com o tiro correto |

**6.0 Integração visual antecipada, incluindo a nave** ✅ (2026-10-18).
Antecipada a pedido do mantenedor. `scripts/build_landing_site.py` usa a sala
extraída da prancha nova de Crateria e a captura arquivada do céu no Mesen.
Restaurados paredes, vegetação, cavernas, plataformas e a base de blocos;
integradas quatro portas azuis fechadas e `starship_hatch_00` em (1057,1101),
com origem conferida na captura original (precisão de um pixel). Nave e portas
são tiles do cenário, sem consumir sprites de hardware adicionais.

As quatro paletas são compartilhadas por tile; as cores de objetos sofrem
aproximação, enquanto as cores efetivas do feixe/impacto são preservadas.
O céu acompanha o scroll vertical e repete a cada 512 px. Mantidos o spawn,
a grade de colisão e os limites atuais de 2304×1248; esta entrega é visual.
Não inclui plataforma/colisão da nave, escotilha, save/recarga, luzes, chuva,
transições, abertura de portas ou destruição. As interações continuam pendentes
nas fases correspondentes. Não se declara equivalência completa ao SNES.

Validação: regressões do host, build SGDK e `SMOKE PASS` no BlastEm via GDB.
`--capture-scenes` lê VRAM/CRAM/SAT pela API e confere cada pixel dos planos
e suas paletas em seis vistas: nave, quatro portas e base de blocos. Capturas
reconstruídas do vídeo ficam em `tests/out/blastem-*.png`. Detalhes e custo em
`docs/SPRITES.md`.

**6.0.1 Scroll contínuo e auditoria dos relatos** ✅ (2026-10-18).
O fundo recebia Y com módulo 512, causando redraw completo nas emendas e dois
VInts perdidos por salto nos cenários medidos. Passa a receber o Y contínuo:
zero VInts perdidos, DMA máxima de 912 bytes no normal e 1.032 no giro.
O smoke inclui os trajetos e recusa regressões. Repetição do horizonte também
confirmada no original na câmera alta; sem alteração arbitrária da arte.
Tiro segurado sem Charge repete no original; wall jump sustenta 1,375 px/quadro,
como o port. Poses/saída do chute permanecem na 3.3. Referências e limites em
`docs/FISICA.md` e `docs/SPRITES.md`; cache reutilizável sem novas sessões Mesen.

**6.1 Formato de sala** ✅ (2026-10-18). `level_def` passa a `room_def`,
com os MAPs, tilesets, paletas, colisão, dimensões, spawn, zonas de câmera,
spawns de inimigos e descritores de portas. Crateria 1 registra as quatro
portas azuis visuais com posições conferidas; destino fica sem conexão até a
etapa 6.3 confirmar rotas. As listas de inimigos continuam vazias até a fase 5.
Crateria 1 e 2 são inicializadas pelo mesmo `game_init` e catálogo `room_get`;
a ROM normal ainda começa na sala 1. Smoke do BlastEm iniciou cada sala em
processo/build separado e confirmou colisão e spawn válidos.

**6.2 Exportador de salas** ✅ (2026-10-20). `rooms/*.tmj` (Tiled) passa a ser a
fonte de cada sala; `scripts/export_rooms.py` gera `src/map_<id>.c`, `inc/map_<id>.h`,
`res/<id>.res` e o catálogo (`inc/map.h`, `src/map.c`). As duas salas atuais foram
convertidas dos arrays escritos à mão e a ROM resultante é idêntica byte a byte à
anterior (mesmo SHA256). O exportador valida o mapa e recusa dados inconsistentes.
Não foi aberto o editor Tiled: o formato segue a especificação JSON, mas a abertura
no programa ainda não foi conferida. Aseprite não é usado. Detalhes em `docs/BUILD.md`.

**6.3 Transição de sala** ✅ (2026-10-20). Encostar na faixa de 8 px da borda
externa de uma porta conectada (`mm_door_touched`, núcleo testado no host) chama
`game_change_room`: fade para preto (20 quadros), libera os mapas, recarrega
tiles, mapas, colisão e paletas da sala de destino, posiciona a Samus em
`target_spawn` mantendo o lado para o qual olha e faz o fade de volta. Música e
mecanismo de sprites seguem sem reinício; o tiro em voo é descartado.
A rota **Landing Site (porta leste) ↔ Crateria 2 (porta oeste) é provisória**:
serve para exercitar a mecânica; a topologia real do original ainda não foi
levantada (6.5). As demais portas continuam sem destino e não fazem nada.
A Samus perde agachada/bola ao atravessar (reinicia em pé). Sem rolagem entre salas;
a abertura da porta por tiro e sua animação ficam na 6.6.

Validação: `doors` no núcleo; no BlastEm, a ROM atravessa a porta leste de verdade
(Landing Site → Crateria 2 → Landing Site), confere em cada chegada a
VRAM (tiles dos dois planos lidos pelo VDP), CRAM, colisão, posição, chão e lado;
uma porta sem rota não transiciona. Depois da ida e volta, as seis vistas do
Landing Site continuam idênticas pixel a pixel (`--capture-scenes`).

**6.4 Orçamentos e limites** ✅ (2026-10-20). Medido na ROM (BlastEm, smoke):

| Recurso | Medida | Folga |
|---------|--------|-------|
| VRAM de tiles do usuário | 1.005 tiles (depois do mecanismo de sprites e da fonte) | Landing Site usa 437 (fg+bg), Crateria 2 usa 121 |
| Heap (SGDK) | 26.972 bytes livres com o Landing Site carregado; maior bloco 9.962 | Três idas e voltas pelas portas devolvem todos os bytes |
| ROM | 1,44 MB de 4 MB | música 1,1 MB; colisão das duas salas 70,5 KB (44.928 + 25.600), cerca de 5% |

`game_room_fits_vram` recusa uma sala cujos tilesets não cabem na VRAM de usuário:
`game_change_room` devolve FALSE antes de tocar a sala atual e `game_init` falha com
mensagem, em vez de sobrescrever o mecanismo de sprites. O smoke confere a folga, o
vazamento e cada chegada. **Não há streaming de mapa**: a sala inteira entra de uma vez
(os MAPs do SGDK já rolam pelo tilemap sem carregar tudo) e a colisão é lida direto
da ROM. **Decisão:** manter a colisão em 1 byte por tile 8×8. Tiles de colisão 16×16
tirariam a precisão das rampas de 45° e da hitbox da Samus, e 2 bits por tile
(÷4, ~17 KB para as duas salas) só compensa se a ROM apertar; a conversão fica no
exportador (6.2), então trocar o formato depois não exige editar salas. O que pesa na
ROM são música e arte, não a colisão. Limite conhecido: o maior bloco livre do heap
(9,9 KB) é menor que o livre total (27 KB) e as futuras salas grandes precisam de
um par de MAPs que caiba nele; a medição por sala entra a cada sala nova no smoke.

**6.5 Mapa do mundo, em passos.**

*6.5.1 Grafo real de Crateria* ✅ (2026-10-20). `scripts/extract_world.py` lê do Super
Metroid original só os cabeçalhos de sala e as tabelas de porta (tamanho em telas, posição
no mapa da área e, por porta, parede, posição, destino e chegada) e grava `rooms/world.json`
(34 salas, 80 portas, sem bytes da ROM, arte ou colisão). A posição de uma porta na sala é a
chegada da porta par da sala vizinha; várias portas entre as mesmas duas salas são casadas em
ordem ao longo da parede. 29 salas se ligam ao Landing Site só por portas de Crateria; as outras
cinco (Forgotten Highway) só se alcançam pelo Wrecked Ship e por Maridia, e 12 portas saem da
região (Wrecked Ship, Maridia, Brinstar, Tourian). Só o Landing Site tem nome registrado: os
demais ficam pelo endereço do cabeçalho (`92FD`...) até serem conferidos. As quatro portas do
Landing Site no port batem com as do original (esquerda y=38 e y=70, direita y=22 e y=70, em
blocos de 16 px). Duas portas (`95D4`) não têm tampa de porta no original (posição 0,0).
Validação: `tests/check_world.py` (grafo simétrico, chegadas dentro das salas, alcançabilidade,
portas do Landing Site contra `crateria_1.tmj`, grafos inválidos recusados).

*6.5.2 Portas pelo grafo* ✅ (2026-10-20). Os mapas Tiled ganham a propriedade `world_node`
(Landing Site = `91F8`; Crateria 2, que é uma sala provisória do projeto anterior, passa a
representar a `93AA`, a sala vizinha da porta leste superior) e cada porta o `link` do grafo.
`target_room`/`target_x`/`target_y` saem dos mapas: o exportador deriva destino e ponto de
chegada de `rooms/world.json` e recusa porta na parede errada, fora da posição do original,
repetida ou ausente, nó repetido e `world.json` inválido. Só a porta leste superior do Landing
Site leva a uma sala construída; as outras três deixam de ser rota provisória e passam a ser
"vizinha ainda não construída" (sem efeito, como antes). Os arquivos gerados saíram idênticos
byte a byte, então a ROM não muda. Portas verticais ainda não têm ponto de chegada definido.

*6.5.3 Tabela na ROM* ✅ (2026-10-20). `export_rooms.py` gera também `inc/world_map.h` e
`src/world_map.c` a partir de `rooms/world.json`: `world_nodes[34]`, com endereço da sala
original, tamanho em telas, posição no mapa, sala do catálogo (`room`, ou `MM_NO_NODE` se
ainda não construída) e as portas (parede, posição, destino, porta par, região de saída).
O núcleo portátil `src/core/worldmap.c` oferece `mm_world_reach` (salas alcançáveis a partir de
uma sala), `mm_world_node_of_room` e `mm_world_consistent` (toda porta é respondida pela par,
na parede oposta). Validação: o teste do host percorre o grafo (29 salas a partir do Landing
Site, 5 no enclave da Forgotten Highway, limites de capacidade, grafo inconsistente recusado);
o smoke da ROM, no BlastEm, confere a tabela embutida, que toda sala do catálogo é um nó cujas
portas ela desenha e que uma porta é conectada exatamente quando a sala do outro lado existe.
**Limite:** só duas das 34 salas existem como mapa jogável (Landing Site e a substituta da
`93AA`); percorrer as demais no jogo depende de construí-las (arte e colisão), o que é trabalho
de conteúdo, não de mecanismo: cada sala nova entra como `.tmj` com `world_node` e `link`.

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
