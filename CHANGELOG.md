# Histórico de alterações

Registro das mudanças entregues, em ordem cronológica inversa. Cada entrada usa
data e hora com fuso explícito (`America/Sao_Paulo`, UTC−03:00), descreve o efeito
no jogo e informa a validação realizada. Novas entregas são acrescentadas neste
mesmo arquivo; não se cria um arquivo por commit. As categorias são **Adicionado**,
**Melhorado**, **Corrigido** e **Removido**, usadas quando aplicáveis.

O registro detalhado começa em 18/10/2026. A entrada anterior abaixo foi
reconstituída do commit indicado; o histórico anterior completo permanece no Git
(`git log`) e as entregas por etapa estão em [docs/PLANO.md](docs/PLANO.md).

## 2026-10-18 15:38:07 −03:00 — Pulo giratório em rampas

### Corrigido

- Pular girando enquanto sobe uma rampa de 45° deixa de zerar a velocidade
  horizontal e o bônus de corrida. A subida é resolvida antes da colisão
  horizontal, evitando interpretar a superfície sob os pés como uma parede.

### Adicionado

- Regressão para decolagem nas duas orientações de rampa, em 64 posições
  subpixel por orientação, caminhando e com corrida máxima.
- Teste da ROM por `scripts/smoke_test.py`, usando a API GDB do BlastEm e um
  resultado explícito em RAM. O emulador roda oculto, sem automação de interface;
  o script encerra somente o processo que iniciou e recusa uma porta já ocupada.
- Este histórico centralizado, com data, hora, categorias e validação; link no
  README e instrução de manutenção no AGENTS.md.

**Validação:** testes do núcleo com gcc (incluindo 40.000 ticks nos mapas reais),
build SGDK da ROM de teste e `SMOKE PASS` pela API GDB do BlastEm.

## 2026-10-18 15:30:43 −03:00 — Power Beam e impactos (`2dd0f52`)

### Melhorado

- Power Beam usa a arte original nas oito direções, com paleta compartilhada
  entre projétil e impacto, sem perda de cores.

### Adicionado

- Impacto de cinco quadros no contato com paredes, com duração de dez ticks.
- Geração reproduzível das tiras de tiro e impacto; testes de contato varrido,
  duração do efeito e saída da tela sem impacto.

### Removido

- Tile manual que representava o tiro.

**Validação registrada na entrega:** testes do núcleo, build SGDK e `SMOKE PASS`
no BlastEm. ROM padrão reconstruída após o commit.
