# MegaMetroid — contexto para o Claude

@AGENTS.md
@MEMORY.md

## Objetivo

Continuar o porte de Super Metroid para Mega Drive/Genesis em C com SGDK.
Pedidos de maior fidelidade abrangem comportamento, HUD, animações e desempenho.
Leia a seção de escopo do `AGENTS.md` antes de avaliar a tarefa concreta.

## Seleção de agentes

Siga a tabela e os critérios de economia do `AGENTS.md`. Classifique o pedido
antes de investigar; não leia o repositório inteiro só para escolher um agente.
No T3 Code, uma delegação com modelo/esforço próprios usa `delegate_task` com
`target.options` explícito, conforme `docs/AGENTES.md`. Não presuma que o agente
do T3 carrega o corpo de `.claude/agents/`: indique o arquivo do papel no briefing.
Fora do T3, use os agentes nativos definidos nessa pasta.
Tarefas triviais ficam no coordenador. Não delegue novamente dentro de um filho.

## Estudar → implementar → comparar → corrigir

1. Leia o fluxo afetado e a etapa correspondente de `docs/PLANO.md`. Preserve
   alterações já existentes. Para pedidos amplos de correção, reproduza os
   defeitos e escolha uma correção verificável por vez, começando pelos bloqueios
   de execução e desempenho; registre o restante no plano da tarefa.
2. Consulte primeiro `tests/reference/super_metroid/`, `docs/FISICA.md` e
   `docs/SPRITES.md`. Separe fatos medidos, inferências e lacunas. Não invente
   constantes de física nem declare equivalência fora dos cenários medidos.
3. Implemente no código compartilhado do projeto, respeitando RAM, VRAM, DMA e
   limites de sprites. Para slowdowns, meça o custo e identifique a causa antes
   de alterar física ou reduzir fidelidade para esconder o problema.
4. Compare cenários equivalentes: no HUD, posição, tiles, paletas e espaçamento;
   no tiro durante a corrida, trajetória em relação à sala e à Samus, velocidade
   inicial e variação por tick; nos saltos, custo por quadro e trabalho de
   renderização. Essas são perguntas de investigação, não causas já comprovadas.
5. Rode os testes locais afetados e valide mudanças da ROM no BlastEm via GDB,
   acelerado, sem áudio e headless, conforme `AGENTS.md`. Use o Mesen apenas nas
   condições previstas ali. Registre cenário, resultado e limites da validação.
6. Corrija divergências verificadas e conclua cada etapa com a documentação,
   commit, push e rebuild padrão exigidos pelo projeto. Não marque como resolvido
   algo que só foi inspecionado no código.

## Origem da abordagem

Adaptação do ciclo de evidências do
[Craft Anything](https://github.com/storytold/craft-anything).
Este projeto consulta medições de memória e possui um pipeline de extração de
recursos; não o descreva como clean room. A abordagem não desativa restrições do
Claude nem demonstra autorização para usar ou distribuir recursos de terceiros.
