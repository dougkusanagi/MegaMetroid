# Referência reutilizável do Super Metroid

Capturas de RAM do jogo original no Mesen 2.1.1, SNES NTSC, a partir do save
state do Landing Site documentado em `docs/BUILD.md`. Nenhuma ROM ou save state
é distribuído aqui. `manifest.json` registra hashes de identificação da ROM e
do state disponíveis na importação, cenário, data do arquivo, número de quadros,
checksum do CSV e todas as combinações observadas de pose e raios.

Cada `*.csv.gz` preserva os valores inteiros originais e as entradas por quadro.
A compressão é sem perdas; o checksum é do CSV descomprimido. `saved_at` é a
data de gravação do arquivo de captura. A revisão Git registrada é a última
versão **commitada** do script antes dessa data: alterações ainda não commitadas
na captura podem diferir. As próprias colunas `buttons` e `step` preservam a
sequência efetivamente aplicada. Capturas antigas não têm `anim_frame/anim_timer`.

## Usar primeiro, medir só quando necessário

1. Consulte o manifesto, o CSV do cenário e as conclusões em `docs/FISICA.md` e
   `docs/SPRITES.md`. A leitura não precisa de emulador nem ROM.
2. Os valores de RAM são exatos dentro da captura. Deslocamentos/velocidades
   calculados a partir deles são derivados; cenário, equipamento e colisões
   limitam a generalização. Não é uma extração completa do código do original.
3. Algumas poses têm mais de um raio observado: há transições de um quadro,
   colisões ou estados intermediários. Não escolha silenciosamente só um deles.
4. Execute um novo cenário no Mesen apenas se faltar cobertura, houver dúvida
   ou contradição, ou se a versão/configuração de referência mudar. Depois,
   importe o resultado e atualize a conclusão e os testes afetados.

```powershell
python scripts/sm_reference_cache.py check
# Somente para incorporar uma nova medição, após rodar sm_reference.ps1:
python scripts/sm_reference_cache.py import tests/out/nova_medicao.csv
```

O importador seleciona a captura mais recente por cenário entre os arquivos
passados e mantém os demais cenários arquivados. A atualização de uma referência
deve ser revisada como mudança de dados esperados, e não usada automaticamente
para fazer um teste quebrado passar.

Exemplo de leitura sem emulador:

```python
import csv, gzip
with gzip.open('tests/reference/super_metroid/jump_stand.csv.gz', 'rt') as f:
    frames = list(csv.DictReader(f))
```

Os testes do host validam checksums, sequência de quadros e os resumos de raios.
O validador gera `tests/out/sm_hitboxes.h` com as dimensões estáveis mais
frequentes das poses de referência; os testes C comparam o perfil real da Samus
com esse arquivo, não com cópias dos valores do código de produção. Os dados
transitórios permanecem nos CSVs e não são substituídos pelo resumo modal.
Os testes do porte continuam rodando no BlastEm pela API, conforme `AGENTS.md`.

As seis capturas `beam_*` adicionadas na 2.8.5 cobrem as direções que faltavam.
Incluem subpixel em Y e a coluna `shot_vectors` (velocidades Q8.8 assinadas,
direção e tipo por slot); capturas anteriores preservam seu formato original.
`check` também gera `tests/out/sm_beams.h` com 16 velocidades iniciais dos
cenários `beam_up` e `beam_up_right`. Os testes usam esses dados para conferir
aceleração e trajetórias. Componentes herdadas de Samus/câmera e impactos devem
ser separados da velocidade base; a análise e os limites estão em `docs/FISICA.md`.


`landing_site/` arquiva também uma referência visual pontual: vídeo/PPU do save
no Mesen, screenshot e recorte da prancha nova de Crateria. Seu manifesto separa
origem, cenário, precisão da posição da nave e limitações. Os checksums são
conferidos pelo mesmo `sm_reference_cache.py check`; a regeneração de arte não
executa o Mesen. Não confundir a cor azul de transparência do mapa com o céu real.


`landing_sky/` registra quatro alturas de câmera e o scroll BG2 por scanline.
As duas alturas artificiais estão identificadas no manifesto. A faixa do
horizonte pode repetir nas alturas observadas; isso também é visível nas
screenshots do original. O port ainda não reproduz vento HDMA e chuva.

`wall_open.csv.gz` acrescenta um chute sem o teto que truncava `wall_jump`.
No quadro 340, a Samus já girando é reposicionada em (629,720), com velocidade
vertical zerada. O terreno não é alterado. O limite lateral observado é 1,375
px/quadro; não usar essa única amostra como prova de todos os wall jumps.

`land_shoot_right/left` cobrem a lacuna E6/E7: após PREP, salto normal e tiro
segurado até depois do pouso; à esquerda há uma virada e espera antes do salto.
Ambos mostram sete ticks de pouso (quadros 0×5 e 1×2). O importador preserva
hashes da ROM, state e script efetivamente usados; o script tinha alterações
locais na captura. Posições e tempos são leituras de RAM por quadro. Não cobrem
mira não frontal, outros equipamentos ou todas as formas de chegada ao chão.
`check` gera `tests/out/sm_land_shoot.h` diretamente dessas duas amostras.
