# Memória do projeto

Fatos operacionais duráveis que o código não mostra. Uma linha por fato.

- XGM2 (SGDK 2.11) tem só 3 canais PCM (CH1–CH3); `SOUND_PCM_CH4` é do XGM antigo e corrompe o driver Z80, congelando o BlastEm conforme o tempo do quadro.
- Travamentos do smoke dependentes de tempo: teste com um laço de atraso no `render` (10, 40, 300 iterações) antes de culpar a mudança atual.
- O BlastEm usa a porta GDB fixa 1234; o LM Studio também usa 1234 por padrão. O smoke recusa rodar com a porta ocupada.
- O stub GDB do BlastEm não atende interrupção assíncrona (``); para achar onde o jogo está, use breakpoint em `_VINT` (0x26c) com `ignore N`.
- Não há banco de paleta livre: PAL0/PAL1 são da sala, PAL2 da Samus e PAL3 do tiro/nave. O HUD reserva PAL0 1/2/3/4/8/13 reaproveitando duplicatas/entradas sem uso; novas salas precisam preservar esse contrato.
- Nesta máquina, Python vem de `uv` em `.venv` (Pillow, numpy); ponha `.venv\Scripts` no PATH antes de `scripts/test.ps1`, `build.ps1` e `smoke_test.py`.
- O SGDK não tem `memcmp`/libc completa (`-fno-builtin`); compare buffers à mão.
- No 68000, `*`, `/` e `%` de `int` (32 bits) por valor variável chamam a libgcc (~1.000 ciclos, ~2 linhas de varredura cada): evite-os em laços por quadro; o traçado do canhão custava ~8 linhas por passo por isso.
- Para cronometrar trechos, leia `GET_VCOUNTER` dentro da área visível (linhas 0–223, sem VInt no meio); o contador repete 0xE5–0xEA no VBlank, então não meça através dele. Um quadro NTSC tem 262 linhas (~488 ciclos cada).
- SGDK quebrou em 2026-11-05 (`Couldn't reserve space for cygwin's heap`, 0x60EA0000): as DLLs de `D:/apps/sgdk/bin` foram rebaseadas para 0x10000000 com `C:/msys64/usr/bin/rebase.exe -4` (backup `msys-1.0.dll.orig`), e o make antigo roda com `SHELL=C:/msys64/usr/bin/bash.exe` (o `sh` antigo segfaulta no fork sob ASLR moderno); pad ROM à mão com `sizebnd.jar` + `finalize_rom.py`.
