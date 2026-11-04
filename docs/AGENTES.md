# Agentes e uso econômico da assinatura

## Configuração recomendada

No T3 Code, selecione **Claude → Sonnet 5.5 → low** na conversa que recebe seus
pedidos. O coordenador classifica a tarefa e aciona o especialista quando isso
justificar outra janela de contexto. Não é necessário escolher Opus para toda
conversa só para poder usar Opus em uma dúvida difícil.

`.claude/settings.json` define Sonnet 5.5 e esforço low para o Claude Code neste
projeto. O T3 Code pode passar modelo/esforço próprios, que têm prioridade;
confira a seleção na interface. Estes arquivos não mudam a configuração global
do usuário nem a seleção de threads já abertas no T3.

A política usa a assinatura Claude, sem habilitar cobrança extra. Subagentes
Claude também consomem seus limites; delegar não significa uso gratuito.
O custo total inclui contexto, raciocínio, respostas, ferramentas e retrabalho.
Os níveis abaixo são um ponto de partida, não uma garantia de menor consumo.

## Papéis

| Papel | Modelo Claude | Esforço | Exemplo |
|---|---|---|---|
| Coordenador | Sonnet 5.5 | low | Receber pedido, escolher etapa e fechar entrega. |
| `consulta` | Haiku | low | Encontrar quem chama uma função e quais testes a cobrem. |
| `implementacao` | Sonnet | low | Implementar comportamento já especificado e medido. |
| `problemas` | Sonnet | high | Investigar slowdown, DMA, áudio ou divergência de física. |
| `revisao` | Sonnet | medium | Revisar uma mudança de lógica sem riscos de hardware. |
| `decisao` | Opus | low | Resolver um impasse com evidências ou comparar arquiteturas. |

Os arquivos em `.claude/agents/` especificam ferramentas e limites de turnos.
Os modelos do coordenador e dos filhos estão fixados nos IDs 5.5 tanto nos
arquivos nativos quanto nos alvos T3. Mudanças de disponibilidade exigem conferência,
sem substituir o modelo silenciosamente.

## Como o pedido deve ser encaminhado

1. Resposta factual pequena ou edição trivial: o coordenador faz diretamente.
2. Consulta extensa, mas delimitada e sem experimento: `consulta`.
3. Etapa com critérios e solução conhecidos: `implementacao`, se separar o
   contexto compensar; caso contrário, o coordenador implementa.
4. Causa desconhecida ou risco de VDP/DMA, VRAM, sprites, XGM2 ou fidelidade:
   `problemas` diretamente. Não gastar tentativas de baixo esforço nesse caso.
5. Duas tentativas sem reduzir a incerteza: devolver diagnóstico ao coordenador,
   que pode acionar `decisao` com uma pergunta concreta e as evidências.
6. Antes de concluir mudança de comportamento: uma revisão independente e
   delimitada, com `revisao` ou `problemas`, conforme o risco. Documentação e
   formatação não precisam desse custo adicional.

Não criar agentes separados só para executar um comando de teste, escrever o
changelog ou fazer commit. O responsável pela etapa executa seus testes; o
coordenador confere as evidências e faz a entrega. Não repetir validações que
já passaram sem mudanças posteriores ou dúvida relevante. O limite de turnos
de um filho não prova que a tarefa terminou: resultado parcial deve ser tratado
como parcial, sem reiniciar o mesmo trabalho automaticamente.

## Delegação no T3 Code

As capacidades foram conferidas no ambiente em 04/11/2026. Use
`orchestrator_capabilities` antes da primeira delegação da thread, conforme as
regras do T3, para conferir novamente disponibilidade e nomes das opções.
Para modelo/esforço explícitos, use `delegate_task` com estes alvos:

| Papel | `target.providerInstanceId` | `target.model` | `target.options` |
|---|---|---|---|
| `consulta` | `claudeAgent` | `claude-haiku-5-5` | `{"effort":"low"}` |
| `implementacao` | `claudeAgent` | `claude-sonnet-5-5` | `{"effort":"low"}` |
| `problemas` | `claudeAgent` | `claude-sonnet-5-5` | `{"effort":"high"}` |
| `revisao` | `claudeAgent` | `claude-sonnet-5-5` | `{"effort":"medium"}` |
| `decisao` | `claudeAgent` | `claude-opus-5-5` | `{"effort":"low"}` |

No briefing, inclua a leitura de `.claude/agents/<papel>.md`, `AGENTS.md` e
`MEMORY.md`, mais objetivo, arquivos, evidências, critério de conclusão e limites.
O filho do T3 não recebe automaticamente a conversa do pai nem necessariamente
aplica o frontmatter nativo. O coordenador deve fornecer o contexto suficiente
e respeitar o limite de trabalho descrito no papel, mesmo nesse caminho.
Use o checkout compartilhado para a etapa sequencial; mantenha só um escritor.
Uma revisão ou nova conferência usa outra delegação. Siga o `next` retornado pelo
T3; não faça polling nem crie uma segunda delegação para o mesmo trabalho ativo.

No Claude Code sem T3, os agentes nativos carregam as definições de
`.claude/agents/`. Como a pasta foi criada agora, abra uma nova sessão para
garantir o carregamento e confira em `/agents`. Modelo/esforço no frontmatter
devem aparecer na execução. Variáveis como `CLAUDE_CODE_EFFORT_LEVEL` e
`CLAUDE_CODE_SUBAGENT_MODEL_FORCE` podem sobrescrever o encaminhamento; se os
níveis não corresponderem, confira essas configurações sem divulgar segredos.

Definições e descrições orientam a seleção pelo modelo; não são um roteador
determinístico. Confira o modelo efetivo do filho e não presuma que o custo caiu
só por haver mais agentes. As restrições de ferramentas nativas também não são
automaticamente aplicadas a filhos T3: as instruções de cada papel continuam
obrigatórias, mas não constituem uma barreira técnica nesse caminho.
O agente `problemas` tem ferramentas de edição porque também implementa
correções; ao ser chamado em modo revisão, a proibição de editar é uma instrução,
não uma restrição técnica. Para revisão comum, `revisao` não recebe Edit/Write.

## Exemplo de pedido

```md
Leia CLAUDE.md e AGENTS.md. Escolha o agente conforme o risco e a incerteza,
seguindo a política de economia. Resolva [problema ou etapa], preserve alterações
existentes e conclua uma entrega verificável. Use um especialista apenas quando
compensar separar o contexto. Informe no resultado os modelos usados, a validação
e as pendências. Não habilite cobrança extra.
```

Não precisa repetir a tabela em cada pedido: ela já está no `AGENTS.md`.
Para documentação simples, diga diretamente o que deve mudar. Para hardware e
desempenho, informe cenário e sintoma; o coordenador encaminha a `problemas`.

## Fontes e validação

- [Subagentes do Claude Code](https://code.claude.com/docs/en/sub-agents).
- [Modelos e esforço](https://code.claude.com/docs/en/model-config).

Ambiente conferido: Claude Code 2.1.318. A disponibilidade retornada pelo T3
não garante cota restante ou acesso de cada conta ao modelo. Se houver recusa
de acesso, informe e não troque para API paga ou outro provedor automaticamente.
