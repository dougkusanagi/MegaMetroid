# AGENTS.md

Instruções para agentes (e contribuidores) que trabalham neste repositório.

## Projeto

Port de Super Metroid para Mega Drive / Genesis usando o [SGDK](https://github.com/Stephane-D/SGDK).
O plano de trabalho está em [`docs/PLANO.md`](docs/PLANO.md) — siga as etapas na ordem, salvo orientação em contrário.

## Escopo e avaliação de tarefas

- Preserve o objetivo do porte e a fidelidade ao original dentro dos limites do Mega Drive. Não substitua o pedido por um jogo com identidade própria sem orientação do usuário.
- `docs/echo-below/`, `docs/plans/echo-below.md` e `docs/divulgacao/` são projetos paralelos (homebrew original e divulgação). Não fazem parte do porte nem de `docs/PLANO.md`: "continuar o desenvolvimento" significa seguir o `docs/PLANO.md`. Trabalhe neles só com pedido explícito do usuário.
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
   - Em trabalho delegado, esta etapa cabe exclusivamente ao coordenador; os filhos retornam alterações e evidências, sem commit/push.
   - Mensagem de commit clara, descrevendo a etapa (ex.: `Etapa 0.3: corrige recursão em stopSoundJump`).
   - Depois do commit, execute `git push`.
4. Marque a etapa como concluída em `docs/PLANO.md` e atualize o `README.md` quando a mudança afetar o uso.
   - Atualize também `CHANGELOG.md` a cada entrega, com data e hora em `America/Sao_Paulo` (inclua o fuso), categorias aplicáveis (Adicionado, Melhorado, Corrigido, Removido), efeito da mudança e validação. Mantenha as entradas mais recentes no topo; use o mesmo arquivo, sem criar um por commit.
5. Depois de cada alteração testada e commitada, rebuilde a ROM padrão, para que `out/rom.bin` reflita o código: `./scripts/build.ps1 -Sgdk D:/apps/sgdk -Java D:/apps/jre/bin/java.exe`. Não crie `build.bin` na raiz: `out/` é o destino padrão para jogar. Não use `-SmokeTest` nem `-Define` nesse passo, pois eles geram ROMs de teste em `tests/out/`.

## Convenções

- Linguagem do código: C (SGDK), no estilo do arquivo editado.
- Evite alocação dinâmica em tempo de execução; prefira arrays estáticos e dados em ROM (`const`).
- Respeite os limites do hardware (VRAM, 80 sprites, 20 por scanline, 64 KB de RAM).

## Agentes e economia de uso

Ao começar a conversa no T3 Code, escolha uma destas entradas:

- **Claude → Sonnet 5.5 → low** para usar a assinatura Claude.
- **Codex → GPT-6.1 Sol → low → service tier default** para usar a assinatura ChatGPT.

Esses são os coordenadores definidos para este fluxo; não escolha qualquer modelo
esperando o mesmo encaminhamento. Classificam o pedido, resolvem tarefas triviais
e coordenam uma etapa por vez. Não altere automaticamente o provedor escolhido.
Os agentes estão em `.claude/agents/` e `.codex/agents/`; instruções em
[`docs/AGENTES.md`](docs/AGENTES.md). A tabela abaixo define as duas famílias:

| Agente | Claude / esforço | GPT / esforço | Quando usar |
|---|---|---|---|
| `consulta` | Haiku 5.5 / low | GPT-6 Luna / low | Localizar chamadas, testes e referências ou responder uma pergunta factual delimitada, sem editar. |
| `implementacao` | Sonnet 5.5 / low | GPT-6 Luna / medium | Implementar uma etapa já definida, sem dúvida de arquitetura ou física. |
| `problemas` | Sonnet 5.5 / high | GPT-6.1 Sol / high | Investigar causa desconhecida, desempenho, VDP/DMA, VRAM/sprites, XGM2 e divergências de fidelidade. |
| `revisao` | Sonnet 5.5 / medium | GPT-6.1 Sol / medium | Revisar um diff de comportamento e suas evidências, sem implementar. |
| `decisao` | Opus 5.5 / low | GPT-6.1 Sol / high | Decisão delimitada de arquitetura com alternativas reais; não reinicia diagnóstico esgotado. Uso excepcional. |

- Escolha o papel pelo risco e pela incerteza, não só pelo tamanho do diff. Se
  não for necessário contexto separado, faça o trabalho no agente principal;
  não abra um subagente para uma consulta ou edição trivial.
- Ao delegar, informe objetivo, arquivos relevantes, evidências existentes,
  critério de conclusão e o que não deve ser alterado. Peça um resumo curto de
  resultado, validação, riscos e próximo passo; não duplique a investigação.
- Subagentes não fazem commit/push e não delegam novamente. O coordenador
  mantém a responsabilidade pelos testes finais, documentação e entrega.
- Use um subagente por vez e apenas um escritor no checkout. Não execute duas
  builds ou sessões de emulador em paralelo; BlastEm compartilha a porta 1234.
- Antes de mudar física ou sprites, consulte as referências existentes. Passe
  diretamente a `problemas` os casos de hardware, áudio e fidelidade acima.
  Também escale após duas falhas pela mesma causa ou quando a solução deixar
  de ser uma implementação já definida. Não repita tentativas sem nova evidência.
- `problemas` trabalha em uma hipótese mensurável por vez. Após duas tentativas
  sem reduzir a incerteza, devolve evidências, tentativas e dúvida concreta ao
  coordenador. No Codex, encerre a investigação com o estado e a dúvida; não
  abra outro Sol high só para repetir o diagnóstico. `decisao` cabe em uma
  pergunta distinta e delimitada de arquitetura. No Claude, o coordenador pode
  acionar `decisao` uma vez. Não crie ciclos de agentes.
- Antes de concluir mudança de comportamento, peça uma revisão independente do
  diff. Use `problemas` para riscos de hardware/fidelidade; nos demais, `revisao`.
  Documentação e formatação não exigem revisor. Quem implementou não se apresenta
  como revisor independente; uma nova revisão usa outra invocação.
- Não escale modelo, esforço ou provedor silenciosamente se o modelo estiver
  indisponível. Informe a limitação e continue apenas trabalho independente.
- Mantenha filhos Claude na assinatura Claude e filhos GPT na assinatura
  ChatGPT/Codex. Use service tier `default` no T3 para GPT; não habilite priority,
  cobrança extra ou API paga. Não adote outro provedor automaticamente.
  A presença de um modelo no catálogo não demonstra acesso ou cota na conta.
  Astra fica fora da seleção automática: só use com pedido explícito do usuário.
  O DeepSeek antes recomendado fica
  como opção somente se o usuário pedir esse provedor; confirme capacidades e
  use a delegação do T3 Code, nunca o trate como modelo nativo do Claude.
