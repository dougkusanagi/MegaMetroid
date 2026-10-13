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
3. **A cada etapa testada e funcionando, faça commit e push.** Não acumule várias etapas em um único commit e não faça commit de etapa quebrada ou não testada.
   - Mensagem de commit clara, descrevendo a etapa (ex.: `Etapa 0.3: corrige recursão em stopSoundJump`).
   - Depois do commit, execute `git push`.
4. Marque a etapa como concluída em `docs/PLANO.md` e atualize o `README.md` quando a mudança afetar o uso.
5. Depois de cada alteração testada e commitada, rebuilde a ROM padrão, para que `build.bin` reflita o código: `./scripts/build.ps1 -Sgdk D:/apps/sgdk -Java D:/apps/jre/bin/java.exe`. Não use `-SmokeTest` nem `-Define` nesse passo, pois eles geram ROMs de teste em `tests/out/`.

## Convenções

- Linguagem do código: C (SGDK), no estilo do arquivo editado.
- Evite alocação dinâmica em tempo de execução; prefira arrays estáticos e dados em ROM (`const`).
- Respeite os limites do hardware (VRAM, 80 sprites, 20 por scanline, 64 KB de RAM).
