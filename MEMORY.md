# Memória do projeto

Fatos operacionais duráveis que o código não mostra. Uma linha por fato.

- XGM2 (SGDK 2.11) tem só 3 canais PCM (CH1–CH3); `SOUND_PCM_CH4` é do XGM antigo e corrompe o driver Z80, congelando o BlastEm conforme o tempo do quadro.
- Travamentos do smoke dependentes de tempo: teste com um laço de atraso no `render` (10, 40, 300 iterações) antes de culpar a mudança atual.
- O BlastEm usa a porta GDB fixa 1234; o LM Studio também usa 1234 por padrão. O smoke recusa rodar com a porta ocupada.
- O stub GDB do BlastEm não atende interrupção assíncrona (``); para achar onde o jogo está, use breakpoint em `_VINT` (0x26c) com `ignore N`.
- Não há paleta livre: PAL0/PAL1 são da sala, PAL2 da Samus, PAL3 do tiro com os tons da nave do Landing Site nos índices 10–15.
- Nesta máquina, Python vem de `uv` em `.venv` (Pillow, numpy); ponha `.venv\Scripts` no PATH antes de `scripts/test.ps1`, `build.ps1` e `smoke_test.py`.
- O SGDK não tem `memcmp`/libc completa (`-fno-builtin`); compare buffers à mão.
