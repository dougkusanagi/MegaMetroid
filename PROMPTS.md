# Como pedir trabalho no MegaMetroid

Use estes exemplos para dar contexto e definir entregas verificáveis ao Claude
ou a outro agente. O objetivo continua sendo o porte de Super Metroid para Mega
Drive/Genesis com SGDK. Não há uma lista comprovada de palavras que provoque ou
impeça recusas; os exemplos reduzem ambiguidades, sem garantir aceitação nem
substituir as regras do provedor.

## Antes de pedir

- Inicie a sessão na raiz deste repositório e peça a leitura de `CLAUDE.md`,
  `AGENTS.md` e `MEMORY.md`.
- Descreva o sintoma observado, a sala, a ação e o resultado esperado. Quando
  possível, informe como repetir. Se não souber a causa, diga isso.
- Escolha uma entrega por vez. Para vários defeitos, peça triagem e priorização
  antes de implementar a primeira correção.
- Defina a evidência esperada: medição arquivada, regressão, comparação visual
  ou resultado da ROM no BlastEm. Não apresente uma suspeita como fato.
- Informe a origem e as permissões dos recursos quando forem relevantes e
  conhecidas. Se não souber, não invente.

## Pedido geral: investigar e corrigir

```text
Leia CLAUDE.md, AGENTS.md e MEMORY.md. Continue o MegaMetroid dentro do objetivo
do porte e preserve as alterações existentes no repositório.

Tenho observado estes problemas:
- HUD com diferenças visuais em relação à referência.
- Tiro durante a corrida com movimento aparentemente diferente.
- Slowdowns durante os saltos.

Trate essas observações como hipóteses a verificar. Consulte as referências
versionadas e o plano, reproduza os defeitos e priorize bloqueios de execução
e desempenho. Comece por uma correção verificável e mantenha os demais itens
registrados no plano da tarefa. Tome as decisões técnicas dentro desse escopo,
justifique as escolhas com evidências e siga a validação e a entrega do projeto.
Não declare resolvido nem fiel ao original o que não foi verificado.
```

## Desempenho durante o salto

```text
Leia as instruções do projeto. Investigue os slowdowns durante os saltos.
Determine um cenário reproduzível e compare o trabalho por quadro parado,
andando e saltando, com câmera parada e em movimento quando aplicável.
Verifique as causas no código e nas medições, incluindo renderização, DMA,
sprites e colisão, sem assumir antecipadamente qual delas é responsável.
Corrija a causa identificada sem mudar a física para mascarar a lentidão.
Valide a ROM no BlastEm via GDB, acelerado, sem áudio e headless, e registre
o que foi medido antes e depois e os limites dessa comparação.
```

## Tiro durante a corrida

```text
Leia as instruções do projeto e as medições arquivadas de projéteis.
Investigue se o Power Beam durante a corrida diverge da referência medida.
Compare posição inicial, deslocamento por tick e trajetória em relação à sala
e à Samus, nos dois sentidos e em condições equivalentes. Separe movimento
da câmera de movimento do projétil. Não suponha que o tiro herda velocidade
nem que tem aceleração sem conferir a evidência.
Se a cobertura não bastar, siga as regras do projeto para novas medições.
Corrija divergências comprovadas, acrescente regressão adequada e valide a ROM.
```

## HUD

```text
Leia as instruções do projeto. Compare o HUD atual com as referências visuais
disponíveis: posição, espaçamento, tiles, cores, números, ícones e prioridade
de desenho. Liste diferenças observadas e adaptações necessárias aos limites
do Mega Drive. Corrija uma diferença verificável por vez, usando o pipeline
existente e informando os recursos envolvidos. Valide a apresentação na ROM
pelo procedimento de inspeção previsto no projeto; testes do núcleo sozinhos
não demonstram correspondência visual.
```

## Retomar depois de uma recusa ampla

```text
Reavalie a tarefa concreta à luz da seção de escopo do AGENTS.md.
O pedido atual é diagnosticar e corrigir [descreva o defeito] no código deste
repositório. Explique qual operação específica estaria impedida e o motivo,
caso exista um impedimento aplicável. Diferencie essa operação das partes
independentes que você pode executar e prossiga com elas. Não proponha mudar
a identidade ou o objetivo do projeto como condição para todo trabalho técnico.
```

Substitua o trecho entre colchetes antes de enviar. Se a resposta apontar um
limite específico do provedor, nenhuma redação deste arquivo garante removê-lo.
Para avaliar uma nova recusa, guarde o pedido e a resposta completos.

## O que evitar e por quê

- **Pedidos sem limite**, como “faça tudo igual ao original” ou “resolva tudo”.
  Eles deixam o escopo e a conclusão indefinidos. Nomeie os comportamentos e
  peça critérios de comparação e uma entrega por vez.
- **Causas presumidas**, como “a aceleração do tiro está errada”, sem medição.
  Descreva a diferença percebida e peça que a hipótese seja verificada.
- **Garantias absolutas**, como “100% idêntico” ou “sem nenhum slowdown”.
  Peça resultados em cenários definidos, com limites da validação.
- **Autorizações inventadas**, como “tenho todos os direitos”, quando isso não
  foi estabelecido. Uso privado e posse de uma ROM não demonstram, por si sós,
  permissão para qualquer uso ou distribuição.
- **Ocultar o porte ou trocar seu nome para disfarçar o objetivo**. Preserve o
  contexto real e delimite a operação técnica que está sendo solicitada.
- **Chamar o projeto de clean room**. O fluxo atual usa medições de memória e
  extração de recursos; essa descrição não corresponde ao que ele faz.
- **Ordens para ignorar políticas ou nunca recusar**. Peça uma avaliação
  específica e continuidade das partes que possam ser executadas.
- **Apagar regras e testes para facilitar a aceitação**. Preserve medições,
  validação no BlastEm e limites do hardware: são critérios de qualidade.

Essas orientações são sobre clareza e delimitação do trabalho. Não são uma
lista de termos proibidos: mencionar Super Metroid, porte, HUD ou fidelidade
é parte do contexto real do projeto.
