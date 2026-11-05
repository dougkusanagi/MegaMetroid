# Plano ativo — MegaMetroid

Plano enxuto para os agentes: leia este arquivo e o [`BUGS.md`](BUGS.md) antes de
continuar. O registro detalhado de cada etapa (medições, custos, validações) fica em
[`HISTORICO.md`](HISTORICO.md): consulte só a seção da etapa em andamento.

Última revisão: 2026-11-04.

## Objetivo: o que é um porte decente

Jogabilidade e aparência **o mais semelhantes possível** ao Super Metroid, dentro dos
limites do Mega Drive (64 KB de VRAM e RAM, 80 sprites e 20 por linha, 4 paletas de 16
cores, ROM de até 4 MB sem mapper). Na prática:

- Controle e física iguais aos medidos no original (`tests/reference/super_metroid/`,
  `docs/FISICA.md`). Diferenças só quando o hardware obrigar, registradas.
- Sprites, cenários, HUD e animações iguais ao original na posição, nas cores
  (precisão do VDP) e nos tempos.
- 60 fps sem quadros perdidos nas salas normais, medido no smoke do BlastEm.
- Nenhum travamento ou defeito de prioridade conhecido sem registro no `BUGS.md`.

## Como continuar

1. Comece pelos defeitos de prioridade **alta** do `BUGS.md`; depois siga a próxima
   tarefa abaixo.
2. Ao concluir: marque aqui, acrescente o relato detalhado em `HISTORICO.md`, atualize
   o `CHANGELOG.md` e feche ou crie as linhas do `BUGS.md` afetadas.
3. Defeito encontrado no caminho e fora do escopo: registre no `BUGS.md` e continue.
4. Ao fechar um marco, peça um playtest ao usuário (15 min, anotado no `BUGS.md`).

## Marco atual: A — Crateria inicial completa

Landing Site e Crateria 2 jogáveis como no original: inimigos que causam dano, morte e
reinício, drops, portas e HUD fiéis, sem quadros perdidos.

**Próximas tarefas, em ordem:**

1. **5.3 Dano e vida** — implementado (contato, knockback, morte e reinício); falta o cenário
   de validação no smoke do BlastEm (dano, morte, vx do knockback).
2. **5.4 Drops** — lógica e sprites provisórios prontos (`core/drop.c`); falta o cenário de smoke
   (matar, ver e pegar), a arte e as tabelas de drop do original (hoje provisórias).

Depois: Geemer andando em paredes e teto (B3), a paleta do míssil (B2) e a 3.3.

## Etapas pendentes

Concluídas: fases 0, 1 e 2; 3.1, 3.2, 3.4; 4.1–4.3; 5.1, 5.2; 6.0–6.5; 7.1.1; 8.1, 8.4 (B1).
Detalhes em `HISTORICO.md`.

| # | Etapa | Falta | Teste |
|---|-------|-------|-------|
| 3.3 | Animações completas | Moonwalk diagonal e no salto, pouco HP, space jump, screw attack | Todas visíveis em sequência |
| 3.5 | Orçamento de VRAM | Alocação automática / streaming dos tiles do sprite | `SPR_showVRAMUsage` dentro do orçamento |
| 3.6 | Paletas da Samus | Varia/Gravity e ciclos (carga, speed booster, screw, dano), separando a barra do HUD da PAL2 | Ciclos sem trocar tiles |
| 4.4 | Beams | Charge, ice, wave | Cada beam altera sprite e dano |
| 5.3 | Dano e vida | Fonte de dano, knockback, morte e reinício | Morrer e reiniciar a sala |
| 5.4 | Drops | Energia e míssil pequenos | Pegar aumenta os contadores |
| 5.5 | Limite de sprites | Culling fora da tela | Sala cheia sem flicker excessivo |
| 6.6 | Portas e blocos | Blocos destrutíveis; destino das outras portas; portas verticais | Abrir com o tiro correto |
| 7.1 | HUD | Mini-mapa | HUD estável durante o scroll |
| 7.2 | Itens coletáveis | Tanques de energia e míssil; um quadrado por tanque, comparado com `tests/reference/super_metroid/hud/` | Coletar persiste; quadrados na posição e na cor da referência |
| 7.3 | Upgrades | Morph Ball, Bombas, Varia, High Jump | Habilidades liberam trechos do mapa |
| 7.4 | Save em SRAM | Itens, posição e flags | Salvar, resetar, continuar |
| 8.2 | SFX | Porta, dano, inimigo, item, mísseis, nave (tiro, giro e pouso feitos) | Canais PCM não se atropelam |
| 8.3 | Título / game over | Estados de jogo | Título → jogo → game over |
| 8.5 | Release | ROM final, README com controles | BlastEm e hardware real |

## Próximos marcos

- **B — Crateria completa:** todas as salas de Crateria, blocos, Morph Ball e bombas,
  mini-mapa, save na nave.
- **C — Brinstar:** primeira área nova, com tanques e o primeiro chefe.

## Riscos

- **ROM:** a música em PCM ocupa ~1,1 MB por faixa; o jogo inteiro exige FM ou PCM a
  meia taxa. Mais de 4 MB exige mapper SSF.
- **VRAM e paletas:** não há banco de paleta livre (ver `MEMORY.md`); cada sala nova deve
  respeitar as cores reservadas do HUD. Planejar tileset por região.
- **Mapas grandes:** avaliar colisão 16×16 ou RLE.
- **Propriedade intelectual:** projeto de fã; não distribui ROMs. Ver `AGENTS.md`.

## Pronto de uma etapa

Compila sem erros novos; testes do host e smoke do BlastEm passam; `HISTORICO.md`,
`CHANGELOG.md`, `BUGS.md` e, se afetar o uso, `README.md` atualizados; commit e push;
ROM padrão rebuildada em `out/rom.bin`.
