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
rótulos `SamusPhysicsConstants_*`. Os valores ainda não foram medidos no jogo original.

| Constante | Valor no projeto | Equivale a | Referência SM (NTSC) |
|---|---|---|---|
| `MM_SPEED` | 704 | 2,75 px/tick | Velocidade de caminhada (ver README) |
| `MM_RUN_ACCEL` | 8 | 1/32 px/tick por tick | Corrida (ver README) |
| `MM_RUN_EXTRA_MAX` | 512 | +2 px/tick | Corrida (ver README) |
| `MM_JUMP` | 1690 | 6,60 px/tick no takeoff | `InitialYSpeeds_Jumping` = `$04E0` → 4,875 px/frame |
| `MM_GRAVITY` | 56 | 0,219 px/tick² | `YSubAccelerationInAir` = `$1C00` → 0,109 px/frame² |
| `MM_MAX_FALL` | 1536 | 6,0 px/tick | Acelera de queda até `$0005` → 5 px/frame |

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

Cálculo do SM com os valores da tabela, só como estimativa: altura v²/2g ≈ 108 px,
tempo no ar ≈ 89 ticks. Não foi medido no jogo original. O plano pede essa medida
antes de fixar constantes (seção 4 de [PLANO.md](PLANO.md)).

## Limites e próximos passos

- A integração é de Euler por tick: `vy += g` e depois `y += vy`. Isso explica a
  diferença de altura da tabela acima. O teste fixa 95 a 97 px para pegar regressões.
- Pulo variável (2.3) vai cortar `vy` quando o botão C é solto na subida. A altura
  medida acima é a do pulo completo, sem corte.
- Rampas (2.5) e agachar/bola (2.6) mudam a colisão, então as medidas precisam ser
  refeitas depois dessas etapas.
