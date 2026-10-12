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
| 2.6 | Agachar e bola | Hitbox menor, animações, passagens estreitas | Entrar em túnel de 1 tile |
| 2.7 | Spin jump / wall jump | Pulo giratório; wall jump depois | Subir poço com wall jump |

### Fase 3 — Sprites e animação

| # | Etapa | Entrega | Teste |
|---|-------|---------|-------|
| 3.1 | Pipeline de sprite | Aseprite → PNG → `resources.res`, convenções de nome/animação documentadas | Sprite novo carrega seguindo o doc |
| 3.2 | Animações completas | Virar, mirar (cima/baixo/diagonal), agachar, bola, spin | Todas visíveis em sequência |
| 3.3 | Hitbox por estado | `collision_size` varia por estado | Sem prender na parede ao trocar de estado |
| 3.4 | Orçamento de VRAM | Alocação automática de VRAM / streaming de tiles do sprite | `SPR_showVRAMUsage` dentro do orçamento |

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
| 8.1 | Música | Tema de Crateria (XGM/XGM2) | Toca sem engasgar |
| 8.2 | SFX | Tiro, dano, item, porta | Canais PCM não se atropelam |
| 8.3 | Título / game over | Estados de jogo | Fluxo título → jogo → game over |
| 8.4 | Desempenho | Medir `SYS_getCPULoad` no pior caso | CPU < 100% |
| 8.5 | Release | ROM final, README com controles, tag de versão | Roda em BlastEm e em hardware real |

## 4. Riscos e decisões em aberto

- **Tamanho dos mapas**: Crateria 1 tem 288x156 tiles de 8 px (2304x1248). Avaliar tiles de colisão 16x16 (4x menos dados) ou RLE para o restante do jogo.
- **VRAM**: cada sala precisa de tiles únicos suficientes junto com sprites; planejar tileset compartilhado por região.
- **Propriedade intelectual**: Super Metroid é da Nintendo. O projeto é de estudo/fã e não deve distribuir ROMs ou assets extraídos do jogo original.
- **Fidelidade da física**: medir o jogo original (frames de pulo, velocidades) antes de fixar constantes.
- **ROM > 4 MB**: exigiria mapper SSF.

## 5. Definição de "pronto" de uma etapa

1. Compila sem erros novos e roda no emulador.
2. O teste da tabela foi executado e passou.
3. `README.md` e `docs/` atualizados se a mudança afetar uso, controles ou formato de dados.
4. Etapa marcada como concluída neste plano.
5. **Commit** com mensagem clara e **push** para o remoto (regra em `AGENTS.md`).
