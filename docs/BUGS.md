# Defeitos e pendências conhecidas

Uma linha por defeito. Prioridade: **alta** (trava, perde quadro ou quebra o jogo),
**média** (diferença visível do original), **baixa** (detalhe ou cobertura de teste).
Ao corrigir, remova a linha e registre a correção no `CHANGELOG.md` e no `HISTORICO.md`.
Impressões de playtest do usuário também entram aqui.

| # | Prioridade | Defeito | Etapa | Origem |
|---|---|---|---|---|
| B4 | média | Efeitos que trocarem a PAL2 (trajes, dano, carga) também tingem a barra do HUD | 3.6 | `HISTORICO.md`, 7.1.1 |
| B5 | baixa | Oclusão da Samus nas bordas dos túneis: sem captura de validação em Crateria 2, nas portas verticais e em todas as poses | 6.6 | `HISTORICO.md`, Fase 7 |
| B6 | baixa | Portas do Landing Site sem sala de destino e portas verticais sem ponto de chegada não fazem nada | 6.6 | `HISTORICO.md`, 6.5 |
| B7 | baixa | A Samus perde agachada/bola ao atravessar uma porta (reinicia em pé); conferir contra o original | 6.3 | `HISTORICO.md`, 6.3 |
| B8 | média | No pulo giratório, alternar esquerda/direita troca a animação para o pulo comum; a troca de direção encerra o giro | 3.3 | Relato do usuário (2026-11-05), sem depuração |
| B9 | baixa | Impressão de playtest (2026-11-05): a cor de um inimigo da área inicial parece diferente do original; conferir Zoomer/Geemer/Ripper em jogo contra o original, pois a arte é remapeada para a PAL1 da sala | 5.2 | Relato do usuário (2026-11-05) |
