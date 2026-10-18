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
com `--blastem` e `--gdb`; `--timeout` limita a espera (padrão: 45 segundos).
O `out/symbol.txt` deve pertencer à mesma build `-SmokeTest`.
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
