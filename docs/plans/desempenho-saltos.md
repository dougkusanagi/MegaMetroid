# Desempenho nos saltos (slowdowns)

## Objetivo

Eliminar os quadros perdidos ao saltar, sem mudar a física, e registrar o que foi medido antes e depois.
Parte da etapa 8.4 de [PLANO.md](../PLANO.md) (defeito B1 em [BUGS.md](../BUGS.md)). Relato original: "slowdowns durante os saltos".

## Como medir

`python scripts/smoke_test.py` após a build `-SmokeTest` (BlastEm via GDB, acelerado, sem áudio, headless).
O smoke imprime `PROFILE_<cenário>_LEFT_MIN` (menor folga em linhas de varredura antes do próximo VBlank, de 224),
`_TIGHT` (quadros com menos de 16 linhas de folga), `_MISSED` (VInts perdidos) e `_DMA` (maior fila em bytes).
Cada cenário roda 300 quadros. Cenários: 0 parado, 1 andando, 2 pulo parado, 3 giro andando, 4 giro atirando,
5 giro sobre a emenda do céu (todos no Landing Site, com Zoomer, Skree e Ripper), 6 pulo parado e 7 giro atirando
na Crateria 2.

## Medidas

Cada célula é `folga mínima / quadros apertados / VInts perdidos` em 300 quadros (folga em linhas de varredura de 224;
apertado = menos de 16 linhas). BlastEm NTSC, acelerado, sem áudio, headless. A build final foi rodada duas vezes, com
resultado idêntico. "Base" é o código anterior com só o perfilador; entrega 1 = `e1de851` (HUD); entrega 2 = `9296490`
(traçado do canhão), a build final.

| Cenário | Base | Entrega 1 (HUD) | Entrega 2 (traçado) = final |
|---|---|---|---|
| 0 parado | 73 / 0 / 0 | 99 / 0 / 0 | 100 / 0 / 0 |
| 1 andando | 30 / 0 / 0 | 56 / 0 / 0 | 56 / 0 / 0 |
| 2 pulo parado | 42 / 0 / 0 | 69 / 0 / 0 | 69 / 0 / 0 |
| 3 giro andando | 21 / 0 / 0 | 47 / 0 / 0 | 47 / 0 / 0 |
| 4 giro atirando | 0 / 65 / **30** | 0 / 21 / **17** | 0 / 7 / **2** |
| 5 giro sobre a emenda do céu | 18 / 0 / 0 | 45 / 0 / 0 | 45 / 0 / 0 |
| 6 pulo parado (Crateria 2) | 74 / 0 / 0 | 101 / 0 / 0 | 101 / 0 / 0 |
| 7 giro atirando (Crateria 2) | 0 / 25 / **16** | 0 / 12 / **9** | 12 / 1 / **0** |

CPU média de 8 quadros (`SYS_getCPULoad`), pior caso dos dois saltos medidos: parado 70% → 60% e giro 76% → 65%, tudo
na entrega 1 (a entrega 2 não muda esses saltos, que não atiram).

**Slowdowns que continuam na build final:** só o cenário 4 (giro atirando no Landing Site): 2 VInts perdidos e 7 quadros
apertados em 300, folga mínima 0. Nenhum outro cenário perde quadro. O valor varia com o layout da ROM (foi 1 numa
build com código de teste a mais). Hardware real não foi medido.

### Custos por trecho (build de diagnóstico, antes das duas entregas)

Contador V do VDP, linhas de varredura por quadro (262 por quadro, ~488 ciclos cada). Microbenchmarks por chamada:
`mm_hud_cells` ~25; `mm_tick` com a Samus parada ~31; `mm_enemy_tick` com 3 inimigos ~36; `mm_rect_blocked` ~1,3 (caixa do
tiro) a mais de 3,7 (caixa da Samus); quadro de disparo de 18 passos ~150, de 30 passos mais de 224. Por trecho do quadro
no giro: `mm_weapon_tick` ~3 sem tiro e ~39 atirando; laço de sprites de portas e inimigos ~28; `SPR_update` ~15.
Depois da entrega 2, o quadro de disparo mede ~35 linhas com 4, 18 ou 30 passos (build de diagnóstico, só nesta análise).

## Passos

1. ✅ Entrega 1: `draw_hud` reconstruía as células todo quadro (~25 linhas). Agora só quando o painel muda.
2. ✅ Entrega 2: o quadro em que o tiro nasce fazia duas multiplicações e duas divisões de 32 bits por passo do traçado
   do canhão (média de 15 passos, até 29), em software no 68000. Agora é incremental e testa o trecho inteiro de uma vez.
   Mesmos pontos de impacto (`cannon_trace_matches_formula`, ~1,1 milhão de comparações; uma mutação foi detectada).
3. ⏳ Próximo gargalo: ver abaixo. Nenhuma mudança antes de medir a hipótese.

## Próximo gargalo (evidências e hipóteses)

**Sobre `mm_weapon_tick`.** Medido: ~3 linhas sem tiro e ~39 atirando; o quadro de disparo custava ~150 e passou a ~35,
**igual com 4, 18 e 30 passos**. Logo o traçado não domina mais. Um tick sem disparo custa ~9 linhas (limpar tiros + tick) no mesmo benchmark; sobram ~25 linhas sem explicação medida. Hipóteses candidatas, nenhuma medida: a caixa do trecho
inteiro (`mm_rect_blocked` numa caixa de até ~50 px, vários tiles e as portas sólidas), a primeira varredura de `advance`
no quadro do disparo, e o `__mulsi3` em `mm_rect_blocked`. Teste: sub-benchmark de cada uma, sem mudar o jogo.

**Sobre os custos fixos.** Pesam em todo quadro, inclusive nos que perdem VInt: `mm_tick` ~31, `mm_enemy_tick` ~36 e o laço de
sprites ~28 linhas. Evidência de que a colisão sozinha não os explica: o contador mediu só ~6,5 chamadas de
`mm_rect_blocked` por quadro parado, e uma chamada com a caixa da Samus custa ≥3,7 linhas, o que dá bem menos que 31.
O código gerado de `mm_tick` tem 4 chamadas a `__divsi3`, 4 a `__mulsi3` e 1 a `__modsi3` (só se vê o código, ainda sem
contar quantas rodam por quadro). **Hipótese mais sustentada:** aritmética de 32 bits da libgcc em `mm_tick` e nos
comportamentos dos inimigos. Teste: contar as chamadas executadas por tick e cronometrar sub-trechos de `mm_tick`.
Também sem medir: por que o laço de sprites custa ~28 linhas com 4 portas e 3 inimigos.

## Tentativas que falharam

Nenhuma até agora.

## Dúvidas abertas

- As medições vêm do BlastEm NTSC. Em hardware real o custo pode diferir (barramento do Z80 com PCM, por exemplo).
- Os outros relatos (HUD diferente da referência, movimento do tiro durante a corrida) seguem sem triagem.

## Resultado (2026-11-05)

B1 resolvido: `advance()` em `weapon.c` testa a caixa do trecho inteiro antes da varredura por pixel. Cenário 4: 0 VInts perdidos, folga 21; cenário 7: folga 43. Hipótese do `__mulsi3` em `mm_rect_blocked` refutada (sem efeito). Custos fixos restantes (`mm_move` ~29, Zoomer ~25, sprites, `SPR_update`) não perdem quadro; medir de novo se aparecer slowdown.
