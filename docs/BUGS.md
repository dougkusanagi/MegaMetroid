# Defeitos e pendências conhecidas

Uma linha por defeito. Prioridade: **alta** (trava, perde quadro ou quebra o jogo),
**média** (diferença visível do original), **baixa** (detalhe ou cobertura de teste).
Ao corrigir, remova a linha e registre a correção no `CHANGELOG.md` e no `HISTORICO.md`.
Impressões de playtest do usuário também entram aqui.

| # | Prioridade | Defeito | Etapa | Origem |
|---|---|---|---|---|
| B1 | alta | Giro atirando no Landing Site perde 2 VInts em 300 quadros (BlastEm NTSC) | 8.4 | `plans/desempenho-saltos.md` |
| B2 | média | Com o míssil selecionado, a paleta dele sobrescreve os tons da nave nos índices 10–15 da PAL3. Correção prevista: folha do míssil só com os índices 0–9 e os verdes que já coincidem com a nave | 4.3 | `HISTORICO.md`, Fase 7 |
| B3 | média | Geemer reusa a IA de solo do Zoomer e não anda em paredes e teto | 5.2 | `HISTORICO.md`, 1. Estado atual |
| B4 | média | Efeitos que trocarem a PAL2 (trajes, dano, carga) também tingem a barra do HUD | 3.6 | `HISTORICO.md`, 7.1.1 |
| B5 | baixa | Oclusão da Samus nas bordas dos túneis: sem captura de validação em Crateria 2, nas portas verticais e em todas as poses | 6.6 | `HISTORICO.md`, Fase 7 |
| B6 | baixa | Portas do Landing Site sem sala de destino e portas verticais sem ponto de chegada não fazem nada | 6.6 | `HISTORICO.md`, 6.5 |
| B7 | baixa | A Samus perde agachada/bola ao atravessar uma porta (reinicia em pé); conferir contra o original | 6.3 | `HISTORICO.md`, 6.3 |
