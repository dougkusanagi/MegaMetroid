# AGENTS.md

Instruções para agentes (e contribuidores) que trabalham neste repositório.

## Projeto

Port de Super Metroid para Mega Drive / Genesis usando o [SGDK](https://github.com/Stephane-D/SGDK).
O plano de trabalho está em [`docs/PLANO.md`](docs/PLANO.md) — siga as etapas na ordem, salvo orientação em contrário.

## Estrutura

- `src/` — código C (`main.c`, `entity.c`, `physics.c`, mapas de colisão por sala)
- `inc/` — headers
- `res/` — recursos SGDK (`*.res`, imagens, sons)
- `docs/` — documentação e plano

## Fluxo de trabalho

1. Trabalhe em uma etapa do plano por vez.
2. Compile e teste a etapa até estar funcionando.
   - **Fidelidade ao Super Metroid:** consulte primeiro as medições versionadas em `tests/reference/super_metroid/` e as conclusões em `docs/FISICA.md` / `docs/SPRITES.md`. Reutilize os dados existentes; não execute o Mesen a cada alteração. Há um Mesen 2 com ROM e save state original neste ambiente: use `scripts/sm_reference.ps1` (`--testrunner`, sem interface) apenas quando faltar cobertura, houver dúvida/contradição ou mudar a referência. Preserve novas medições com `scripts/sm_reference_cache.py`, incluindo origem, cenário e limites de precisão. Caminhos e instruções em `docs/BUILD.md`. Não trate um conjunto finito de amostras como prova de todos os comportamentos do original.
   - **Os testes da ROM devem ser feitos no BlastEm via API/protocolo GDB**, usando `python scripts/smoke_test.py` após a build `-SmokeTest` (ver `docs/BUILD.md`). Não use skills de controle do computador, cliques, teclado ou automação da interface do emulador. Mostre a sessão de teste no monitor secundário, sem ativar a janela nem roubar o foco; sem monitor secundário, mantenha-a oculta. `--hidden` força a sessão oculta. Encerre apenas o processo iniciado pelo teste, sem interferir no uso do PC. Testes do núcleo no host complementam, mas não substituem, a validação da ROM no BlastEm.
3. **A cada etapa testada e funcionando, faça commit e push.** Não acumule várias etapas em um único commit e não faça commit de etapa quebrada ou não testada.
   - Mensagem de commit clara, descrevendo a etapa (ex.: `Etapa 0.3: corrige recursão em stopSoundJump`).
   - Depois do commit, execute `git push`.
4. Marque a etapa como concluída em `docs/PLANO.md` e atualize o `README.md` quando a mudança afetar o uso.
   - Atualize também `CHANGELOG.md` a cada entrega, com data e hora em `America/Sao_Paulo` (inclua o fuso), categorias aplicáveis (Adicionado, Melhorado, Corrigido, Removido), efeito da mudança e validação. Mantenha as entradas mais recentes no topo; use o mesmo arquivo, sem criar um por commit.
5. Depois de cada alteração testada e commitada, rebuilde a ROM padrão, para que `build.bin` reflita o código: `./scripts/build.ps1 -Sgdk D:/apps/sgdk -Java D:/apps/jre/bin/java.exe`. Não use `-SmokeTest` nem `-Define` nesse passo, pois eles geram ROMs de teste em `tests/out/`.

## Convenções

- Linguagem do código: C (SGDK), no estilo do arquivo editado.
- Evite alocação dinâmica em tempo de execução; prefira arrays estáticos e dados em ROM (`const`).
- Respeite os limites do hardware (VRAM, 80 sprites, 20 por scanline, 64 KB de RAM).
