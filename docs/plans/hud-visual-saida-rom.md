# Plano: visual do HUD e saída da ROM

Tipo: tarefa

## Objetivo

Separar a otimização anterior do HUD da fidelidade visual ainda pendente.
Corrigir o visual com base nos recursos/referências existentes e nas capturas
fornecidas, validando na ROM. A ROM padrão para jogar fica em `out/rom.bin`,
sem gerar `build.bin` na raiz.

## Passos

- [x] Conferir thread anterior e causa da cópia de build.bin na raiz.
- [x] Corrigir pipeline, referências operacionais e testar build padrão.
- [ ] Investigar e corrigir visual do HUD com especialista de fidelidade.
- [ ] Correção distinta: reproduzir e corrigir oclusão da Samus pela moldura/túnel à esquerda (seta vermelha), entrada e saída nos dois sentidos, sem ocultar o sprite inteiro nem priorizar toda a sala.
- [ ] Revisar diff, testes locais, smoke GDB e inspeção visual da ROM.
- [ ] Documentar limites reais, commit/push por etapa e rebuild padrão.

## Tentativas que falharam

- Nenhuma.

## Dúvidas abertas

- O minimapa ainda é uma funcionalidade pendente; não fingir implementá-lo com
  um desenho estático. Delimitar fidelidade possível sem mudar progressão do jogo.

## Próximo passo

Encaminhar a correção visual ao especialista de fidelidade, preservando a
otimização anterior. A ROM padrão mantém SHA256 `423b528f…`; cópia antiga da
raiz removida apenas após confirmar igualdade com `out/rom.bin`.
