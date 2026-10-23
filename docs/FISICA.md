# Física da Samus

Constantes do núcleo (`inc/core/world.h`), o que elas significam e como foram medidas.
Etapa 2.2 do [plano](PLANO.md).

## Unidades

- Posição e velocidade são **Q24.8**: `MM_UNIT` = 256 unidades por pixel. A soma de
  `x` e `vx` é exata, sem arredondar a fração. Valores de posição e velocidade ficam
  em `int`, sem `fix16` do SGDK, para o núcleo seguir portável e testável no host.
- Um **tick** é um passo de simulação a 60 Hz. No PAL, `game_update` roda 6 ticks a
  cada 5 quadros, então a física tem o mesmo ritmo nos dois sistemas.
- Velocidades em px/tick. Gravidade em px/tick².

## Constantes

Referência SM: valores NTSC em `bank_90.asm` do
[disassembly de Super Metroid](https://github.com/InsaneFirebat/sm_disassembly),
rótulos `SamusPhysicsConstants_*`. As medidas no jogo original estão em
[Medidas no Super Metroid original](#medidas-no-super-metroid-original-mesen-2).

| Constante | Valor no projeto | Equivale a | Referência SM (NTSC) |
|---|---|---|---|
| `MM_SPEED` | 704 | 2,75 px/tick | Velocidade de caminhada, medida |
| `MM_WALK_ACCEL` / `MM_WALK_SNAP` | 48 / 480 | +0,1875 por tick; passando de 1,875 vai a 2,75 | Medido: 2,75 no 11º quadro |
| `MM_DECEL` | 128 | −0,5 px/tick | Soltar o direcional, virar e inverter no ar; medido |
| `MM_TURN_TICKS` | 7 | 7 ticks, o do aperto incluído | Virada no chão, no pulo normal e na queda; medido |
| `MM_RUN_ACCEL` | 16 | 1/16 px/tick por tick | Corrida: medido, +2 em 32 ticks |
| `MM_RUN_EXTRA_MAX` | 512 | +2 px/tick | Corrida: medido (4,75 no total) |
| `MM_AIR_ACCEL` | 192 | +0,75 px/tick | Aceleração no ar e na bola; medido |
| `MM_JUMP_AIR_SPEED` / `MM_SPIN_AIR_SPEED` / `MM_FALL_AIR_SPEED` | 320 / 352 / 256 | 1,25 / 1,375 / 1,0 px/tick | Pulo normal, giro, queda; medido |
| `MM_BALL_SPEED` | 832 | 3,25 px/tick | Bola rolando; medido |
| `MM_JUMP` | 1248 | 4,875 px/tick | `InitialYSpeeds_Jumping` = `$04E0` → 4,875 px/frame |
| `MM_GRAVITY` | 28 | 0,109375 px/tick² | `YSubAccelerationInAir` = `$1C00` → 0,109375 px/frame² |
| `MM_MAX_FALL` | 1280 | 5 px/tick (para de acelerar ao chegar; fica em 5,03) | Medido: 5,03 px/frame |
| `MM_JUMP_STILL` | 2 | 2 ticks parada na saída do pulo normal; 1 no giro | Medido: pose `4B` e mais 1 quadro sem subir |
| `MM_WALL_JUMP` | 1184 | 4,625 px/tick | Pulo de parede: `InitialYSpeeds_WallJumping` = `$04A0` → 4,625 px/frame |
| `MM_WALL_REACH` | 8 | 8 px | Distância máxima da parede para o pulo de parede (decisão do projeto) |

O movimento vertical segue o Super Metroid (2.8.1): a Samus anda e depois a gravidade é somada,
o ápice tem um tick com velocidade 0 (o original guarda velocidade e direção separadas), soltar C
na subida zera a velocidade na hora e não existe pulo depois de sair de uma borda (sem coyote time).

O movimento horizontal segue o original (2.8.2). No chão: +0,1875 por tick até 1,875 e depois 2,75;
soltar o direcional desliza com −0,5 por tick (na saída, mais 1 px no primeiro tick); virar leva 7 ticks
(o aperto e a velocidade antiga caem juntos, depois um empurrão de 1 px e a aceleração do zero); a corrida
soma 1/16 por tick até +2. No ar: pulo normal a 1,25 (soltar o direcional para na hora), giro a 1,375 e queda
a 1,0 alternando com 1,75, ambos mantendo a inércia, e bola a 3,25. Inverter no ar freia em 0,5 por tick
e passa por um tremor de 0,75 px; o pulo parado com direção no mesmo tick não gira (só gira quem já se
movia, também durante o deslize). O pulo de parede tira do repouso e deixa o direcional livre.

Outras velocidades de referência no SM, para etapas futuras: pulo com Hi-Jump
`$0600` (6,0 px/frame), pulo de parede `$04A0` (4,625 px/frame).

## Medidas

Medidas por `physics()` em `tests/core_test.c`: chão plano, hitbox 8x16, pulo a partir
do repouso ou andando, com C segurado.

| Medida | Port | Super Metroid |
|---|---|---|
| Altura do pulo parado | 111 px | 111,09 px |
| Tempo fora do chão | 92 ticks | 92 quadros |
| Alcance do giro depois de 10 ticks andando | ~127 px (`1,875 + 91 × 1,375`) | ver a comparação abaixo; o cenário `walk_spin` bate no teto |
| Queda máxima | 5,03 px/tick | 5,03 px/quadro |
| C segurado 2 / 3 / 10 / 30 ticks | 0 / 4 / 35 / 95 px | 0 / 4,9 / 35,9 / 95,2 px |
| Pular 1 tick depois de sair da borda | não pula | não pula |

Os testes `physics()`, `variable_jump()` e `ledge_jump()` fixam esses valores em
`tests/core_test.c`.

## Medidas no Super Metroid original (Mesen 2)

Medido em 2026-10-13 com `scripts/sm_reference.ps1` (ver [BUILD.md](BUILD.md)), lendo a RAM da
Samus a cada quadro. Chão plano à direita da nave no Landing Site, sem itens além da Morph Ball
(dada pelo script nos cenários de bola). O SNES roda a 60 quadros por segundo, o mesmo ritmo do tick,
então px/quadro e px/tick são comparáveis. `python scripts/sm_reference_report.py` reproduz os números.

| Medida | Super Metroid | Port | Bate? |
|---|---|---|---|
| Caminhada, velocidade máxima | 2,75 px/quadro | 2,75 | sim |
| Caminhada, aceleração | +0,1875 por quadro; 2,75 no 11º quadro (~10 px) | igual | sim (2.8.2) |
| Soltar o direcional | −0,5 por quadro; desliza ~16 px em 7 quadros | igual | sim (2.8.2) |
| Virar no chão | 7 quadros (pose 25/26): 2,75 … 0,25, 0, empurrão de −1, depois −0,1875 … | igual; do repouso 0, 0, −0,75, 0, −0,75, 0, −0,75 | sim (2.8.2) |
| Corrida (B) | +1/16 por quadro, +2 em 32 quadros (4,75 no total) | igual | sim (2.8.2) |
| Pulo, velocidade de saída | 4,875 px/quadro | 4,875 | sim (2.8.1) |
| Gravidade | 0,109375 px/quadro² | 0,109375 | sim (2.8.1) |
| Pulo parado, altura e tempo no ar | 111 px, pico no 46º quadro, 92 quadros no ar | 111 px, 92 ticks | sim (2.8.1) |
| Saída do pulo | 2 quadros sem subir (pose 4B e mais um); no giro, 1 | igual | sim (2.8.1) |
| Queda máxima | 5,03 px/quadro, a partir de ~46 quadros caindo | 5,03 | sim (2.8.1) |
| Soltar o pulo na subida | velocidade vertical vai a 0 na hora | igual | sim (2.8.1) |
| Pulo mais curto | ~5 px (segurando 3 quadros; 1–2 quadros não saem do chão) | igual | sim (2.8.1) |
| Pular logo depois de sair da borda | não pula, nem no 1º quadro | igual (era coyote de 5 ticks) | sim (2.8.1) |
| Velocidade no ar: pulo normal / giro / queda | 1,25 / 1,375 / 1,0 px/quadro (+ bônus da corrida) | igual | sim (2.8.2) |
| Pulo correndo, no ar | 3,375 (1,375 + 2) | igual | sim (2.8.2) |
| Inverter o direcional no ar | freia 0,5 por quadro, tremor de 0,75, depois a nova direção (pulo, giro e queda têm sequências próprias) | igual | sim (2.8.2) |
| Quando gira | só se já andava ao pular (também no deslize sem direcional); direção + pulo parado é pulo normal (pose 51) | igual | sim |
| Pulo de parede, subida | 4,625 px/quadro | 4,625 | sim |
| Pulo de parede, lado | 1,375 px/quadro, direcional livre | igual | sim (2.8.2) |
| Pouso | pulo normal: 7 quadros (A4); giro: 1 quadro (A6/A7) | 7 / 1 ticks | sim (2.8.3) |
| Agachar (↓) | transição de 3 quadros; fica agachada ao soltar, mesmo segurando ↓ por muito tempo | igual | sim (2.8.3) |
| ↓ agachada | vira bola, transição de 6 quadros | igual | sim (2.8.3) |
| ↓ no ar | o 1º mira para baixo, o 2º vira bola | dois toques em até 20 ticks | sim |
| ↑ na bola | volta para agachada (6 quadros); outro ↑ levanta (3 quadros) | igual, segurado não conta como novo aperto | sim (2.8.3) |
| Pulo na bola, sem Spring Ball | desfaz a bola, sem impulso de pulo, no chão ou no ar | igual; no chão termina agachada | sim (2.8.3) |
| Bola rolando | 3,25 px/quadro, acelera +0,75 por quadro | igual | sim (2.8.2) |
| Hitbox (largura × altura) | em pé 10×42, pulo 10×38, giro 10×24, agachada 10×32, bola 10×14 | mesmas dimensões estáveis; mira para baixo no ar 10×20 | sim (2.8.4; ver limites transitórios abaixo) |
| Feixe (Power Beam), base sem movimento herdado | cardinal 4 px/quadro; diagonal 683/256 por eixo; aceleração 1/16 por eixo ativo | mesmos valores, primeiro avanço já acelerado | sim (2.8.5; limites abaixo) |
| Intervalo de tiro | 15 quadros apertando; segurado dispara de novo a cada 25 | 15 / 25 | sim |
| Feixes ao mesmo tempo | ao menos 3 (o limite não apareceu nos cenários) | 3 | — |
| Míssil, saída | 1 px/quadro por eixo em movimento (256 Q8.8), sem avançar no quadro de criação | igual | sim (4.3.1) |
| Míssil, aceleração | 80 Q8.8 no cardinal, 70 na diagonal | igual | sim (4.3.1) |
| Super míssil, aceleração | 256 Q8.8 no cardinal, 182 na diagonal | igual | sim (4.3.1) |
| Cadência de míssil/super | 10 / 20 quadros | igual | sim (4.3.1) |
| Dano de míssil/super | 100 / 300 | igual | — (sem inimigos) |

Os quadros de pose (`4B`, `A4`, …) são os números de pose do Super Metroid (`$0A1C`), os mesmos da
folha de sprites. As velocidades batem com `SamusXSpeedTable_Normal` em `bank_90.asm`: aceleração
`$0030`, máxima 2,75 no chão, 1,25 no pulo normal, 1,375 no giro e no pulo de parede, 1,0 na queda
e 3,25 na bola; desaceleração 0,5.

## Feixe (2.8.5)

- Power Beam: base cardinal `$0400`, diagonal `$02AB` por eixo, velocidades
  Q8.8. Aceleração `$0010` em cada eixo ativo, com o sinal da direção; eixos
  parados continuam em zero. O primeiro movimento já usa 1040/256 = 4,0625 px
  ou 699/256 = 2,73046875 px por eixo diagonal. Não há normalização por √2.
- Fontes: tabelas `BeamSpeeds_Horizontal_Vertical`, `BeamSpeeds_Diagonal` e
  `ProjectileAccelerations` em [bank_90.asm](https://github.com/InsaneFirebat/sm_disassembly/blob/master/src/bank_90.asm),
  conferidas com as novas capturas `beam_up`, `beam_down`, `beam_up_right`,
  `beam_down_right`, `beam_up_left` e `beam_down_left` no Mesen em 18/10/2026.
  Só faltavam medições direcionais; as horizontais já estavam arquivadas.
- As novas capturas incluem Y com subpixel e `shot_vectors`, no formato
  `slot:direção:vx:vy:tipo` (entradas separadas por espaço). Velocidades são inteiros
  Q8.8 com sinal. Tipo `8000` é o feixe vivo; `8700` é impacto e não deve ser
  usado para deduzir aceleração ou velocidade máxima.
- O validador gera `tests/out/sm_beams.h` com 16 amostras iniciais exatas de
  `beam_up` e `beam_up_right`, sem componentes herdadas nesses trechos. O núcleo
  compara cada velocidade e posição acumulada; as outras direções usam a
  simetria das tabelas. A ROM executa as mesmas operações no 68000 e mantém os
  testes de renderização/expiração do impacto pelo BlastEm via API.
- Cadência de 15/25 ticks, três slots, origem no canhão e colisões por varredura
  permanecem. O feixe agora avança no tick de criação e pode atingir uma parede
  já nesse tick. Não há novo estado por projétil nem recursos gráficos novos.
- Limites: o original soma componentes do deslocamento da Samus/câmera na
  criação. Elas aparecem, por exemplo, nos disparos aéreos para baixo e não são
  reproduzidas nesta etapa de velocidade base/aceleração. O porte mantém a
  vida de 60 ticks e seu recorte de tela; os dados finitos não confirmam esses
  limites como idênticos ao SNES. A observação antiga de “até ~6” era um trecho
  antes da colisão, não uma velocidade máxima comprovada.

## Mísseis e super mísseis (4.3)

Medido em 2026-10-21 no Landing Site, com os cenários `missile_right`,
`missile_up_right` e `super_right` de `scripts/sm_reference.ps1`, gravados a cada quadro e
arquivados em `tests/reference/super_metroid/*.csv.gz` (48 amostras válidas no total).
`python scripts/sm_reference_cache.py` valida os cenários e gera `tests/out/sm_missiles.h`.

- Saída: 1 px/quadro por eixo em movimento (`$0100` Q8.8). Diferente do feixe, o projétil
  **não avança no quadro em que nasce**: a primeira posição só muda no quadro seguinte.
- Aceleração do míssil: 80 Q8.8 (`$0040` + o `$0010` genérico) no cardinal e 70 (`$0036` +
  `$0010`) na diagonal. Super míssil: 256 no cardinal e 182 na diagonal, da tabela
  `SuperMissileAccelerations`, sem somar o `$0010` genérico.
- Cadência: 10 quadros para o míssil e 20 para o super, sem repetição por segurar o botão.
- Tipo na RAM: `$8100`/`$8200` identificam míssil/super vivos; o dano é 100/300.
- Limites: os cenários cobrem três direções e poucos quadros de voo; a simetria das tabelas
  cobre as demais. Não foram medidos ricochete, limite simultâneo de projéteis nem a explosão.

## Agachar, bola e pousos (2.8.3)

Conferido no Mesen em 18/10/2026, com os cenários `crouch_morph`, `down_hold`,
`morph_jump`, `morph_up_hold`, `morph_air_jump`, `jump_stand` e `walk_spin`.
Exemplo: `./scripts/sm_reference.ps1 -Scenario morph_up_hold -Out tests/out/ref_morph_up_hold.csv`.

- `35/36` (abaixar) e `3B/3C` (levantar): 3 ticks cada.
- `37/38` (morfar) e `3D/3E` (desmorfar): 6 ticks, 3 por quadro.
- `A4/A5` (pouso normal): 7 ticks, quadro 0 por 5 e quadro 1 por 2.
- `A6/A7` (pouso do giro): 1 tick, antes de voltar ao movimento.
- ↓ mantém agachada depois de soltar, tanto após um toque quanto após segurar.
- ↑ ou pulo desfaz a bola sem impulso vertical. No chão, termina agachada;
  segurar ↑ não levanta até haver outro aperto. No ar, a transição usa a caixa
  agachada antes de voltar à caixa de pulo (perfil 2.8.4).
- A caixa de destino precisa caber. No chão, basta espaço para agachar;
  tentar levantar ou pular sob teto que impede a caixa em pé mantém agachada.

`measured_postures()` e `poses()` fixam os tempos medidos e verificam os dois
lados, botão segurado, teto baixo e desmorfagem sem pulo. A ROM testa retenção
da agachada, retorno da bola para agachada e os quadros de morfar/desmorfar
pelo BlastEm via API. As larguras e alturas foram atualizadas na 2.8.4 abaixo.

## Rampas (45°)

- Os tiles 2 e 3 são rampas de 45°. Em `mm_rect_blocked`, o tile 2 bloqueia a
  linha de pixels abaixo da diagonal (altura `7 - coluna`), e o tile 3 faz o
  inverso. Assim a altura de cada coluna de pixel já está definida pelo tile, sem
  lógica separada de rampa.
- `mm_move` sobe 1 px por passo de 1 px quando está no chão, nos dois sentidos,
  então a Samus sobe e desce rampas de 45° sem quicar.
- O teste `slope_sweep()` cai sobre cada superfície de rampa de Crateria 1 que cabe
  no hitbox em pé, anda para a direita, para a esquerda e para a direita de novo.
  Resultado: 71 superfícies percorridas, 0 paradas indevidas e 0 quadros no ar com
  chão a 1 px dos pés.
- Duas superfícies (colunas 284 e 285) ficam em uma passagem mais estreita que o
  hitbox em pé e não são percorridas. Elas dependem da bola (2.6).
- Rampas de 22,5° não existem no mapa de Crateria 1 e não foram implementadas. Vão
  entrar com um tipo de tile novo.

## Hitboxes (2.8.4)

| Estado | Caixa (px, relativa ao corpo) | Regra |
|---|---|---|
| Em pé | 10×42 (`{15,6,25,48}`) | Padrão no chão e decolagem |
| Pulo / queda | 10×38 (`{15,8,25,46}`) | Pulo normal, queda e chute de parede |
| Giro | 10×24 (`{15,15,25,39}`) | Pulo giratório |
| Agachada | 10×32 (`{15,16,25,48}`) | Pés mantidos no chão; também durante desmorfagem |
| Bola | 10×14 (`{15,34,25,48}`) | No chão ou no ar |
| Mira diretamente para baixo no ar | 10×20 (`{15,17,25,37}`) | Pose `17/18`, fora do giro |

- A origem visual do porte continua a mesma. No chão, mudar de caixa mantém os
  pés; no ar, mantém o centro de colisão. Morfar no ar desloca o centro 9 px para
  baixo, como observado em `morph_air_twice`. Pousar restabelece a caixa do chão
  preservando o apoio dos pés. O alinhamento dos sprites de chão não muda.
- Uma expansão só é aplicada se couber. Se atirar para sair do giro sob teto não
  permite crescer de 24 para 38 px, a caixa compacta fica até haver espaço. No
  pouso tenta-se agachar quando a caixa em pé não cabe, sem penetrar o teto.
- Os tamanhos esperados dos testes são gerados de `tests/reference/super_metroid/`,
  por pose e raios mais frequentes. Os CSVs preservam também raios transitórios:
  no original, alguns mudam um quadro depois da pose. Esta etapa reproduz as
  dimensões estáveis e as regras de encaixe; não replica toda a ordem interna
  de atualização dos quadros transitórios do SNES nem prova todos os casos.
- `mm_samus_init` ativa esse perfil no jogo. `mm_body_init` mantém caixas
  personalizadas para os testes genéricos de física e colisão. A medida do pulo
  normal também é repetida com o perfil real: 111 px e 92 ticks no ar.
- `samus_tunnels()` confere passagens de 16 px para a bola de 14 px e de 32 px
  para agachar, além de expansão aérea bloqueada. Um túnel de 8 px continua menor
  que a bola original, portanto não é um requisito válido para esse equipamento.
- `samus_slope_sweep()` percorre 72 superfícies de Crateria 1 sem paradas indevidas
  nem flutuação. Os mapas reais recebem 80 mil ticks, divididos entre perfil
  genérico e Samus, incluindo pulo, morfo, desmorfo e tiro.
- Custo: 96 bytes de tabelas em ROM e 4 bytes por corpo para selecionar o perfil;
  sem heap, sprites, tiles ou paletas novos. Caixas iguais retornam sem nova
  consulta de colisão; não foi feita medição de ciclos em hardware.
- Agachar, bola e suas transições têm poses próprias desde a fase 3.
- Sob teto baixo, um pulo agachado só é aceito se a caixa em pé couber (2.8.3).

## Pulo giratório e pulo de parede (2.7)

- **Giratório**: pular enquanto já se anda vira `spin_jumping`. A pose de giro dura o
  voo e para ao tocar o chão ou ao atirar.
- **Parede**: como no Super Metroid, só no pulo giratório (ou logo depois de outro pulo
  de parede, que também gira), encostado numa parede sólida e com o direcional apontando
  para o lado oposto a ela; aí apertar C faz um pulo de parede. "Encostado" aceita a
  parede até `MM_WALL_REACH` = 8 px atrás. O pulo dá `MM_WALL_JUMP` = 4,625 px/tick
  para cima. Na horizontal, acelera de 0,75 até 1,375 px/tick, conserva a inércia
  quando se solta o direcional e permite mudar de direção; não há `MM_WALL_LOCK`.
  Usa o estado SPIN. Pulo
  parado, direcional contra a parede ou sem direcional não fazem pulo de parede
  (`wall_jump_rules()` em `tests/core_test.c`).
- O teste `wall_climb()` sobe um poço de 40 px de largura, do chão (y 112) até y 40,
  exigindo pelo menos 2 impulsos de parede, para que um
  pulo do chão não baste.
- Sem deslizar na parede: a queda continua no ritmo normal enquanto encosta.

## Limites e próximos passos

- A integração segue a ordem do Super Metroid: `y += vy` e depois `vy += g`, com a saída
  parada e o ápice em 0. Assim a sequência de velocidades é a mesma do original, quadro a quadro.
- O pulo completo, medido acima, exige C segurado durante toda a subida. Soltar C antes zera a
  subida na hora; soltar nos 2 primeiros ticks nem tira a Samus do chão.
- Rampas (2.5) e agachar/bola (2.6) mudam a colisão, então as medidas precisam ser
  refeitas depois dessas etapas.


## Auditoria dos relatos de tiro e wall jump (18/10/2026)

- **Segurar tiro:** a referência já arquivada `shoot_hold` confirma disparos no
  quadro 216, novamente no 231 e no 256: primeira espera de 15, depois 25 quadros.
  O save está sem beams/upgrades equipados. Portanto o Power Beam básico repete
  no original; mantido no port. Charge continua para a 4.4, sem inferir seu
  comportamento a partir desses cenários sem equipamento.
- **Força lateral do wall jump:** `wall_open` evita o teto que interrompia a
  subida em `wall_jump`. Após o quadro 339, o coletor reposiciona a Samus já
  girando em (629,720), zera a velocidade vertical e mantém o terreno original.
  São dois quadros apertando para longe e então pulo segurado. O chute leva a
  pose 83 e sobe 100 px nesta amostra; velocidade lateral: 0, depois 0,75 e
  1,375 px/quadro, sem bônus de corrida. O limite sustentado de 1,375 já coincide
  com o port e agora é comparado diretamente com o cache pelos testes.
- Não houve aumento arbitrário do impulso. Isso não prova equivalência completa:
  o port começa a mover um tick antes na saída; à época, sua pose de chute durava 10 ticks,
  enquanto a 83 permanece por toda a janela observada no original. Tempos e
  poses do chute ficam explicitamente na auditoria 3.3, e não foram corrigidos
  nesta entrega. A amostra nova cobre só a parede à esquerda, sem outros itens
  ou bônus de corrida; não generalizar para todas as condições.

**Revisão visual 3.3.2 (22/10/2026).** Reutilizando `wall_open`, sem executar o
Mesen, o port agora mantém WALL_KICK durante o giro no ar. O relógio visual
segue os 80 quadros arquivados: 0/1 por cinco ticks cada e 3–10 em ciclo 3/2.
A caixa de 38 px também permanece durante o chute. Tiro, morph e pouso saem
da pose pelas transições normais. A antecipação do deslocamento na saída e a
caixa transitória de 24 px no primeiro tick original continuam como limites;
não há alegação de equivalência de toda a trajetória. O tempo espelhado para
a esquerda não possui medição independente.

**Saída 3.3.3 (22/10/2026).** A saída agora preserva a posição no primeiro tick,
sem gravidade nesse tick, e arma o impulso para o seguinte. A caixa transitória
é 24 px antes de expandir para 38 px (respeitando obstáculos). As velocidades
laterais dos 80 quadros são comparadas com o cache, nos dois sentidos;
o lado esquerdo é uma aplicação simétrica. Isso resolve os limites de saída
listados na revisão 3.3.2, sem afirmar igualdade de toda a trajetória vertical.
