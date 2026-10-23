<h1 align="center">Mega Metroid</h1>

Powered by  <a href="https://github.com/Stephane-D/SGDK">SGDK</a>

Build instructions: [`docs/BUILD.md`](docs/BUILD.md).

Os smoke tests mostram o BlastEm no monitor secundário sem retirar o foco.
Use `python scripts/smoke_test.py --hidden` para mantê-lo oculto; sem monitor secundário,
a execução fica oculta automaticamente.

Pipeline de extração e importação de sprites: [`docs/SPRITES.md`](docs/SPRITES.md).

Histórico de correções, melhorias e adições: [`CHANGELOG.md`](CHANGELOG.md).

O Landing Site usa a arte nova de Crateria: paredes e vegetação restauradas,
céu baseado no original, quatro portas azuis fechadas, base de blocos e a nave.
O scroll do fundo foi corrigido para evitar redraw completo e travadas nas emendas.
As portas azuis abrem com um tiro e só deixam passar depois de terminar a abertura; a porta
do outro lado já fica aberta quando Samus chega. A tampa usa os cinco quadros originais da
abertura. Blocos não quebram e a nave ainda
não tem colisão, save ou recarga. O ponto inicial da Samus foi mantido; caminhe
para a direita até o centro da sala para ver a nave. Há adaptação de cores às
quatro paletas do Mega Drive. Regeneração e capturas da ROM em `docs/BUILD.md`.

A porta leste superior do Landing Site leva à Crateria 2. A leste inferior ainda
não tem sala de destino: depois de abri-la e chegar à borda, o jogo mostra
`ROOM NOT BUILT YET`; afastar-se da porta remove o aviso.

A Samus usa animações originais de movimento, pulo, agachar, bola, mira e virada.
A virada tem três quadros, tanto no chão quanto no ar, preservando a física do movimento.

Small project with the intention of porting Super Metroid for Mega Drive / Sega Genesis.

Update: simple tile collision system implemented. 

Controles da build atual (Mega Drive): direcional para andar, **B segurado**
para correr, **C** para pular (soltar C na subida para a subida na hora, como no Super Metroid; não dá para pular depois de sair de uma borda), **C** durante o pulo
giratório, encostado numa parede e com o direcional apontando para longe dela, faz pulo de parede, e **A** para o Power Beam básico. **START** troca a arma: Power Beam,
míssil e super míssil (os compartimentos vazios são pulados). No teclado
do Fusion configurado nesta máquina: A = tecla A, B = tecla S, C = tecla D.
No controle de 6 botões, a mira de L/R do SNES fica em Z (baixo) e Y (cima); X fica livre para
cancelar item. Nas portas conectadas, aproxime-se e use **A** para atirar na porta; atravesse
quando a animação terminar. O jogo mostra esse comando ao chegar perto. Segure Y para mirar na diagonal acima ou Z para a diagonal abaixo, inclusive
parado e agachado; soltar retorna à mira do direcional. Y+Z juntos deixam a mira no direcional. Para jogar com um controle Xbox no layout do SNES, veja o
remap do RetroArch descrito em `docs/BUILD.md`.
Segure cima para mirar acima; cima + esquerda/direita mira na diagonal, parado ou andando, e a pose
de mira aparece mesmo sem atirar. Baixo + esquerda/direita anda mirando na diagonal abaixo. No ar,
baixo mira para baixo. Tocar A dispara; segurar repete, como o Power Beam sem Charge medido no original. O tiro sai da boca do canhão da pose,
usa o sprite original nas oito direções e mostra um impacto animado ao atingir paredes.
↓ no chão agacha, com hitbox e pose menores; a Samus continua agachada ao soltar,
independentemente do tempo segurado. Para levantar, dê um novo toque de ↑, ande ou pule,
desde que haja espaço.
Dois toques de ↓ em até 1/3 de segundo, sem atirar, entram em bola, no chão ou no ar (hitbox de 10×14 px).
Agachada, um novo toque de ↓ também entra em bola, sem pressa. Um toque no ar não faz nada. A bola não
atira e só gira enquanto se move. ↑ ou C desfaz a bola sem dar impulso de pulo;
no chão, volta para agachada se houver espaço. Outro toque de ↑ levanta. Segurar ↑
não conta como um segundo toque; em túneis que só comportam a bola, ela permanece bola.

As hitboxes seguem as dimensões medidas no original: 10×42 px em pé, 10×38 no
pulo normal, 10×24 no giro e 10×32 agachada. A caixa só cresce quando há espaço.

A caminhada acelera 0,1875 px/tick por tick até 2,75 px/tick (11 ticks); soltar o direcional desliza
a 0,5 px/tick por tick e virar leva 7 ticks. A corrida acrescenta 0,0625 px/tick a cada tick
no chão, até +2 após 32 ticks (4,75 px/tick no total). O salto conserva o bônus;
parar, inverter, bater numa parede ou soltar B no chão reinicia a corrida.
No ar o pulo normal anda a 1,25 px/tick e para ao soltar o direcional; o giro anda a 1,375 e mantém a
inércia; a queda de uma borda anda a 1 px/tick; a bola rola a 3,25. Os valores foram medidos no Super
Metroid original no Mesen 2 (`docs/FISICA.md`).
Referência: [disassembly de Super Metroid, banco 90](https://github.com/InsaneFirebat/sm_disassembly/blob/main/src/bank_90.asm),
rotina `Handle_Samus_XExtraRunSpeed` e tabela `SamusXSpeedTable_Normal`.

O Power Beam não consome munição, não carrega e mantém até três projéteis.
Sua velocidade base é 4 px/tick na horizontal/vertical e cerca de 2,668 por eixo
diagonal, acelerando 1/16 por eixo ativo a cada tick, como nas tabelas originais.
Há colisão com paredes e rampas, descarte ao sair da tela e o som de tiro original.
Mísseis e super mísseis entram como armas selecionáveis por START, com a arte original de 16×16
nas oito direções e paleta própria (a PAL3 do beam é trocada enquanto a arma está ativa). A ROM
começa com 5 mísseis e nenhum super, provisoriamente até os itens coletáveis; o contador na HUD
entra na 7.1. Um míssil abre porta azul, cinco (ou um super) abrem vermelha, e verde só aceita super.
A troca de arma espera o projétil e o impacto saírem da tela para não misturar as paletas, e o
impacto do míssil ainda usa o quadro do beam, sem arte própria nesta etapa.
Os efeitos sonoros do Super Metroid ficam em `res/sound/Sound-Effects/`; o tiro, o giro (em loop
enquanto a Samus gira) e o pouso já tocam. A música do Landing Site ("Arrival on Crateria") toca como
PCM, convertida dos MP3 de `res/sound/musics/` por `scripts/convert_music.py` (ver a 8.1 em `docs/PLANO.md`).
As fases atuais ainda não têm entidades de inimigos nem blocos destrutíveis; as portas azuis
já abrem com o tiro. O valor de dano está preparado no projétil para a integração futura.
O sprite da Samus usa as poses originais do jogo (parada, andando, pulando, girando, agachando, em bola, mirando e atirando, cada uma com desenho próprio para a esquerda). Ver a etapa 3.2 em `docs/PLANO.md`.

O salto giratório só começa quando C é pressionado enquanto Samus já estava em
movimento (andando ou deslizando). Direcionar depois de um salto parado move no ar sem iniciar o giro.
Atirar no ar interrompe o giro e mostra a pose correspondente à direção do tiro.

Compilação com SGDK 2.11, Java e Python 3, a partir da raiz do projeto:

```powershell
./scripts/build.ps1 -Sgdk D:/apps/sgdk -Java D:/apps/jre/bin/java.exe
```

A ROM final é `build.bin`, com uma cópia em `out/rom.bin`. Os intermediários ficam em `out/`,
a pasta padrão do SGDK (a mesma do `make -f %GDK%\makefile.gen`, ver `docs/BUILD.md`), que não é versionada.
Para ver posição, velocidade, chão e tiles sob os pés na tela, gere a variante de
depuração. Ela vai para `tests/out/variant.bin` e não substitui `build.bin`:

```powershell
./scripts/build.ps1 -Sgdk D:/apps/sgdk -Java D:/apps/jre/bin/java.exe -Define '-DMM_DEBUG_OVERLAY=1'
```

O núcleo em `src/core` não depende do SGDK; `src/game.c` integra entrada,
renderização e áudio. Os projéteis usam um pool fixo, sem alocar durante o jogo.

<h3>✔️ To do</h3>

Next things i plan to make. <br><br>

- [x] Crateria first scenario
- [x] Sprite Animations
- [x] Platform Collisions
- [ ] Slopes Collisions

<h3>☕ Seja um dos contribuidores</h3>

Feel free to fork and contribute or just using for study this lil snippet code.
