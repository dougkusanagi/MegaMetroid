# Histórico de alterações

Registro das mudanças entregues, em ordem cronológica inversa. Cada entrada usa
data e hora com fuso explícito (`America/Sao_Paulo`, UTC−03:00), descreve o efeito
no jogo e informa a validação realizada. Novas entregas são acrescentadas neste
mesmo arquivo; não se cria um arquivo por commit. As categorias são **Adicionado**,
**Melhorado**, **Corrigido** e **Removido**, usadas quando aplicáveis.

O registro detalhado começa em 18/10/2026. A entrada anterior abaixo foi
reconstituída do commit indicado; o histórico anterior completo permanece no Git
(`git log`) e as entregas por etapa estão em [docs/PLANO.md](docs/PLANO.md).

## 2026-10-18 16:25:15 −03:00 — Hitboxes originais por estado (2.8.4)

### Melhorado

- A Samus usa caixas de 10 px de largura, com alturas de 42 em pé, 38 no pulo
  normal, 24 no giro, 32 agachada e 14 em bola. Mirar diretamente para baixo no
  ar usa 20 px. As medidas vêm das capturas de RAM já arquivadas.
- Mudanças de caixa preservam o apoio no chão ou o centro no ar. A expansão é
  adiada se estiver bloqueada; pousos sob teto tentam a caixa agachada.
- Morfar cancela o giro; desmorfar usa a caixa de transição antes de retornar
  ao estado seguinte. Nenhuma nova execução do Mesen foi necessária.
- Mantido suporte a caixas personalizadas nos testes genéricos do núcleo.

### Adicionado

- Expectativas de hitbox geradas das capturas originais versionadas e testes
  do perfil real: ambos os lados, transições, passagens estreitas, tetos,
  72 superfícies de rampa e 80 mil ticks nos mapas de Crateria.
- A ROM verifica pela API as seis dimensões de colisão durante o roteiro.

**Validação:** testes do núcleo com gcc, pulo normal a 111 px/92 ticks, build
SGDK e `SMOKE PASS` no BlastEm via API GDB. Documentados os limites dos raios
de um único quadro nas transições do original.

## 2026-10-18 16:17:35 −03:00 — Referência do original reutilizável

### Adicionado

- Arquivo de 71 cenários medidos no Mesen, com 20.442 amostras de RAM e entradas
  por quadro, em CSV comprimido sem perdas. Manifesto com origem, datas,
  checksums e variações de hitbox observadas; sem ROM ou save state no repositório.
- Importador/validador das capturas, integrado aos testes do host.

### Melhorado

- `AGENTS.md` orienta reutilizar os dados salvos e iniciar o Mesen somente
  quando faltar cobertura, surgir dúvida/contradição ou mudar a referência.
- Documentados os limites: cenários finitos não descrevem toda a lógica do jogo;
  raios transitórios e capturas antigas incompletas não são descartados.

**Validação:** checksums e sequência das 20.442 amostras, regressões do núcleo,
build SGDK e teste da ROM pela API GDB do BlastEm. Nenhuma nova execução do
Mesen foi necessária para arquivar as medições existentes.

## 2026-10-18 16:03:11 −03:00 — Fidelidade de agachar, bola e pousos (2.8.3)

### Corrigido

- A Samus permanece agachada após soltar ↓, inclusive depois de segurá-lo por
  mais tempo. ↑, movimento ou pulo permitem levantar quando há espaço.
- ↑ ou C desfaz a bola sem impulso de pulo. No chão, termina agachada e só
  outro aperto de ↑ levanta; segurar o botão não conta como um novo aperto.
- Sair da bola no chão exige espaço para agachar, sem exigir a altura em pé.
  Teto baixo também impede um pulo agachado que colocaria a Samus dentro dele.
- Transições de abaixar/levantar passam a 3 ticks; morfar/desmorfar a 6.
  Pouso normal dura 7 ticks e pouso de giro 1, com quadros controlados pela
  simulação, conforme medição no original.

### Melhorado

- `AGENTS.md` registra a disponibilidade do Mesen 2, ROM e save state original,
  e exige medir o original antes de ajustar comportamentos de fidelidade.
- O teste pela API GDB informa uma máscara dos grupos que falharam.
- Documentação distingue as ações já implementadas dos ajustes de fidelidade;
  hitboxes originais continuam reservadas para a etapa 2.8.4.

**Validação:** sete cenários no Mesen, regressões do núcleo com gcc (tempos,
dois lados, tetos, botões segurados e mapas reais), build SGDK e teste da ROM
no BlastEm via API GDB.

## 2026-10-18 15:47:57 −03:00 — Virada da Samus (etapa 3.2.7)

### Adicionado

- Animação de virada original nos dois lados: `25/26` no chão e `2F/30` no ar.
  São três quadros de dois ticks, com a tira escolhida pelo lado inicial da
  Samus, conforme medição no Mesen. Depois, retorna à pose de movimento.
- Regressões de duração, orientação e cancelamento por giro/bola; teste na ROM
  verifica os três quadros, nos dois lados, no chão e durante o pulo.

### Melhorado

- A transição visual de direção acompanha a simulação sem alterar aceleração,
  frenagem ou velocidade. Mira e tiro mantêm suas poses; bola, giro e chute de
  parede mantêm suas animações próprias.
- `AGENTS.md` exige teste da ROM no BlastEm via API GDB, sem skills de controle
  do computador nem automação da interface, com sessão oculta e isolada.
- Concluída a sequência 3.2.1–3.2.7 de importação das animações atuais.

**Validação:** medição no Mesen (`turn`, `turn_moving`, `air_turn_rest`), geração
da folha, testes do núcleo com gcc, build SGDK e `SMOKE PASS` pela API do BlastEm.

## 2026-10-18 15:41:13 −03:00 — Descida do pulo sem giro

### Corrigido

- O pulo sem giro mantém a animação própria de pulo durante a descida,
  em vez de trocar no ápice para a pose de queda de borda.
- Os quadros deixam de repetir em loop: o quadro de subida fica até iniciar
  a descida; os três quadros seguintes duram quatro ticks cada, e o último
  permanece até pousar. O avanço segue os ticks da simulação também em PAL.

### Melhorado

- A medição do original no Mesen agora registra também quadro e contador da
  animação, permitindo reproduzir a comparação com o SNES.

**Validação:** sequência medida no Mesen (`jump_stand`), regressões do núcleo
para ambos os lados e pulos longos/curtos, build SGDK e teste da ROM pela API
GDB do BlastEm verificando a tira e os quadros renderizados para os dois lados.

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
