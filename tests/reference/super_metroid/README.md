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
Os testes do porte continuam rodando no BlastEm pela API, conforme `AGENTS.md`.
