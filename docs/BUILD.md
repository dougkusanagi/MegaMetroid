# Build do MegaMetroid

Como compilar a ROM (`out/rom.bin`) a partir de um clone limpo.

## Requisitos

- **SGDK** instalado e apontado pela variável de ambiente `GDK` (raiz do SGDK, a pasta que contém `bin/`, `inc/`, `src/`, `makefile.gen`).
  - Testado com SGDK 2.11 (abril de 2025).
- **Java** no `PATH` para o `rescomp.jar` (compilador de recursos do SGDK). Testado com OpenJDK 17. Neste ambiente o JRE está em `D:\apps\jre` (fora da pasta do SGDK), por isso é preciso adicioná-lo ao `PATH`.
- O `make.exe` e o `gcc` vêm com o SGDK (`%GDK%\bin`). Não precisa de MSYS/MinGW à parte.

## Ambiente (PowerShell)

```powershell
$env:GDK = 'D:\apps\sgdk'                 # ajuste para o seu SGDK
$env:PATH = "D:\apps\jre\bin;" + $env:PATH  # Java para o rescomp
```

Para deixar permanente, defina `GDK` nas variáveis de ambiente do usuário e adicione o Java ao `PATH`.

## Comando de build

A partir da raiz do projeto:

```powershell
& "$env:GDK\bin\make.exe" -f "$env:GDK\makefile.gen"
```

Em `cmd`, equivalente a `make -f %GDK%\makefile.gen`.

Saída esperada: `out/rom.bin` (≈ 256 KB). Os `.o` intermediários ficam em `out/`.

Para forçar uma build limpa, apague `out/` antes de rodar. O makefile do SGDK não faz isso sozinho.

## Arquivos gerados pelo makefile

- `src/boot/sega.s` e `src/boot/rom_head.c` são copiados do SGDK na primeira build. Estão no `.gitignore` e não devem ser editados aqui: a versão do projeto deve ser a do SGDK instalado.
- `res/*.rs` e `res/*.d` são intermediários do `rescomp` e são removidos ao fim da build.

## Emulador

Teste das ROMs em:

- **BlastEm** (preciso, recomendado para validar comportamento).
- **Gens KMod** (permite depuração com GDB na porta `6868`, ver `.vscode/launch.json`).

Neste ambiente o BlastEm 1.0.0 está em `D:\emuladores\blastem\blastem-win64-1.0.0\blastem.exe`. Abra com `blastem.exe out\rom.bin`. O FPS aparece no título da janela.

## Teste automatizado no BlastEm, visível no monitor secundário

```powershell
./scripts/build.ps1 -Sgdk D:/apps/sgdk -Java D:/apps/jre/bin/java.exe -SmokeTest
python scripts/smoke_test.py
```

O script inicia o BlastEm com `-D` e conecta ao protocolo GDB em
`localhost:1234`. Aguarda o fim da sequência roteirizada e lê `mm_smoke_result`
diretamente da RAM (1 = passou, 2 = falhou). O log fica em
`tests/out/smoke-gdb.log`; o processo iniciado é encerrado mesmo se o teste falhar.
No Windows, a janela aparece centralizada na área útil do primeiro monitor secundário,
sem ativação, sem subir na ordem das janelas e sem retirar o foco do aplicativo em uso.
A janela nasce oculta e só é mostrada depois do posicionamento. Sem monitor secundário,
ou se não for possível posicioná-la, permanece oculta. `python scripts/smoke_test.py --hidden`
força o teste oculto. Apenas a janela pertencente ao processo do teste é movida.
Não usa cliques, teclado ou captura da área de trabalho. Se a porta já estiver
ocupada, recusa executar para não interferir em outra sessão. Ajuste os caminhos
com `--blastem` e `--gdb`; `--timeout` limita a espera (padrão: 120 segundos; o
smoke completo leva cerca de 50).
O `out/symbol.txt` deve pertencer à mesma build `-SmokeTest`. Para uma ROM em
outro diretório, passe os caminhos correspondentes com `--rom` e `--symbols`.
Ao fim, o smoke imprime também as métricas de orçamento (`VRAM_TILES_*`, `HEAP_*`;
ver 6.4 em `docs/PLANO.md`) e falha se uma sala não couber na VRAM ou a troca vazar heap.
O log também imprime `MM_SMOKE_FAILURE`, uma máscara dos grupos que falharam
(zero quando todos passaram). Os bits são definidos junto às verificações em
`tests/rom_smoke.c`, para localizar falhas pela API sem abrir a interface.

## Medidas do Super Metroid original (Mesen 2)

**Antes de executar o emulador**, consulte `tests/reference/super_metroid/`:
as capturas existentes estão versionadas em CSV comprimido sem perdas, com
manifesto e checksums. `python scripts/sm_reference_cache.py check` valida os
dados sem iniciar o Mesen. Use o original apenas para lacunas, contradições ou
mudanças na referência; consulte o README dessa pasta para importar medições.

`scripts/sm_reference.ps1` roda o Super Metroid no Mesen 2 sem janela (`--testrunner`), com
`scripts/sm_reference.lua`. Cada cenário carrega o save state, aperta uma sequência fixa de botões e
grava posição, velocidade, pose e hitbox da Samus a cada quadro em `tests/out/sm_reference.csv`.
`python scripts/sm_reference_report.py` resume o CSV; os números estão em `docs/FISICA.md`.

```powershell
./scripts/sm_reference.ps1                      # todos os cenários (~1,5 min)
./scripts/sm_reference.ps1 -Scenario jump_stand # só um
python scripts/sm_reference_report.py
```

- Padrões: Mesen 2.1.1 em `D:\emuladores\Mesen_2.1.1_Windows\Mesen.exe`, ROM em
  `D:\roms\snes\Super Metroid (Japan, USA) (En,Ja).zip` e save state do slot 1
  (`Documentos\Mesen2\SaveStates\Super Metroid (Japan, USA) (En,Ja)_1.mss`). Troque com `-Mesen`, `-Rom` e `-State`.
- O save state é a Samus em pé sobre a nave no Landing Site, com os controles padrão (X tiro, A pulo,
  B corrida). Os cenários dependem desse ponto de partida.
- O `--testrunner` não abre ROM dentro de `.zip`: o script extrai para uma pasta temporária.
- O script Lua precisa de `io`/`os` (ler o state e gravar o CSV). O `.ps1` liga a opção só nesta
  execução, por linha de comando, sem mudar as configurações do Mesen.

## Controle Xbox no layout do SNES (RetroArch, core BlastEm)

O core BlastEm do RetroArch liga o RetroPad ao Mega Drive de outro jeito que o jogo espera. Para ter correr = Xbox A (baixo), pular = Xbox B (direita) e atirar = Xbox Y (cima), crie `config\remaps\BlastEm\BlastEm.rmp` na pasta do RetroArch com:

```
input_player1_btn_b = "8"
input_player1_btn_a = "11"
input_player1_btn_x = "0"
input_player1_btn_y = "1"
input_player1_btn_l = "10"
input_player1_btn_r = "9"
```

L e R chegam ao jogo como Z e Y do controle de 6 botões (ainda sem uso).

## Problemas conhecidos

- Build atual com avisos de `const` em `src/map_crateria_1.c` e `src/map_crateria_2.c`. Serão tratados na etapa 0.3.
- Se uma build antiga deixou `out/` com arquivos `.d` que apontam para outro caminho (ex.: `D:/MegaDrive/Projects/MegaMetroid`), a build falha com `No rule to make target ...`. Solução: apague `out/` e compile de novo.


## Salas: mapas Tiled e exportador (etapa 6.2)

As salas não são mais editadas em C. Cada `rooms/<id>.tmj` é um mapa do
[Tiled](https://www.mapeditor.org/) (JSON, tiles 8×8) com a camada de colisão
(`collision.tsj`: sólido, aberto e duas rampas), o ponto de partida (`spawn`),
zonas de câmera (`camera`), portas (`doors`) e inimigos (`enemies`); as
propriedades do mapa dão as imagens de cenário e a rolagem do fundo. A ordem em
`rooms/index.json` define os números `ROOM_*`. O cabeçalho de `scripts/export_rooms.py`
descreve cada camada e propriedade.

```powershell
python scripts/export_rooms.py           # rooms/*.tmj -> src/map_*.c, inc/map*.h, res/*.res, src/map.c
python scripts/export_rooms.py --check   # falha se algum gerado estiver desatualizado
```

Os arquivos gerados têm aviso de "Generated" no topo e ficam no Git; não os edite
à mão. No Tiled, mantenha o formato da camada de tiles em CSV. O exportador
recusa mapa com célula de colisão vazia, imagem de tamanho diferente do mapa,
spawn/porta/inimigo/zona fora dos limites, porta para sala inexistente ou sala
fora do índice. `tests/check_rooms.py` (parte de `scripts/test.ps1`) cobre isso.
Para criar uma sala nova: copie um `.tmj`, ajuste e acrescente o id em `index.json`.
O `-SmokeTest` compila automaticamente todo `src/map*.c`.

## Arte e verificação visual do Landing Site

Os PNGs e dados de referência necessários estão versionados. Para regenerar
com Pillow e numpy, sem abrir emulador:

```powershell
python scripts/build_landing_site.py
./scripts/build.ps1 -Sgdk D:/apps/sgdk -Java D:/apps/jre/bin/java.exe -SmokeTest
python scripts/smoke_test.py --capture-scenes
```

`--capture-scenes` salva o readback da VRAM/CRAM e reconstrói a nave, as quatro
portas e a base de blocos em `tests/out/blastem-*.png`. Os pixels dos planos,
paletas e scroll são conferidos automaticamente. Não captura a área de trabalho.
Pode combinar com `--hidden`. As leituras reservam um buffer de 8 KB na ROM de
smoke, sem afetar a ROM padrão.

Para conferir separadamente o carregamento da segunda sala, gere uma variante
com diretório de saída próprio e use a tabela de símbolos correspondente:

```powershell
./scripts/build.ps1 -Sgdk D:/apps/sgdk -Java D:/apps/jre/bin/java.exe -SmokeTest -Define '-DMM_SMOKE_ROOM=1' -OutDir out/room2-smoke
python scripts/smoke_test.py --rom tests/out/smoke.bin --symbols out/room2-smoke/symbol.txt --timeout 60
```

Somente se faltar referência visual, o coletor de cena roda uma vez no Mesen:

```powershell
./scripts/sm_reference.ps1 -Script "$PWD/scripts/sm_scene_reference.lua" -Out tests/out/scene-state.txt
python scripts/sm_reference_cache.py import-scene tests/out/scene-state.txt
```

O cache inclui screenshot original, memória de vídeo, CGRAM, OAM, WRAM e estado
PPU comprimidos, com checksums, origem e limites em seu manifesto. Não distribui
ROM nem save state. Não repetir a captura para ajustes comuns no cenário.


O smoke também imprime `NORMAL_CPU/DMA/MISSED` e `SPIN_CPU/DMA/MISSED` para dois
saltos atravessando a emenda do fundo. CPU é o maior valor da média de oito
quadros do SGDK; DMA é a maior fila em bytes; MISSED conta VInts além de um por
frame. A fase faz 40 quadros de aquecimento e mede 110 quadros por salto.

Para preencher uma lacuna de referência de céu, o coletor específico é
`scripts/sm_sky_reference.lua`, usando o mesmo wrapper headless. Os probes já
estão arquivados; não executar novamente para ajustar o port. A opção
`sm_reference_cache.py import-sky prefixo1 prefixo2` preserva CSV, screenshots,
VRAM/CGRAM e hashes da ROM/state/script de cada captura. O cenário `wall_open`
no coletor de física documenta seu reposicionamento, para não confundir uma
amostra controlada com uma travessia normal da sala.
