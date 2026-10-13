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
| `MM_JUMP` | 1690 | 6,60 px/tick no takeoff | `InitialYSpeeds_Jumping` = `$04E0` → 4,875 px/frame |
| `MM_GRAVITY` | 56 | 0,219 px/tick² | `YSubAccelerationInAir` = `$1C00` → 0,109 px/frame² |
| `MM_MAX_FALL` | 1536 | 6,0 px/tick | Acelera de queda até `$0005` → 5 px/frame |
| `MM_JUMP_CUT` | 768 | 3,0 px/tick | Soltar C na subida corta a velocidade até este valor (decisão do projeto) |
| `MM_COYOTE_TICKS` | 6 | 5 ticks após sair do chão | Janela para pular depois de cair de uma borda (decisão do projeto) |
| `MM_WALL_JUMP` | 1184 | 4,625 px/tick | Pulo de parede: `InitialYSpeeds_WallJumping` = `$04A0` → 4,625 px/frame |
| `MM_WALL_LOCK` | 12 | 12 ticks | Impulso de parede: afasta da parede a 2,75 px/tick, sem direcional (decisão do projeto) |
| `MM_WALL_REACH` | 8 | 8 px | Distância máxima da parede para o pulo de parede (decisão do projeto) |

Diferenças em relação ao SM: a gravidade é o dobro, o pulo é 35% mais rápido e a
queda vai até 6 px/tick (SM para perto de 5). O projeto mantém esses valores por
decisão própria, registrada no README. Trocá-los para os da tabela SM é uma decisão
de jogabilidade, não de implementação.

Outras velocidades de referência no SM, para etapas futuras: pulo com Hi-Jump
`$0600` (6,0 px/frame), pulo de parede `$04A0` (4,625 px/frame).

## Medidas

Medidas por `physics()` em `tests/core_test.c`: chão plano, hitbox 8x16, pulo a partir
do repouso ou em corrida com ↓ parado.

| Medida | Resultado | Teórico (tempo contínuo) | Motivo da diferença |
|---|---|---|---|
| Altura do pulo parado | 96 px | 99,6 px = v²/2g | Gravidade é aplicada antes do movimento a cada tick; a soma discreta fica em 96,3 px |
| Tempo no ar | 60 ticks (1,0 s) | 60,3 ticks = 2v/g | Idem, arredondado a ticks |
| Alcance de pulo corrido | 165 px | 165,8 px = 2,75 × 60,3 | Mesma aproximação discreta |
| Velocidade de queda | 6 px/tick (teto) | — | O teto `MM_MAX_FALL` é alcançado antes do pouso |
| Pulo curto (C solto após 1 tick) | 25 px | — | A subida cai para 3 px/tick ao soltar C |
| Janela de coyote | 5 ticks | — | Pulo começa no 5º tick depois de sair do chão, não no 6º |

Os testes `physics()`, `variable_jump()` e `coyote_jump()` fixam esses valores em
`tests/core_test.c`.

Cálculo do SM com os valores da tabela, só como estimativa: altura v²/2g ≈ 108 px,
tempo no ar ≈ 89 ticks. A medida no jogo original está na seção seguinte.

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
| Pulo, velocidade de saída | 4,875 px/quadro | 6,6 | não |
| Gravidade | 0,109375 px/quadro² | 0,219 | não |
| Pulo parado, altura e tempo no ar | 111 px, pico no 46º quadro, 92 quadros no ar | 96 px, 60 ticks | não |
| Primeiro quadro do pulo | parado (pose 4B), sobe no quadro seguinte | já sobe | não |
| Queda máxima | 5,03 px/quadro, a partir de ~46 quadros caindo | 6,0 | não |
| Soltar o pulo na subida | velocidade vertical vai a 0 na hora | limita a 3 px/tick | não |
| Pulo mais curto | ~5 px (segurando 3 quadros; 1–2 quadros não saem do chão) | 25 px (1 tick) | não |
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

- A integração é de Euler por tick: `vy += g` e depois `y += vy`. Isso explica a
  diferença de altura da tabela acima. O teste fixa 95 a 97 px para pegar regressões.
- O pulo completo, medido acima, exige C segurado durante toda a subida. Soltar C
  antes corta a subida em `MM_JUMP_CUT` (2.3). Um toque de 1 tick sobe 25 px.
- Rampas (2.5) e agachar/bola (2.6) mudam a colisão, então as medidas precisam ser
  refeitas depois dessas etapas.
