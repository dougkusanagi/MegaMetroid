# Echo Below — documento de design

Homebrew original para Mega Drive, feito em paralelo ao MegaMetroid e com o mesmo
motor. Personagem, história, arte, música, sons e salas são próprios do projeto
ou vêm de fontes com licença livre. Nada vem do Super Metroid.

## Por que existe

- Dá ao canal um conteúdo que não depende de propriedade de terceiros e que pode
  ser vendido e distribuído.
- Força o motor do MegaMetroid a separar código de conteúdo. As melhorias passam
  a valer para os dois jogos.

## Assets visuais

- Base: [Warped Caves](https://ansimuz.itch.io/warped-caves) (ansimuz), versão
  grátis. A página informa a licença CC0 1.0. Um comentário antigo de usuário
  cita CC-BY 3.0. **Antes de usar, confira o arquivo de licença do download** e
  guarde uma cópia em `res/echo_below/LICENSES/`.
- A versão grátis tem: uma heroína com 9 animações, 3 inimigos animados, um
  tileset de caverna, objetos e um fundo de 2 camadas com parallax. O Expansion
  Pack (US$ 5) e o Player add-on (Patreon) são vendidos à parte.
- O pack é muito usado. Para o jogo ter identidade própria:
  - aplique paletas próprias em cada área;
  - redesenhe a heroína ou modifique-a bastante;
  - crie o chefe e pelo menos dois inimigos.
- Os limites do Mega Drive estão em [Conversão de paleta](#conversão-de-paleta).

## História

**Premissa.** Ina Varre é cartógrafa de cavernas, não soldado. Um terremoto abre
um poço, e ela desce para mapear uma rede de cavernas que, segundo a lenda,
"canta". Lá embaixo, ela descobre que o som vem de uma criatura enorme e
adormecida. As cavernas são o corpo da criatura e os tremores são a respiração.

**Tom.** Solitário e curioso, com pouco texto. Anotações de uma expedição
anterior, espalhadas pelas salas, contam a história. Não há cutscenes longas.

**Diferença de Metroid.** Ina não usa armadura nem arma de fogo. A progressão é
sonora e a meta é entender a criatura, não destruí-la.

## Mecânica central: o diapasão

| Habilidade | Como se obtém | Efeito |
|---|---|---|
| Pulso | Início | Atordoa inimigos e quebra cristais finos. |
| Eco | Galerias de Cristal | Revela passagens escondidas por alguns segundos. |
| Ressonância | Rio Subterrâneo | Faz plataformas de cristal vibrarem e ficarem sólidas. |
| Grito | Antes do chefe | Acorda ou acalma a criatura e define o final. |

Movimento: andar, pular e agarrar bordas. O wall jump só entra se combinar com a
movimentação ajustada. A física deve ter constantes próprias, e não as medidas do
Super Metroid.

## Estrutura (15–25 salas)

1. **O Poço** — tutorial. A corda arrebenta e não há volta.
2. **Galerias de Cristal** — Eco e as primeiras anotações da expedição anterior.
3. **Rio Subterrâneo** — Ressonância e o primeiro "órgão" da criatura.
4. **O Coração** — chefe: o líder da expedição anterior, que quer manter a
   criatura dormindo a qualquer custo.
5. **Final** — acalmar a criatura e sair pela superfície que se abre, ou fugir e
   deixar tudo como estava.

## Música

- Composta para o YM2612 (FM) e o PSG, e não como PCM longo. No MegaMetroid, o
  PCM longo pesa no XGM2.
- Ferramenta: Furnace Tracker (grátis), com exportação VGM para o XGM2 do SGDK.
  O DefleMask é a alternativa.
- Faixas: título, Poço (ambiente), Galerias (arpejos), Rio (grave e pulsado),
  chefe e final.
- Fontes: um compositor de chiptune ou FM (itch.io, r/chiptunes, Discord do SGDK),
  com a licença combinada por escrito. Também servem faixas CC0 ou CC-BY do
  OpenGameArt.

## Efeitos sonoros

São gerados por síntese no próprio projeto (ver o plano), sem amostras de
terceiros.

- PCM (máximo de 3 canais no XGM2): pulso, Eco e a criatura.
- Os demais sons em FM ou PSG: pulo, pouso, passo, acerto, dano, morte, cristal
  quebrando, item e passagem aberta.

## Conversão de paleta

- 4 paletas de 16 cores. O índice 0 é transparente, então sobram 15 cores
  visíveis em cada paleta.
- Cor de 9 bits: 3 bits por canal, ou seja, 8 níveis (0, 36, 73, …, 255 nos PNGs
  do SGDK).
- Tiles de 8×8. Cada tile usa uma só paleta.
- Divisão inicial: PAL0 para o cenário, PAL1 para o fundo, PAL2 para a Ina e
  PAL3 para os inimigos e efeitos.
- Limites de sprites: 80 sprites e 20 por linha. A VRAM tem 64 KB.
