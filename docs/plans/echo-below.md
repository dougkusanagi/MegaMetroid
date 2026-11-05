# Plano — Echo Below

Objetivo: um homebrew original e curto, com o motor do MegaMetroid, a arte do
Warped Caves convertida para o Mega Drive e sons próprios. O design está em
[`docs/echo-below/design.md`](../echo-below/design.md).

## Decisões em aberto

- **Repositório:** pasta `games/echo_below/` neste repo ou repo próprio. A
  recomendação é um repo próprio quando o motor estiver separado (passo 1), para
  que o código do jogo original não fique junto de recursos do Super Metroid.
- **Licença do pack:** confirmar o arquivo do download (CC0 × CC-BY 3.0).
- **Personagem:** usar a do pack com mudanças ou desenhar uma nova.

## Passos

### 0. Assets e licença
- [ ] Baixar a versão grátis do Warped Caves em `assets_src/warped_caves/`, fora
      do build.
- [ ] Guardar o arquivo de licença e anotar a origem em `LICENSES/`.
- [ ] Inventário: tamanho dos tiles, número de cores, quadros por animação.

### 1. Separar motor e conteúdo
- [ ] Mapear em `src/` e `inc/` o que é motor (física, salas, portas, câmera, pool
      de inimigos, HUD, áudio) e o que é conteúdo do MegaMetroid (constantes
      medidas, salas, sprites, sons).
- [ ] Mover as constantes e as tabelas de conteúdo para arquivos por jogo, sem
      mudar o comportamento do MegaMetroid. Validação: a mesma suíte e o smoke no
      BlastEm, com ROM equivalente.
- [ ] Escolher a forma de build: um define de jogo em `scripts/build.ps1` ou um
      repo próprio que usa o motor.

### 2. Conversão de paleta (`scripts/eb_convert.py`)
- [ ] Quantizar cada imagem para cores de 9 bits (8 níveis por canal).
- [ ] Reduzir cada grupo (cenário, fundo, personagem, inimigos) para 15 cores mais
      a transparente, com uma paleta fixa por grupo que cobre todas as imagens
      dele.
- [ ] Cortar em tiles de 8×8 e verificar que cada tile cabe em uma paleta.
- [ ] Aplicar paletas próprias por área (Poço, Galerias, Rio, Coração).
- [ ] Testes em `tests/`: cores válidas, no máximo 16 por paleta, tamanho múltiplo
      de 8 e contagem de tiles dentro do orçamento de VRAM.
- [ ] Gerar uma prancha PNG de antes e depois para revisão visual.

### 3. Efeitos sonoros (`scripts/eb_sfx.py`)
- [ ] Síntese com numpy (ondas quadradas, ruído, envelopes, varredura de tom),
      exportada como WAV de 8 bits mono na taxa aceita pelo XGM2.
- [ ] Sons: pulso, Eco, criatura (PCM), pulo, pouso, passo, acerto, dano, morte,
      cristal, item e passagem.
- [ ] Parâmetros com semente fixa, para gerar os mesmos arquivos sempre.
- [ ] Teste de tamanho e duração de cada som contra o orçamento de ROM.

### 4. Protótipo jogável
- [ ] Ina com movimento próprio (constantes novas, não as do Super Metroid).
- [ ] Uma sala do Poço com o tileset convertido e o parallax.
- [ ] O Pulso: atordoar um inimigo e quebrar um cristal.
- [ ] Smoke no BlastEm via GDB (acelerado, sem áudio, headless) e medição de
      quadros perdidos.

### 5. Conteúdo
- [ ] Mapas das 4 áreas no Tiled, com o exportador existente.
- [ ] Eco, Ressonância e Grito.
- [ ] Inimigos do pack e dois inimigos próprios.
- [ ] Chefe e os dois finais.
- [ ] Música FM (Furnace → VGM → XGM2).

### 6. Lançamento
- [ ] Tela de título e créditos com as licenças.
- [ ] Página no itch.io e vídeos de divulgação (ver `docs/divulgacao/`).

## Próximo passo

Passo 0: baixar o pack e conferir a licença. O download do itch.io exige uma ação
no navegador (preço livre); depois disso, os passos 1 a 3 podem ser feitos aqui.
