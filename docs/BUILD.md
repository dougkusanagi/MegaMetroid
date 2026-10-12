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

Neste ambiente o BlastEm 1.0.0 está em `C:\Users\dl_ag\Tools\blastem\blastem-win64-1.0.0\blastem.exe`. Abra com `blastem.exe out\rom.bin`. O FPS aparece no título da janela.

## Problemas conhecidos

- Build atual com avisos de `const` em `src/map_crateria_1.c` e `src/map_crateria_2.c`. Serão tratados na etapa 0.3.
- Se uma build antiga deixou `out/` com arquivos `.d` que apontam para outro caminho (ex.: `D:/MegaDrive/Projects/MegaMetroid`), a build falha com `No rule to make target ...`. Solução: apague `out/` e compile de novo.
