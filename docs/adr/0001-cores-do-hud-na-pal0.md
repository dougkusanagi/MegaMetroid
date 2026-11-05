# 0001. Cores do HUD na PAL0

- Status: Aceito
- Data: 2026-11-04

## Contexto

O HUD em PAL2 herdava fundo roxo e letras creme/verde da Samus. Os quatro bancos
do VDP já são usados. Inspeção dos PNGs encontrou duplicatas sem pixels no
Landing Site e entradas livres na Crateria 2; a reindexação conserva todas as
cores do terreno na precisão do hardware.

## Decisão

Reservar PAL0 1/2/3/4/8/13 para preto, azul, branco e três tons rosa.
Geradores e testes mantêm o contrato, incluindo hashes RGB/32 das duas salas.

## Alternativas

- Manter PAL2: preserva simplicidade, mas mantém as diferenças visuais relatadas.
- Trocar paleta por interrupção horizontal: permite um banco próprio por faixa,
  mas acrescenta sincronização de raster desnecessária enquanto existem duplicatas.
- Quantizar novamente o terreno: poderia mudar arte já validada, sem necessidade.

## Consequências

O HUD ganha cores estáveis sem alterar Samus, tiros ou aparência das salas,
sem interrupções novas e sem um quinto banco. Novas salas devem reservar essas
entradas. A reserva ocorre depois da quantização para não mudar a arte existente.
Implementado e revisado: cores efetivas e transparência preservadas nas duas salas.
