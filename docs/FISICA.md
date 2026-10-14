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
| `MM_SPEED` | 704 | 2,75 px/tick | Velocidade de caminhada (ver README) |
| `MM_RUN_ACCEL` | 8 | 1/32 px/tick por tick | Corrida (ver README) |
| `MM_RUN_EXTRA_MAX` | 512 | +2 px/tick | Corrida (ver README) |
| `MM_JUMP` | 1248 | 4,875 px/tick | `InitialYSpeeds_Jumping` = `$04E0` → 4,875 px/frame |
| `MM_GRAVITY` | 28 | 0,109375 px/tick² | `YSubAccelerationInAir` = `$1C00` → 0,109375 px/frame² |
| `MM_MAX_FALL` | 1280 | 5 px/tick (para de acelerar ao chegar; fica em 5,03) | Medido: 5,03 px/frame |
| `MM_JUMP_STILL` | 2 | 2 ticks parada na saída do pulo normal; 1 no giro | Medido: pose `4B` e mais 1 quadro sem subir |
| `MM_WALL_JUMP` | 1184 | 4,625 px/tick | Pulo de parede: `InitialYSpeeds_WallJumping` = `$04A0` → 4,625 px/frame |
| `MM_WALL_LOCK` | 12 | 12 ticks | Impulso de parede: afasta da parede a 2,75 px/tick, sem direcional (decisão do projeto) |
| `MM_WALL_REACH` | 8 | 8 px | Distância máxima da parede para o pulo de parede (decisão do projeto) |

O movimento vertical segue o Super Metroid (2.8.1): a Samus anda e depois a gravidade é somada,
o ápice tem um tick com velocidade 0 (o original guarda velocidade e direção separadas), soltar C
na subida zera a velocidade na hora e não existe pulo depois de sair de uma borda (sem coyote time).

Outras velocidades de referência no SM, para etapas futuras: pulo com Hi-Jump
`$0600` (6,0 px/frame), pulo de parede `$04A0` (4,625 px/frame).

## Medidas

Medidas por `physics()` em `tests/core_test.c`: chão plano, hitbox 8x16, pulo a partir
do repouso ou andando, com C segurado.

| Medida | Port | Super Metroid |
|---|---|---|
| Altura do pulo parado | 111 px | 111,09 px |
| Tempo fora do chão | 92 ticks | 92 quadros |
| Alcance do pulo andando | 253 px | ainda diferente (velocidade no ar, 2.8.2) |
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
| Caminhada, aceleração | +0,1875 por quadro; 2,75 no 11º quadro (~10 px) | instantânea | não |
| Soltar o direcional | −0,5 por quadro; desliza ~16 px em 7 quadros | para na hora | não |
| Virar no chão | 6 quadros virando (pose 25/26), depois acelera do zero | instantâneo | não |
| Corrida (B) | +1/16 por quadro, +2 em 32 quadros (4,75 no total) | +1/32, +2 em 64 ticks | não (decisão no README) |
| Pulo, velocidade de saída | 4,875 px/quadro | 4,875 | sim (2.8.1) |
| Gravidade | 0,109375 px/quadro² | 0,109375 | sim (2.8.1) |
| Pulo parado, altura e tempo no ar | 111 px, pico no 46º quadro, 92 quadros no ar | 111 px, 92 ticks | sim (2.8.1) |
| Saída do pulo | 2 quadros sem subir (pose 4B e mais um); no giro, 1 | igual | sim (2.8.1) |
| Queda máxima | 5,03 px/quadro, a partir de ~46 quadros caindo | 5,03 | sim (2.8.1) |
| Soltar o pulo na subida | velocidade vertical vai a 0 na hora | igual | sim (2.8.1) |
| Pulo mais curto | ~5 px (segurando 3 quadros; 1–2 quadros não saem do chão) | igual | sim (2.8.1) |
| Pular logo depois de sair da borda | não pula, nem no 1º quadro | igual (era coyote de 5 ticks) | sim (2.8.1) |
| Velocidade no ar: pulo normal / giro / queda | 1,25 / 1,375 / 1,0 px/quadro (+ bônus da corrida) | 2,75 (+ bônus) em todos | não |
| Pulo correndo, no ar | 3,375 (1,375 + 2) | 4,75 | não |
| Quando gira | só se já andava ao pular; direção + pulo parado é pulo normal (pose 51) | igual | sim |
| Pulo de parede, subida | 4,625 px/quadro | 4,625 | sim |
| Pulo de parede, lado | 1,375 px/quadro, direcional livre | 2,75 travado por 12 ticks | não |
| Pouso | pulo normal: 7 quadros (A4); giro: 1 quadro (A6/A7) | 10 / 15 ticks | não |
| Agachar (↓) | transição de 3 quadros; fica agachada ao soltar, mesmo segurando ↓ por muito tempo | 5 ticks; segurar ↓ mais de 12 ticks levanta ao soltar | não |
| ↓ agachada | vira bola, transição de 6 quadros | igual, 10 ticks | lógica sim, tempo não |
| ↓ no ar | o 1º mira para baixo, o 2º vira bola | dois toques em até 20 ticks | sim |
| ↑ na bola | volta para agachada (6 quadros); outro ↑ levanta (3 quadros) | levanta direto (10 ticks) | não |
| Bola rolando | 3,25 px/quadro, acelera +0,75 por quadro | 2,75 (+ bônus), sem aceleração | não |
| Hitbox (largura × altura) | em pé 10×42, pulo 10×38, giro 10×24, agachada 10×32, bola 10×14 | 24×40, 24×24 agachada, 16×16 bola; giro usa a de pé | não |
| Feixe (Power Beam) | sai a 4 px/quadro e acelera até ~6 em ~28 quadros | 12 px/tick constante | não |
| Intervalo de tiro | 15 quadros apertando; segurado dispara de novo a cada 25 | 15 / 25 | sim |
| Feixes ao mesmo tempo | ao menos 3 (o limite não apareceu nos cenários) | 3 | — |

Os quadros de pose (`4B`, `A4`, …) são os números de pose do Super Metroid (`$0A1C`), os mesmos da
folha de sprites. As velocidades batem com `SamusXSpeedTable_Normal` em `bank_90.asm`: aceleração
`$0030`, máxima 2,75 no chão, 1,25 no pulo normal, 1,375 no giro e no pulo de parede, 1,0 na queda
e 3,25 na bola; desaceleração 0,5.

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

## Hitboxes (2.6)

| Estado | Caixa (px, relativa ao corpo) | Regra |
|---|---|---|
| Em pé | 24 x 40 (`stand_box`) | Padrão |
| Agachada | 24 x 24 (topo desce 2/5 da altura, pés ficam) | No chão, ↓ segurado sem direção; continua agachada enquanto a caixa em pé não cabe |
| Bola | 16 x 16 (2/3 da largura, centrada, apoiada nos pés) | ↓ apertado no ar; ↑ sai só onde a caixa em pé cabe |

- O teste `tunnels()` faz a bola atravessar um túnel de 2 tiles (16 px) sem parar e
  mostra que ↑ não sai dentro dele. A caixa em pé não cabe ali.
- O plano pede um túnel de 1 tile (8 px). Uma bola de 16 px não passa por 8 px, então
  o túnel de teste tem 2 tiles. Se a bola precisar ser menor, esse número muda junto.
- Sprites: agachar usa a pose de pé e a bola usa os quadros de giro, até a fase 3.
- Pular a partir de agachado sob teto baixo não é tratado: o corpo volta à caixa em
  pé já no tick seguinte ao pulo, e a colisão empurra a Samus para baixo do teto.

## Pulo giratório e pulo de parede (2.7)

- **Giratório**: pular enquanto já se anda vira `spin_jumping`. A pose de giro dura o
  voo e para ao tocar o chão ou ao atirar.
- **Parede**: como no Super Metroid, só no pulo giratório (ou logo depois de outro pulo
  de parede, que também gira), encostado numa parede sólida e com o direcional apontando
  para o lado oposto a ela; aí apertar C faz um pulo de parede. "Encostado" aceita a
  parede até `MM_WALL_REACH` = 8 px atrás, porque apertar para longe já afasta a Samus
  2,75 px por tick. O pulo dá `MM_WALL_JUMP` para cima e um impulso de 2,75 px/tick para
  o lado apertado, sem direcional, por `MM_WALL_LOCK` ticks. Usa o estado SPIN. Pulo
  parado, direcional contra a parede ou sem direcional não fazem pulo de parede
  (`wall_jump_rules()` em `tests/core_test.c`).
- O teste `wall_climb()` sobe um poço de 40 px de largura, do chão (y 112) até y 40,
  com 3 impulsos de parede. Ele conta os impulsos e exige pelo menos 2, para que um
  pulo do chão não baste.
- Sem deslizar na parede: a queda continua no ritmo normal enquanto encosta.

## Limites e próximos passos

- A integração segue a ordem do Super Metroid: `y += vy` e depois `vy += g`, com a saída
  parada e o ápice em 0. Assim a sequência de velocidades é a mesma do original, quadro a quadro.
- O pulo completo, medido acima, exige C segurado durante toda a subida. Soltar C antes zera a
  subida na hora; soltar nos 2 primeiros ticks nem tira a Samus do chão.
- Rampas (2.5) e agachar/bola (2.6) mudam a colisão, então as medidas precisam ser
  refeitas depois dessas etapas.
