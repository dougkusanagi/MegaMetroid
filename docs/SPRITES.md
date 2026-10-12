# Pipeline de sprites

## Exportar a prancha de referência local

Com Python 3 e Pillow instalados (`python -m pip install "Pillow>=9.1"`), execute na raiz do projeto:

```powershell
python scripts/extract_original_samus_sprites.py
```

Por padrão, o script lê `res/images/original-references/SNES - Super Metroid - Playable Characters - Samus Aran.png` e grava o resultado em `res/images/samus-original-sgdk/`. Ambos os diretórios são ignorados pelo Git porque contêm arte extraída do jogo original.

Use `--input` e `--output` para apontar para outros caminhos. A pasta de saída deve ser dedicada: ao regenerar, o script substitui o manifesto, o `.res` e os PNGs com os prefixos `samus_ref_` e `samus_loose_`.

O script recorta os quadros nas caixas lilás de 80×80 pixels e também separa arte avulsa fora das caixas. Remove os fundos conhecidos, corta as margens transparentes e arredonda as dimensões para tiles de 8×8. Cada recorte vira um PNG indexado independente, com o índice 0 transparente e no máximo 15 cores visíveis. Recortes que excedem esse limite são quantizados sem dithering.

Na prancha atual, o resultado contém 877 quadros em caixas e 106 figuras avulsas com pelo menos 10×20 pixels e 60 pixels visíveis. Texto, paletas e marcas pequenas são descartados. Sete recortes são quantizados.

Arquivos gerados:

- `frames/`: PNGs prontos para o `rescomp` do SGDK.
- `sprites.res`: declaração `SPRITE` para cada recorte.
- `manifest.csv`: tipo de recorte (`boxed` ou `unboxed`), coordenadas na prancha, dimensões do recorte em pixels e tiles, e indicação de quantização.

Para compilar o pacote isoladamente, entre na pasta gerada e execute:

```powershell
Push-Location res/images/samus-original-sgdk
java -jar "$env:GDK\bin\rescomp.jar" sprites.res sprites.s
Pop-Location
```

O pacote não é incluído no build do jogo para evitar adicionar centenas de sprites sem uso à ROM. Para usar um quadro, copie sua declaração de `sprites.res` para um `.res` do projeto e ajuste o caminho do PNG em relação à pasta `res/`. Por exemplo:

```text
SPRITE samus_ref_0001 "images/samus-original-sgdk/frames/samus_ref_0001.png" 3 6 FAST 1 NONE
```

Use `manifest.csv` para consultar o tamanho em tiles e a posição original de cada recorte. A integração com animações e o posicionamento comum entre poses ainda precisam ser validados no jogo.
