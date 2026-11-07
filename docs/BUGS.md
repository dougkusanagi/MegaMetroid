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
| B9 | baixa | Impressão de playtest (2026-11-05): a cor de um inimigo da área inicial parece diferente do original; conferir Zoomer/Geemer/Ripper em jogo contra o original, pois a arte é remapeada para a PAL1 da sala | 5.2 | Relato do usuário (2026-11-05) |
| B10 | média | Crateria 2 é só uma substituta da `93AA`: tem 5×5 telas e arte de outra região, enquanto a `93AA` real do grafo tem 2×1 telas (512×256 px), uma porta à esquerda e, no mapa salvo, provavelmente fica em `(6912, 256)` px (sala dos canos). Falta recortar e substituir | A | Investigação de 2026-11-06 |
| B11 | média | O fundo das salas 2, 3 e 4 (`bg-2`, padrão verde escuro, só recolorido para o PAL1 do Landing Site) não foi conferido com o original: o relato do usuário (2026-11-06) diz que não se parece em nada. Falta uma referência do fundo real da `92FD`; o céu do Landing Site não cabe junto da arte de 868 tiles (126 + 868 > 968). Desde 2026-11-06 o verde do fundo vira cinza-escuro só para contrastar com a rocha | A | Relato do usuário (2026-11-06) |

