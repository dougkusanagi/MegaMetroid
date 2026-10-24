# Histórico de alterações

Registro das mudanças entregues, em ordem cronológica inversa. Cada entrada usa
data e hora com fuso explícito (`America/Sao_Paulo`, UTC−03:00), descreve o efeito
no jogo e informa a validação realizada. Novas entregas são acrescentadas neste
mesmo arquivo; não se cria um arquivo por commit. As categorias são **Adicionado**,
**Melhorado**, **Corrigido** e **Removido**, usadas quando aplicáveis.

O registro detalhado começa em 18/10/2026. A entrada anterior abaixo foi
reconstituída do commit indicado; o histórico anterior completo permanece no Git
(`git log`) e as entregas por etapa estão em [docs/PLANO.md](docs/PLANO.md).

## 2026-10-23 21:58:24 −03:00 (America/Sao_Paulo) — Fundo e prioridade das portas (6.6.4)

### Melhorado

- Porta completa de 32×64 com transparência real no lugar da faixa opaca de
  16×64; a tampa azul gravada no mapa foi removida pela máscara original e o
  metal fica à frente da Samus. Some o retângulo preto que cobria o cenário.
  Custo: 32 tiles por porta, mesmos 16 sprites de hardware, sem RAM nova.

### Corrigido

- Troca da paleta do tiro (PAL3) passa a entrar na fila de DMA do VBlank. O DMA
  imediato no meio do quadro competia com o VBlank e corrompia o comando do VDP,
  congelando o BlastEm logo após disparar um míssil.
- Smoke: removido o modo benchmark (`-b`), que derrubava o BlastEm; a saída do
  emulador vai para `tests/out/blastem.log`. Leituras de CRAM e textos do smoke
  protegidos contra o VBlank.

**Validação:** `scripts/test.ps1 -Compiler gcc`, `tests/check_assets.py`, build
`-SmokeTest`, `python scripts/smoke_test.py`: `SMOKE PASS` em 12,1 s, acelerado,
sem áudio, headless; zero VInts perdidos, heap estável. `--capture-scenes` e
`scripts/check_room_video.py`: oito vistas, com 532 e 420 pixels da Samus cobertos
pelo metal nas portas oeste e leste.

## 2026-10-22 22:02:24 −03:00 (America/Sao_Paulo) — Smoke acelerado e headless

### Corrigido

- Negociação GDB com `qSupported` desativado, IPv4 e timeout remoto explícito;
  preserva a execução acelerada sem áudio. Headless agora é o padrão, `--visible`
  exige monitor secundário. AGENTS.md proíbe usar tempo real como contorno de falha.
- O resultado informa duração e modo para tornar regressões de tempo visíveis.

**Validação:** build SGDK `-SmokeTest` e `python scripts/smoke_test.py`:
`SMOKE PASS` em 11,5 s, acelerado, sem áudio, headless; heap estável e zero
VInts perdidos nos saltos medidos. Nenhuma alteração de configuração global do PC.

## 2026-10-22 21:56:11 −03:00 (America/Sao_Paulo) — Moonwalk frontal (3.3.5)

### Adicionado

- Moonwalk frontal no chão, ligado por padrão: tiro segurado e direção para
  trás mantêm o canhão, com entrada parada, 0,5 px/tick e seis quadros de
  16 ticks. Soltar tiro permite virar; perder o chão encerra a animação.
- Duas capturas headless do original com a opção ligada, preservadas com
  origem e limites. Host e ROM comparam 110 posições e quadros por lado;
  host cobre também saída, paredes, bordas e opção desativada.
- Doze desenhos na ROM e 4 bytes por corpo no 68000; reserva de VRAM mantida.
  Piso controlado: mais 2 KB estáticos apenas no smoke.

**Validação:** `scripts/test.ps1 -Compiler gcc`, build SGDK `-SmokeTest`,
`python scripts/smoke_test.py --realtime --timeout 180`: `SMOKE PASS` via GDB
no monitor secundário sem ativação. Heap estável nas três viagens; zero VInts
perdidos; CPU máxima média 40/45%, DMA 912/1.032 bytes nos saltos medidos.
Limites: diagonais e transições BF/C0 ainda pendentes; entrada com velocidade
prévia, rampas e demais combinações não comprovadas pela referência plana.

## 2026-10-22 21:43:44 −03:00 (America/Sao_Paulo) — Pouso atirando (3.3.4)

### Adicionado

- Poses E6/E7 no pouso com tiro frontal: cinco ticks no quadro 0 e dois no 1,
  medidos nos dois sentidos no Mesen headless. Capturas e origem arquivadas.
- Smoke compara os sete quadros do sprite após VBlank, volta à pose parada,
  prioridade da mira e cancelamento ao andar. Quatro desenhos novos na ROM,
  sem aumentar a reserva de VRAM ou a struct do jogo; fixture de 2 KB só no smoke.

**Validação:** `scripts/test.ps1 -Compiler gcc`, build SGDK `-SmokeTest` e
`python scripts/smoke_test.py --realtime`: `SMOKE PASS` via GDB no monitor
secundário, sem ativação. Heap estável em três viagens, zero VInts perdidos;
CPU máxima média 39/45%, DMA 912/1.032 bytes nos saltos medidos. Duas tentativas
aceleradas anteriores falharam na negociação GDB antes da execução da ROM.
Limites: salto normal, tiro frontal segurado e equipamento do save de referência.

## 2026-10-22 21:37:00 −03:00 (America/Sao_Paulo) — Saída do wall jump (3.3.3)

### Corrigido

- O chute de parede preserva a posição no primeiro tick e começa a acelerar no
  seguinte, com caixa transitória de 24 px antes dos 38 px. Reutiliza o cache
  original, sem nova execução do Mesen, novos tiles ou campos de RAM.
- Testes host e ROM comparam 80 velocidades laterais por lado, posição inicial,
  caixas e animação. O teste de subida libera o botão entre chutes sucessivos.

**Validação:** `scripts/test.ps1 -Compiler gcc`, build SGDK `-SmokeTest` e
`python scripts/smoke_test.py`: `SMOKE PASS` via GDB no monitor secundário,
sem ativação. Heap estável em três viagens; zero VInts perdidos nos saltos.
A aplicação à esquerda é simétrica; trajetória vertical completa não comprovada.

## 2026-10-22 21:33:03 −03:00 (America/Sao_Paulo) — Animação do wall jump (3.3.2)

### Melhorado

- O chute de parede mantém a pose durante o giro no ar e usa os tempos do cache
  original `wall_open`: dois quadros de cinco ticks e ciclo de giro 3/2 ticks.
  A caixa de 38 px permanece no chute; o relógio visual é reiniciado ao sair,
  evitando índices fora da tira normal após atirar. Sem novos tiles ou RAM na ROM padrão.
- A build de smoke gera as expectativas diretamente das medições versionadas.
  Host e ROM comparam 80 quadros por lado e verificam interrupção por tiro e pouso.

**Validação:** `scripts/test.ps1 -Compiler gcc`, build SGDK `-SmokeTest` sem novos
avisos e `smoke_test.py --capture-scenes`: `SMOKE PASS`, máscara zero e seis cenas
VDP aprovadas. BlastEm no monitor secundário via GDB; heap estável nas viagens,
CPU máxima média nos saltos medidos 39/45%, DMA 912/1.032 bytes, VInts perdidos zero.
Não executado Mesen. Limites: medição original só para a direita; saída física
um tick antecipada e caixa transitória do primeiro tick ainda pendentes.

## 2026-10-22 21:04:35 −03:00 (America/Sao_Paulo) — Aviso de porta sem travamento

### Corrigido

- A atualização do aviso de porta bloqueia interrupções durante os comandos de
  escrita no VDP, evitando conflito com scroll/DMA do VBlank e o travamento no BlastEm.
- Restaurado `ROOM NOT BUILT YET` na borda da porta leste inferior do Landing Site,
  cuja sala de destino ainda não foi construída.
- Removido o reposicionamento temporário de depuração; o smoke entra e sai da
  porta três vezes, confere o texto, sua remoção e a permanência na mesma sala.

**Validação:** `scripts/test.ps1 -Compiler gcc`, build SGDK `-SmokeTest` e
`smoke_test.py --capture-scenes`: `SMOKE PASS`, seis cenas VDP aprovadas,
heap estável nas três viagens entre salas e nenhum VBlank perdido nos saltos
medidos. BlastEm visível no monitor secundário, via GDB, sem retirar o foco.

## 2026-10-21 22:34:00 −03:00 (America/Sao_Paulo) — Mísseis e supers na ROM (4.3.2)

### Adicionado

- **START** troca a arma em jogo (Power Beam → míssil → super, pulando compartimentos vazios);
  X continua reservado para cancelar item no controle de 6 botões.
- `res/images/player/missile.png` (gerada por `scripts/build_missile_sheet.py`): uma folha de
  16 quadros 16×16 com os oito quadros do míssil e os oito do super, na ordem de direção do beam,
  em 11 cores opacas mais transparência.
- Enquanto a arma está selecionada ou há projétil na tela, a PAL3 usa a paleta da folha de mísseis;
  ao voltar para o beam (inclusive automaticamente quando a munição zera) a PAL3 do beam é
  restaurada. A seleção pedida durante um tiro fica pendente até o projétil e o impacto saírem.
- Sons de míssil, super e clique de troca (`missile_sfx`, `super_sfx`, `select_sfx`), nos canais 3
  e 4, e impacto do míssil desenhado com o quadro do beam (a arte própria entra com as explosões).

### Corrigido

- O míssil e o super não avançam no tick em que nascem, como medido no original (o beam mantém o
  avanço imediato). Mísseis abrem portas azuis; vermelhas e verdes já estavam no núcleo desde 4.3.1.

**Validação:** `./scripts/test.ps1 -Compiler gcc` (todos os testes de núcleo, incluindo
`measured_missiles` e `missile_loadout`) e `./scripts/build.ps1 -SmokeTest` seguido de
`python scripts/smoke_test.py` no BlastEm via API GDB: `SMOKE PASS`, com o novo check
`missile_weapon` (START, paleta, quadro por direção, munição, porta azul abrindo com o míssil,
super e persistência de munição/seleção na troca de sala). Limites: sem arte de explosão, um
projétil por disparo de super e contador de munição apenas na 7.1.

## 2026-10-21 22:19:00 −03:00 (America/Sao_Paulo) — Núcleo de mísseis e super mísseis (4.3.1)

### Adicionado

- `src/core/weapon.c` ganhou seleção de arma (`MM_WEAPON_BEAM/MISSILE/SUPER`), munição
  (`missiles`/`supers`) e o tipo do tiro em `MmBeam.kind` (`MM_SHOT_*`, agora em
  `inc/core/projectile.h`), compartilhado com as portas.
- Física medida no original: mísseis saem a 1,0 px/tick por eixo e aceleram 80 Q8.8 no
  cardinal e 70 na diagonal; supers aceleram 256 no cardinal e 182 na diagonal (tabela
  `SuperMissileAccelerations`, sem o `$0010` genérico). Dano 100/300 e cadência 10/20 ticks.
- Mísseis e supers só disparam em um toque novo (sem repetição), consomem munição, voltam para
  o beam quando a munição zera e não têm tempo de vida: somem na parede ou fora da tela. A troca
  de arma pedida por `mm_weapon_cycle` fica pendente enquanto há tiro na tela, porque a paleta
  do projétil muda com o tipo.
- O tiro que para na tampa de uma porta agora informa seu tipo (`beam->kind`) ao `mm_lock_hit`:
  vermelha aceita mísseis e supers, verde só supers.
- Munição e seleção sobrevivem à troca de sala (`mm_weapon_clear_shots`); o carregamento inicial
  de 5 mísseis e 0 supers é provisório até os itens coletáveis (7.2).

**Validação:** `./scripts/test.ps1 -Compiler gcc` (novos `measured_missiles` e
`missile_loadout`, incluindo as 48 amostras de velocidade medidas no Mesen, arquivadas em
`tests/reference/super_metroid/{missile_right,missile_up_right,super_right}.csv.gz` e
`tests/out/sm_missiles.h`) e build SGDK da ROM normal. Ainda não há controle nem arte de
míssil na ROM: entram na 4.3.2.

## 2026-10-21 21:41:33 −03:00 (America/Sao_Paulo) — Tampa de porta animada com a arte original (6.6.3)

### Adicionado

- A tampa da porta virou um sprite de 16×64 px com os cinco quadros originais de
  `doors_door_r00`: fechada, três formas de abertura de 6 ticks e batente aberto. O cenário
  continua com a porta fechada e o sprite opaco cobre a faixa durante a abertura.
- `scripts/build_door_cap.py` gera `res/images/doors/door_cap.png` na PAL1 do Landing Site;
  pixels transparentes viram preto opaco para o azul do fundo não vazar.
- Portas criadas antes de Samus e feixes, com profundidade explícita: a Samus e os tiros passam
  na frente da tampa.

### Melhorado

- O smoke no BlastEm confere quadro, espelhamento, posição, visibilidade e profundidade da
  tampa nos quadros 1–4, além de manter ocultos os sprites de portas inexistentes.

### Corrigido

- A nota da 6.6.1 dizia que só o bojo azul de 8 px mudava. A faixa que muda tem 16 px: os
  quadros 3 e 4 também trocam o metal em x=16..23, e o quadro 4 deixa o batente aberto.

**Limites:** portas verticais ficam ocultas (nenhuma sala construída tem uma); a PAL1 da
Crateria 2 não tem cores de porta e sua área de porta é preta, então a tampa reaproveita a
paleta do Landing Site sem regressão visível.

**Validação:** `./scripts/test.ps1 -Compiler gcc`, build SGDK `-SmokeTest` e
`python scripts/smoke_test.py`: `SMOKE PASS` via GDB no BlastEm, sem ativar a janela. Custo:
2 sprites de hardware e 16 tiles por porta; oito portas reservam 16 sprites e 128 tiles, dentro
da região de 420 tiles do `SPR_init()`.

## 2026-10-21 20:53:08 −03:00 (America/Sao_Paulo) — Portas abrem com tiro e só então deixam passar (6.6.2)

### Adicionado

- A tampa de cada porta fechada entra na colisão da sala a cada tick: bloqueia a Samus e os
  feixes como uma parede. Um feixe que para sobre a tampa vale como tiro daquela porta, usando
  a caixa de 5×5 do projétil para alcançar a borda; a porta azul começa a abrir no impacto.
- A travessia de sala exige a porta já aberta, após os 18 ticks medidos. Ao entrar numa sala, a
  porta par da sala de origem fica aberta atrás da Samus.
- `RoomDoorDef.link` liga cada porta desenhada à porta correspondente no grafo do mundo; o
  exportador emite o índice e o jogo resolve a porta de chegada pelo `peer` da porta de origem.

### Melhorado

- O smoke no BlastEm passa a cobrir o ciclo das portas: porta sem rota bloqueia, abre com um
  tiro e deixa a Samus na faixa sem trocar de sala; a rota leste faz três idas e voltas, com a
  porta par aberta em cada chegada e heap estável.

**Validação:** `./scripts/test.ps1 -Compiler gcc` (novo caso da caixa do tiro em `door_locks`),
build SGDK `-SmokeTest` e `python scripts/smoke_test.py`: `SMOKE PASS` via GDB no BlastEm, sem
ativar a janela. Heap livre igual em três viagens: 26.600 bytes.

## 2026-10-21 17:50:31 −03:00 (America/Sao_Paulo) — Mira diagonal de seis botões (3.3.1)

### Adicionado

- Y/Z miram na diagonal acima/abaixo sem andar, em pé ou agachada, com oito poses
  originais e origem do feixe calculada por pose. Y+Z retornam à mira do direcional.
- Testes das duas posturas e direções, soltura, poses na ROM e direção dos disparos.

**Validação:** `scripts/test.ps1 -Compiler gcc`, build SGDK `-SmokeTest` e
`python scripts/smoke_test.py`: `SMOKE PASS` via GDB no BlastEm, sem ativar a janela.
Sem VInts perdidos nas medições; heap estável nas três viagens entre salas.

## 2026-10-20 20:40:00 −03:00 (America/Sao_Paulo) — Núcleo de portas com tampa e cores (6.6.1)

### Adicionado

- `mm_door_cap`, sólidos extras em `MmWorld` e `src/core/doorlock.c`: a tampa de uma porta
  fechada bloqueia Samus e tiros; abrir exige o tiro da cor (azul: qualquer um; vermelha:
  5 mísseis ou 1 super; verde: super). Duração do quadro de abertura provisória (não medida).

Duração medida no original (3 formas de 6 quadros, 18 quadros do impacto à passagem), arquivada em
`tests/reference/super_metroid/door_open` (`sm_reference_cache.py import-door`).

**Validação:** `scripts/test.ps1` com o novo `door_locks` (geometria da tampa nas quatro
orientações, regras por cor, Samus parada pela tampa e livre depois, tiro barrado). O jogo
ainda não usa o núcleo (6.6.2).

## 2026-10-20 20:10:00 −03:00 (America/Sao_Paulo) — Tabela do mundo na ROM (6.5.3)

### Adicionado

- `world_nodes[34]` na ROM (`src/world_map.c`, gerado de `rooms/world.json`) e o núcleo
  `src/core/worldmap.c`: alcançabilidade a partir de uma sala, sala do catálogo de cada nó e
  verificação de que toda porta é respondida pela porta par. Concluída a etapa 6.5.

**Validação:** `scripts/test.ps1` (novo teste do mapa: 29 salas alcançáveis do Landing Site,
5 no enclave da Forgotten Highway) e `SMOKE PASS` no BlastEm via GDB, com a conferência da
tabela embutida; `build.bin` refeito. Só Landing Site e a substituta da `93AA` são jogáveis.

## 2026-10-20 19:50:00 −03:00 (America/Sao_Paulo) — Portas ligadas ao grafo real (6.5.2)

### Melhorado

- As portas dos mapas Tiled apontam para o grafo de Crateria (`world_node` no mapa, `link` na
  porta). O exportador deriva sala de destino e ponto de chegada de `rooms/world.json` e
  recusa porta na parede errada, fora da posição do original, repetida ou ausente. A rota
  Landing Site ↔ Crateria 2 deixa de ser escrita à mão: Crateria 2 representa a sala `93AA`.

**Validação:** `tests/check_rooms.py` (novos casos de recusa), `tests/check_world.py` e
`scripts/test.ps1`; os oito arquivos gerados saíram byte a byte idênticos, portanto a ROM
não mudou.

## 2026-10-20 19:25:00 −03:00 (America/Sao_Paulo) — Grafo real de Crateria (6.5.1)

### Adicionado

- `scripts/extract_world.py` e `rooms/world.json`: as 34 salas de Crateria do original, com
  tamanho, posição no mapa e as 80 portas (parede, posição, sala de destino, chegada).
  Nenhum dado da ROM é guardado, só o desenho do mapa. 29 salas se ligam ao Landing Site por
  portas de Crateria; 5 (Forgotten Highway) só pelo Wrecked Ship e por Maridia.
- `scripts/world_graph.py` e `tests/check_world.py`, ligado a `scripts/test.ps1`.

**Validação:** `scripts/test.ps1` completo (gcc do host), incluindo o novo teste: grafo
simétrico, chegadas dentro das salas, alcançabilidade e as quatro portas do Landing Site
idênticas às de `rooms/crateria_1.tmj`. O jogo ainda não usa o grafo (6.5.2).

## 2026-10-20 19:23:35 −03:00 (America/Sao_Paulo) — Smoke no BlastEm sem limite de 60 fps

### Melhorado

- `scripts/smoke_test.py` roda o BlastEm sem a trava de tempo real: o driver de áudio
  `disk` do SDL com atraso zero, definido só no processo do teste, deixa a emulação
  correr na velocidade da CPU. O smoke completo passou de ~58 s para ~11 s, sem mudar
  a configuração do usuário. `--realtime` mantém o comportamento anterior.

**Validação:** smoke completo oculto, visível no monitor secundário e com
`--capture-scenes`; todas as linhas `MM_SMOKE_*`, `NORMAL_*`, `SPIN_*`, `VRAM_*` e
`HEAP_*` idênticas às da execução em tempo real.

## 2026-10-20 20:45:00 −03:00 (America/Sao_Paulo) — Orçamentos de VRAM, heap e ROM (6.4)

### Adicionado

- `game_room_fits_vram`: uma sala cujos tilesets não cabem na VRAM de usuário é
  recusada. `game_change_room` devolve FALSE antes de mexer na sala atual; `game_init`
  falha com mensagem em vez de sobrescrever os tiles de sprites.
- O smoke mede e imprime VRAM disponível e usada por sala e o heap livre antes e depois
  de cada uma de três idas e voltas pelas portas; falha se faltar VRAM ou se sobrar vazamento.

### Melhorado

- Orçamentos medidos e registrados em `docs/PLANO.md` (6.4): 1.005 tiles de VRAM livres
  (Landing Site usa 437, Crateria 2 usa 121), 26.972 bytes de heap sem perda pelas trocas,
  ROM em 1,44 MB de 4 MB. Decisão: a colisão segue em 1 byte por tile 8×8 (≈5% da ROM).

**Validação:** smoke completo no BlastEm via GDB com as métricas acima; as três idas e voltas
devolvem exatamente os mesmos 26.972 bytes. Regressões do host inalteradas (sem mudança no núcleo).

## 2026-10-20 20:45:00 −03:00 (America/Sao_Paulo) — Orçamentos de VRAM, heap e ROM (6.4)

### Adicionado

- `game_room_fits_vram`: uma sala cujos tilesets não cabem na VRAM de usuário é
  recusada. `game_change_room` devolve FALSE antes de mexer na sala atual; `game_init`
  falha com mensagem em vez de sobrescrever os tiles de sprites.
- O smoke mede e imprime VRAM disponível e usada por sala e o heap livre antes e depois
  de cada uma de três idas e voltas pelas portas; falha se faltar VRAM ou se sobrar vazamento.

### Melhorado

- Orçamentos medidos e registrados em `docs/PLANO.md` (6.4): 1.005 tiles de VRAM livres
  (Landing Site usa 437, Crateria 2 usa 121), 26.972 bytes de heap sem perda pelas trocas,
  ROM em 1,44 MB de 4 MB. Decisão: a colisão segue em 1 byte por tile 8×8 (≈5% da ROM).

**Validação:** smoke completo no BlastEm via GDB com as métricas acima; as três idas e voltas
devolvem exatamente os mesmos 26.972 bytes. Regressões do host inalteradas (sem mudança no núcleo).

## 2026-10-20 20:10:00 −03:00 (America/Sao_Paulo) — Transição entre salas (6.3)

### Adicionado

- Ao empurrar a faixa de 8 px na borda externa de uma porta com destino, o jogo
  escurece, troca tiles, mapas, colisão e paletas, coloca a Samus no ponto de chegada
  (mantendo o lado para o qual olha) e clareia. `game_change_room` faz a troca sem
  reiniciar sprites nem música.
- Rota provisória para exercitar a mecânica: porta leste do Landing Site ↔ porta oeste
  de Crateria 2 (nova, em `rooms/crateria_2.tmj`). A topologia real fica para a 6.5.
- `mm_door_rect`/`mm_door_touched` no núcleo, com teste no host para as quatro direções.

### Melhorado

- `game_init` compartilha o carregamento de sala com a transição.
- Exportador e smoke usam o tamanho real da porta por direção (32×64 ou 64×32); o smoke
  valida também que cada destino de porta é um ponto livre da sala de chegada.
- O timeout padrão do smoke passou de 45 para 120 s (o teste completo leva cerca de 50 s).

**Validação:** regressões do host; smoke no BlastEm via GDB atravessando as portas de
verdade (ida e volta), conferindo VRAM, CRAM, colisão, posição e lado em cada chegada,
e que a porta sem rota não transiciona. As seis vistas do Landing Site continuam
idênticas pixel a pixel depois da ida e volta. O fade em si não foi capturado quadro a
quadro; só seus estados inicial e final. Sem rolagem entre salas; a Samus reinicia em pé.

## 2026-10-20 19:03:00 −03:00 (America/Sao_Paulo) — Exportador de salas (6.2)

### Adicionado

- `rooms/*.tmj`: cada sala é um mapa Tiled com colisão, spawn, zonas de câmera,
  portas e inimigos. `scripts/export_rooms.py` gera `src/map_<id>.c`, `inc/map_<id>.h`,
  `res/<id>.res` e o catálogo `inc/map.h`/`src/map.c`; `--check` acusa arquivo gerado
  desatualizado. Crateria 1 e 2 foram convertidas dos arrays escritos à mão.
- `tests/check_rooms.py`, no `scripts/test.ps1`: confere os arquivos gerados, que pintar
  um tile altera só aquela célula, que porta/inimigo/zona aparecem nas tabelas e que
  mapas inválidos (célula vazia, spawn ou porta fora do mapa, destino inexistente,
  tamanho de imagem divergente, sala fora do índice) são recusados.

### Melhorado

- O build de smoke compila todo `src/map*.c`; uma sala nova não exige editar o script.
- Removidos os `extern` de largura/altura que nunca tiveram definição.

**Validação:** regressões do host e `check_rooms.py`; a ROM padrão reconstruída ficou
idêntica à anterior (SHA256 e403423354d8…1be0124) e o smoke completo passou no BlastEm
via GDB. O formato Tiled segue a especificação, mas não foi aberto no editor.

## 2026-10-18 18:53:45 −03:00 (America/Sao_Paulo) — Formato de sala (6.1)

### Adicionado

- `room_def` reúne MAPs, tilesets, paletas, colisão, dimensões, spawn, câmera,
  inimigos e portas. O catálogo contém Crateria 1 e 2; a inicialização comum
  agora carrega qualquer uma das duas.
- Quatro descritores de portas azuis registram as posições visuais de Crateria 1.
  Rotas ainda não confirmadas ficam sem destino; listas de inimigos permanecem vazias.
- `smoke_test.py` aceita ROM/tabela de símbolos próprios, e o build aceita
  diretório de saída específico para validar uma variante em separado.

### Corrigido

- A validação não presume mais que todas as salas têm 288×156 tiles: verifica
  dimensões, colisão, spawn e descritores de porta para ambas as salas.
- Cada conjunto de `-Define` usa uma pasta própria de objetos, evitando reutilizar
  objetos compilados com flags diferentes.

**Validação:** regressões do host; smoke completo de Crateria 1 e inicialização
de Crateria 2 no BlastEm via GDB, ambos visíveis no monitor secundário. Em cada
sala, a ROM conferiu recursos, dimensões e spawn sem colisão.

## 2026-10-18 17:47:59 −03:00 (America/Sao_Paulo) — Scroll contínuo e auditoria dos relatos (6.0.1)

### Corrigido

- Fundo deixa de receber Y com módulo 512. O MAP já repete os blocos; o módulo
  provocava um salto artificial de 511 px e redraw completo nas emendas.
- Nos dois saltos medidos no BlastEm NTSC, VInts perdidos caíram de 2 para 0.
  Fila DMA máxima: normal 4.368 → 912 bytes; giro 4.872 → 1.032 bytes. As maiores
  médias de carga em oito quadros caíram de 42/46% para 28/32%, respectivamente.
- Documentação do wall jump deixa de citar o antigo impulso de 2,75 px/tick
  e um bloqueio de direção que já não existia no código.

### Adicionado

- Regressão na ROM mede os saltos e rejeita VInts perdidos e DMA acima de 2 KB;
  verifica que a câmera atravessou a emenda e que o giro realmente aconteceu.
- Quatro probes do céu original e um wall jump sem teto arquivados com origem
  e limites. O tiro segurado sem Charge já se repetia na referência existente;
  o wall jump sustenta 1,375 px/quadro, como o port. Forças não foram alteradas.

**Validação:** regressões do host, cache e ROM no BlastEm via GDB; seis vistas
comparadas por VRAM/CRAM. Medições limitadas aos trajetos descritos nos docs.
O horizonte também repete na captura original com câmera Y=43; arte preservada.
Chuva/HDMA e tempos/poses do chute continuam pendentes, sem declarar equivalência
completa ao SNES.

## 2026-10-18 17:06:19 −03:00 (America/Sao_Paulo) — Landing Site e nave (6.0)

### Adicionado

- Nave estática no centro do Landing Site e quatro portas azuis fechadas com
  os recortes originais. Integradas ao MAP, sem sprites de hardware adicionais.
- Captura visual pontual do Mesen arquivada com checksums, origem e limites;
  regenerador da sala usa esses dados, sem repetir a execução do emulador.
- `--capture-scenes`: readback de VRAM/CRAM/SAT pela API GDB do BlastEm,
  reconstrução de seis vistas e conferência de pixels, paletas e scroll.

### Corrigido

- A sala deixa de carregar o cenário editado anterior. O recorte da prancha
  nova restaura paredes, vegetação, cavernas, plataformas e base de blocos.
- Céu claro substituído pelo BG2 medido no original, com nuvens neutras e
  repetição vertical de 512 px. Cores de nave/portas adaptadas por tile aos
  quatro bancos; feixe e impacto conservam exatamente suas cores no VDP.

**Validação:** regressões completas do núcleo com gcc, checksums do cache,
limites de paleta, transparência/cores do feixe, build SGDK sem novos avisos e
`SMOKE PASS` no BlastEm. Seis vistas aprovadas com comparação dos planos e CRAM;
janela de teste no monitor secundário, sem ativação. Cenário: 437 tiles / 13.984
bytes de VRAM, nenhum sprite adicional; ROM alinhada em 1.441.792 bytes.

**Limites:** entrega visual; mantidos spawn, colisão e tamanho atuais. Portas
não abrem, blocos não quebram e a nave ainda não tem plataforma, save/recarga,
escotilha ou animação de luzes. Chuva e demais fases de iluminação ficam pendentes.

## 2026-10-18 17:01:00 −03:00 (America/Sao_Paulo) — Smoke no monitor secundário (0.5)

### Melhorado

- O BlastEm do smoke test aparece no primeiro monitor secundário, centralizado
  na área útil e sem ativação ou alteração da ordem das janelas. A validação
  continua pela API GDB, sem injetar entrada de interface.
- A janela inicia oculta e só aparece após o posicionamento. `--hidden`, ausência
  de monitor secundário ou falha ao posicionar mantêm a sessão oculta. Apenas
  o processo iniciado pelo teste é movimentado e encerrado.

**Validação:** `SMOKE PASS` no BlastEm via GDB; janela do teste encontrada e
posicionada no `DISPLAY2`, à esquerda do principal. Opções Win32
`SWP_NOACTIVATE`, `SWP_NOZORDER` e `SW_SHOWNOACTIVATE` preservam o foco.

## 2026-10-18 16:37:00 −03:00 — Velocidade e aceleração do Power Beam (2.8.5)

### Corrigido

- O feixe usava 12 px/tick constantes. Agora começa a 4 px/tick nos eixos
  cardinais e 683/256 por eixo diagonal, acelerando 1/16 por eixo ativo antes
  de cada movimento. O primeiro avanço já ocorre no quadro do disparo.
- Preservadas colisões por varredura, impacto animado, três slots e cadência
  de 15/25 ticks. Disparos podem atingir uma parede no próprio quadro de criação.

### Adicionado

- Seis capturas direcionais do original, com velocidades de projétil e Y com
  subpixel, arquivadas para reutilização sem iniciar o Mesen novamente.
- Expectativas de trajetória geradas da RAM salva; testes de oito direções,
  cadência, toques rápidos, pool cheio e colisão em alta velocidade. A ROM
  verifica também velocidades e posições no 68000, com resultado pela API.

**Validação:** regressão do núcleo com gcc, build SGDK e `SMOKE PASS` no
BlastEm via API GDB, em sessão oculta. Os componentes de movimento da Samus/
câmera herdados pelo projétil original permanecem pendentes; limites registrados
em `docs/FISICA.md`.

## 2026-10-18 16:25:15 −03:00 — Hitboxes originais por estado (2.8.4)

### Melhorado

- A Samus usa caixas de 10 px de largura, com alturas de 42 em pé, 38 no pulo
  normal, 24 no giro, 32 agachada e 14 em bola. Mirar diretamente para baixo no
  ar usa 20 px. As medidas vêm das capturas de RAM já arquivadas.
- Mudanças de caixa preservam o apoio no chão ou o centro no ar. A expansão é
  adiada se estiver bloqueada; pousos sob teto tentam a caixa agachada.
- Morfar cancela o giro; desmorfar usa a caixa de transição antes de retornar
  ao estado seguinte. Nenhuma nova execução do Mesen foi necessária.
- Mantido suporte a caixas personalizadas nos testes genéricos do núcleo.

### Adicionado

- Expectativas de hitbox geradas das capturas originais versionadas e testes
  do perfil real: ambos os lados, transições, passagens estreitas, tetos,
  72 superfícies de rampa e 80 mil ticks nos mapas de Crateria.
- A ROM verifica pela API as seis dimensões de colisão durante o roteiro.

**Validação:** testes do núcleo com gcc, pulo normal a 111 px/92 ticks, build
SGDK e `SMOKE PASS` no BlastEm via API GDB. Documentados os limites dos raios
de um único quadro nas transições do original.

## 2026-10-18 16:17:35 −03:00 — Referência do original reutilizável

### Adicionado

- Arquivo de 71 cenários medidos no Mesen, com 20.442 amostras de RAM e entradas
  por quadro, em CSV comprimido sem perdas. Manifesto com origem, datas,
  checksums e variações de hitbox observadas; sem ROM ou save state no repositório.
- Importador/validador das capturas, integrado aos testes do host.

### Melhorado

- `AGENTS.md` orienta reutilizar os dados salvos e iniciar o Mesen somente
  quando faltar cobertura, surgir dúvida/contradição ou mudar a referência.
- Documentados os limites: cenários finitos não descrevem toda a lógica do jogo;
  raios transitórios e capturas antigas incompletas não são descartados.

**Validação:** checksums e sequência das 20.442 amostras, regressões do núcleo,
build SGDK e teste da ROM pela API GDB do BlastEm. Nenhuma nova execução do
Mesen foi necessária para arquivar as medições existentes.

## 2026-10-18 16:03:11 −03:00 — Fidelidade de agachar, bola e pousos (2.8.3)

### Corrigido

- A Samus permanece agachada após soltar ↓, inclusive depois de segurá-lo por
  mais tempo. ↑, movimento ou pulo permitem levantar quando há espaço.
- ↑ ou C desfaz a bola sem impulso de pulo. No chão, termina agachada e só
  outro aperto de ↑ levanta; segurar o botão não conta como um novo aperto.
- Sair da bola no chão exige espaço para agachar, sem exigir a altura em pé.
  Teto baixo também impede um pulo agachado que colocaria a Samus dentro dele.
- Transições de abaixar/levantar passam a 3 ticks; morfar/desmorfar a 6.
  Pouso normal dura 7 ticks e pouso de giro 1, com quadros controlados pela
  simulação, conforme medição no original.

### Melhorado

- `AGENTS.md` registra a disponibilidade do Mesen 2, ROM e save state original,
  e exige medir o original antes de ajustar comportamentos de fidelidade.
- O teste pela API GDB informa uma máscara dos grupos que falharam.
- Documentação distingue as ações já implementadas dos ajustes de fidelidade;
  hitboxes originais continuam reservadas para a etapa 2.8.4.

**Validação:** sete cenários no Mesen, regressões do núcleo com gcc (tempos,
dois lados, tetos, botões segurados e mapas reais), build SGDK e teste da ROM
no BlastEm via API GDB.

## 2026-10-18 15:47:57 −03:00 — Virada da Samus (etapa 3.2.7)

### Adicionado

- Animação de virada original nos dois lados: `25/26` no chão e `2F/30` no ar.
  São três quadros de dois ticks, com a tira escolhida pelo lado inicial da
  Samus, conforme medição no Mesen. Depois, retorna à pose de movimento.
- Regressões de duração, orientação e cancelamento por giro/bola; teste na ROM
  verifica os três quadros, nos dois lados, no chão e durante o pulo.

### Melhorado

- A transição visual de direção acompanha a simulação sem alterar aceleração,
  frenagem ou velocidade. Mira e tiro mantêm suas poses; bola, giro e chute de
  parede mantêm suas animações próprias.
- `AGENTS.md` exige teste da ROM no BlastEm via API GDB, sem skills de controle
  do computador nem automação da interface, com sessão oculta e isolada.
- Concluída a sequência 3.2.1–3.2.7 de importação das animações atuais.

**Validação:** medição no Mesen (`turn`, `turn_moving`, `air_turn_rest`), geração
da folha, testes do núcleo com gcc, build SGDK e `SMOKE PASS` pela API do BlastEm.

## 2026-10-18 15:41:13 −03:00 — Descida do pulo sem giro

### Corrigido

- O pulo sem giro mantém a animação própria de pulo durante a descida,
  em vez de trocar no ápice para a pose de queda de borda.
- Os quadros deixam de repetir em loop: o quadro de subida fica até iniciar
  a descida; os três quadros seguintes duram quatro ticks cada, e o último
  permanece até pousar. O avanço segue os ticks da simulação também em PAL.

### Melhorado

- A medição do original no Mesen agora registra também quadro e contador da
  animação, permitindo reproduzir a comparação com o SNES.

**Validação:** sequência medida no Mesen (`jump_stand`), regressões do núcleo
para ambos os lados e pulos longos/curtos, build SGDK e teste da ROM pela API
GDB do BlastEm verificando a tira e os quadros renderizados para os dois lados.

## 2026-10-18 15:38:07 −03:00 — Pulo giratório em rampas

### Corrigido

- Pular girando enquanto sobe uma rampa de 45° deixa de zerar a velocidade
  horizontal e o bônus de corrida. A subida é resolvida antes da colisão
  horizontal, evitando interpretar a superfície sob os pés como uma parede.

### Adicionado

- Regressão para decolagem nas duas orientações de rampa, em 64 posições
  subpixel por orientação, caminhando e com corrida máxima.
- Teste da ROM por `scripts/smoke_test.py`, usando a API GDB do BlastEm e um
  resultado explícito em RAM. O emulador roda oculto, sem automação de interface;
  o script encerra somente o processo que iniciou e recusa uma porta já ocupada.
- Este histórico centralizado, com data, hora, categorias e validação; link no
  README e instrução de manutenção no AGENTS.md.

**Validação:** testes do núcleo com gcc (incluindo 40.000 ticks nos mapas reais),
build SGDK da ROM de teste e `SMOKE PASS` pela API GDB do BlastEm.

## 2026-10-18 15:30:43 −03:00 — Power Beam e impactos (`2dd0f52`)

### Melhorado

- Power Beam usa a arte original nas oito direções, com paleta compartilhada
  entre projétil e impacto, sem perda de cores.

### Adicionado

- Impacto de cinco quadros no contato com paredes, com duração de dez ticks.
- Geração reproduzível das tiras de tiro e impacto; testes de contato varrido,
  duração do efeito e saída da tela sem impacto.

### Removido

- Tile manual que representava o tiro.

**Validação registrada na entrega:** testes do núcleo, build SGDK e `SMOKE PASS`
no BlastEm. ROM padrão reconstruída após o commit.
# 2026-10-22 18:24:40 -03:00

## Corrigido
- Ao se aproximar da porta conectada do Landing Site, a tela agora mostra “A: SHOOT TO OPEN”. Isso explica por que caminhar até a porta fechada não troca de sala; depois de abrir, a travessia continua automática. Verificado no smoke do BlastEm pela API GDB.

