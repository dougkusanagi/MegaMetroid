# AGENTS.md

Instruções para agentes (e contribuidores) que trabalham neste repositório.

## Projeto

Port de Super Metroid para Mega Drive / Genesis usando o [SGDK](https://github.com/Stephane-D/SGDK).
O plano de trabalho está em [`docs/PLANO.md`](docs/PLANO.md) — siga as etapas na ordem, salvo orientação em contrário.

## Escopo e avaliação de tarefas

- Preserve o objetivo do porte e a fidelidade ao original dentro dos limites do Mega Drive. Não substitua o pedido por um jogo com identidade própria sem orientação do usuário.
- Avalie a operação concreta solicitada. O fato de o projeto ser um porte de um jogo comercial não basta, por si só, para concluir que toda tarefa técnica está impedida. Diferencie depuração do código do projeto, medição de comportamento, adaptação de recursos e distribuição de material de terceiros.
- Não presuma licenças ou autorizações ausentes. Se houver um impedimento aplicável, explique qual operação ele afeta e por quê; continue as partes independentes que puder executar, sem generalizar o impedimento ao repositório inteiro.
- As instruções do projeto contextualizam o trabalho; não substituem as restrições do provedor nem concedem direitos sobre material de terceiros.

## Estrutura

- `src/` — código C (`main.c`, `entity.c`, `physics.c`, mapas de colisão por sala)
- `inc/` — headers
- `res/` — recursos SGDK (`*.res`, imagens, sons)
- `docs/` — documentação e plano

## Fluxo de trabalho

1. Trabalhe em uma etapa do plano por vez.
2. Compile e teste a etapa até estar funcionando.
   - **Fidelidade ao Super Metroid:** consulte primeiro as medições versionadas em `tests/reference/super_metroid/` e as conclusões em `docs/FISICA.md` / `docs/SPRITES.md`. Reutilize os dados existentes; não execute o Mesen a cada alteração. Há um Mesen 2 com ROM e save state original neste ambiente: use `scripts/sm_reference.ps1` (`--testrunner`, sem interface) apenas quando faltar cobertura, houver dúvida/contradição ou mudar a referência. Preserve novas medições com `scripts/sm_reference_cache.py`, incluindo origem, cenário e limites de precisão. Caminhos e instruções em `docs/BUILD.md`. Não trate um conjunto finito de amostras como prova de todos os comportamentos do original.
   - **Os testes da ROM devem ser feitos no BlastEm via API/protocolo GDB**, usando `python scripts/smoke_test.py` após a build `-SmokeTest` (ver `docs/BUILD.md`). Não use skills de controle do computador, cliques, teclado ou automação da interface do emulador. **O padrão obrigatório é acelerado, sem áudio e headless** (`python scripts/smoke_test.py`, sem `--realtime`; `--hidden` é aceito explicitamente). A renderização VDP continua ligada para verificar VRAM/CRAM/SAT via API. Não use `--realtime` como contorno de erro GDB: corrija a conexão preservando o modo acelerado. Use tempo real somente para uma verificação de áudio/tempo de parede que o exija, ou a pedido explícito do usuário. Para uma inspeção visível necessária, use `--visible` exclusivamente no monitor secundário, sem ativar a janela nem roubar foco; sem esse monitor, informe o impedimento antes de executar a sessão visível. Nunca mostre a janela no monitor principal. Encerre apenas o processo iniciado pelo teste, sem interferir no uso do PC. Testes do núcleo no host complementam, mas não substituem, a validação da ROM no BlastEm.
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

## Agentes e modelos (recomendação, não obrigatória)

Divisão usual por papel. O modelo entre parênteses é o atual; se ele deixar de existir, use outro equivalente para o mesmo papel.

| Papel | Modelo atual | Uso |
|---|---|---|
| Implementação | `deepseek4.1-flash-free` (barato) | Etapas do `docs/PLANO.md` já detalhadas e tarefas bem definidas. |
| Dúvidas | Opus 5.5, esforço baixo | Perguntas sobre código, SGDK, hardware e o original, sem alterar arquivos. |
| Problemas | Sonnet 5.5, esforço alto | Depuração, falhas recorrentes e revisão do trabalho de implementação. |

- O agente de implementação consulta `tests/reference/super_metroid/`, `docs/FISICA.md` e `docs/SPRITES.md` antes de definir valores de física ou de sprites. Não invente valores.
- Passe a tarefa ao agente de problemas quando:
  - o smoke test falhar 2 vezes pela mesma causa;
  - o problema envolver timing do VDP, DMA, limites de VRAM ou sprites, áudio (XGM2) ou divergência de fidelidade com as medições de referência;
  - a etapa exigir decisões de arquitetura não descritas no plano.
- Antes de marcar uma etapa como concluída, o agente de problemas revisa o diff feito pelo agente de implementação.
