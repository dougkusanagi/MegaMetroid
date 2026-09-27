<h1 align="center">Mega Metroid</h1>

Powered by  <a href="https://github.com/Stephane-D/SGDK">SGDK</a>

Small project with the intention of porting Super Metroid for Mega Drive / Sega Genesis.

Update: simple tile collision system implemented. 

Controles da build atual (Mega Drive): direcional para andar, **B segurado**
para correr, **C** para pular e **A** para o Power Beam básico. No teclado
do Fusion configurado nesta máquina: A = tecla A, B = tecla S, C = tecla D.
Segure cima ao atirar para mirar acima; cima + esquerda/direita mira na diagonal.
No ar também é possível mirar para baixo. Tocar A dispara; segurar repete.

A caminhada usa 2,75 px/tick. A corrida acrescenta 0,03125 px/tick a cada tick
no chão, até +2 após 64 ticks (4,75 px/tick no total). O salto conserva o bônus;
parar, inverter, bater numa parede ou soltar B no chão reinicia a corrida.
O limite reproduz a corrida NTSC sem Speed Booster, com aceleração alongada para
aproximadamente um segundo; a resposta ao direcional e o salto são do projeto.
Referência: [disassembly de Super Metroid, banco 90](https://github.com/InsaneFirebat/sm_disassembly/blob/main/src/bank_90.asm),
rotina `Handle_Samus_XExtraRunSpeed` e tabela `SamusXSpeedTable_Normal`.

O Power Beam não consome munição, não carrega e mantém até três projéteis.
Há colisão com paredes e rampas, descarte ao sair da tela e som sintetizado.
As fases atuais ainda não têm entidades de inimigos ou portas destrutíveis;
o valor de dano está preparado no projétil para essa integração futura.
O sprite atual da Samus é preservado, sem novas poses de mira.

O salto giratório só começa quando C é pressionado enquanto Samus já estava em
movimento. Direcionar depois de um salto parado move no ar sem iniciar o giro.
Atirar no ar interrompe o giro e mostra a pose correspondente à direção do tiro.

Compilação com SGDK 2.11, Java e Python 3, a partir da raiz do projeto:

```powershell
./scripts/build.ps1 -Sgdk D:/apps/sgdk -Java D:/apps/jre/bin/java.exe
```

A ROM final é `build.bin`, com uma cópia em `build/rom.bin`.
`out/rom.bin` é o binário antigo versionado e não é atualizado pelo script.
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
