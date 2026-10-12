# Arte do cenário: do SNES para o Mega Drive

## Fluxo

`res/images/maps/crateria/original-whithout-sprites.png` (sala 1) e `tilemap2.png` / `bg-2.bmp` (sala 2) guardam as cores **originais do SNES**. O script gera os PNGs usados no jogo, sem alterar nenhum pixel:

```
python tools/make_md_assets.py
```

| Saída (usada em `res/*.res`) | Fonte |
|---|---|
| `crateria1-fg-md.png` | `original-whithout-sprites.png` (recorte de 1248 linhas) |
| `crateria2-fg-md.png` | `tilemap2.png` |
| `crateria2-bg-md.png` | `bg-2.bmp` |

Os PNGs gerados são indexados de 4 bits, índice 0 = transparente. Dá para abrir no Aseprite e ajustar a paleta direto; ao mexer, edite também as tabelas `FG` / `BG2` em `tools/make_md_assets.py`, senão a próxima execução do script sobrescreve o ajuste.

## Por que as cores mudaram

As cores de Crateria são muito escuras (ex.: rocha `8,32,24`). O MD só tem 8 níveis por canal, e os medidos (`Genesis-palette-512.gpl`) são 0, 52, 87, 116, 144, 172, 206 e 255. Convertendo direto, as cores escuras colapsam umas nas outras e o dither some. O mapeamento é cor a cor, escolhido para manter:

- a **ordem de luminosidade** (rocha < "+" índigo < borda lilás < metal);
- o **matiz** de cada família (rocha verde-azulada, "+" índigo, borda rosada, grama oliva, contornos neutros);
- o **contraste entre cores vizinhas** (dither da rocha, "+" e bordas), que é o que dá a textura.

A rocha ficou mais clara que no original (corpo `0,52,52` contra `8,32,24`): é o nível mais baixo que o MD permite sem virar preto. As cavernas ficaram quase pretas, como no original.

`tools/genesis_adapt.py` é o otimizador que ajudou a explorar candidatos (recozimento simulado sobre a adjacência real dos pixels). A paleta final foi ajustada à mão e conferida no BlastEm.

## Pendências

- `bg1.png` (céu/montanhas da sala 1) ainda está com a paleta antiga, bege/oliva, e destoa do resto. Precisa do sheet original de fundo para refazer.
- Referências originais: baixar os sheets de https://www.spriters-resource.com/snes/smetroid/ (o site exige captcha, não dá para baixar por script) e guardar em `res/reference/original/`.
