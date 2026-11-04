# Desempenho nos saltos (slowdowns)

## Objetivo

Eliminar os quadros perdidos ao saltar, sem mudar a física, e registrar o que foi medido antes e depois.
Parte da etapa 8.4 de [PLANO.md](../PLANO.md). Relato original: "slowdowns durante os saltos".

## Como medir

`python scripts/smoke_test.py` após a build `-SmokeTest` (BlastEm via GDB, acelerado, sem áudio, headless).
O smoke imprime `PROFILE_<cenário>_LEFT_MIN` (menor folga em linhas de varredura antes do próximo VBlank, de 224),
`_TIGHT` (quadros com menos de 16 linhas de folga), `_MISSED` (VInts perdidos) e `_DMA` (maior fila em bytes).
Cada cenário roda 300 quadros. Cenários: 0 parado, 1 andando, 2 pulo parado, 3 giro andando, 4 giro atirando,
5 giro sobre a emenda do céu (todos no Landing Site, com Zoomer, Skree e Ripper), 6 pulo parado e 7 giro atirando
na Crateria 2.

## Medidas

Um quadro NTSC tem 262 linhas (~488 ciclos cada). Medido com o contador V do VDP, trecho a trecho, numa build de
diagnóstico (não versionada): `mm_hud_cells` ~25 linhas por chamada; `mm_tick` com a Samus parada ~31; `mm_enemy_tick`
com 3 inimigos ~36; `mm_rect_blocked` ~1,3 (caixa do tiro) a mais de 3,7 (caixa da Samus); `mm_weapon_tick` ~3 sem tiro
e ~39 com tiros no ar; laço de sprites de portas e inimigos ~28; `SPR_update` ~15.

| Cenário | Antes (perdidos / folga mín.) | Após o passo 1 |
|---|---|---|
| 0 parado | 0 / 73 | 0 / 99 |
| 3 giro andando | 0 / 21 | 0 / 47 |
| 4 giro atirando | 30 / 0 | 17 / 0 |
| 7 giro atirando (Crateria 2) | 16 / 0 | 9 / 0 |

CPU média de 8 quadros (`SYS_getCPULoad`): parado 70% → 60%; giro 76% → 65%.

## Passos

1. ✅ `draw_hud` reconstruía as células todo quadro. Agora só quando o estado do painel muda.
2. ⏳ Saltar atirando ainda perde quadros. O maior custo é `mm_weapon_tick` com tiros no ar (varredura pixel a pixel,
   uma `mm_rect_blocked` por pixel, com o laço das portas sólidas a cada chamada). Próximo passo: medir por que o
   custo por chamada é tão alto e reduzi-lo no código compartilhado.
3. ⏳ Outros custos fixos medidos: `mm_tick` e `mm_enemy_tick` parados (~31 e ~36 linhas), laço de sprites de
   portas e inimigos (~28), inimigos fora da tela que ainda simulam.

## Tentativas que falharam

Nenhuma até agora.

## Dúvidas abertas

- As medições vêm do BlastEm NTSC. Em hardware real o custo pode diferir (barramento do Z80 com PCM, por exemplo).
- Os outros relatos (HUD diferente da referência, movimento do tiro durante a corrida) seguem sem triagem.
