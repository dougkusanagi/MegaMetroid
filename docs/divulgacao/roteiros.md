# Divulgação — roteiros e postagens

Roteiros falados em inglês (como o canal). Notas em português.
Formato: vídeo longo de atualizações → cortes de 20–45 s para Shorts, TikTok e X.

## Já publicado

### Vídeo 1 — "I'm Porting Super Metroid to the Mega Drive (Wall Jump, Camera & More)"

- Vídeo: https://www.youtube.com/watch?v=EHHBxe-qnpw
- Shorts: btMtJfg0_fE, YIBCr7kxrC4, JhemaCMFOHY, y_cuO8tz_UY
- X: https://x.com/DougLopesReal/status/2111561883069116554

Roteiro (reconstituído a partir do resumo do vídeo):

| Tempo | Bloco | Conteúdo |
|---|---|---|
| 00:00 | Abertura | Retomei o porte depois de muito tempo; o desenvolvimento recente usa IA como apoio. |
| 00:14 | Corrida | Andar e correr com o botão mapeado (B no controle Xbox). |
| 00:33 | Tiro | Samus dispara projéteis. |
| 00:47 | Áudio | Música de fundo e sons de pulo, tiro e pouso. |
| 01:06 | Viradas | Sprites de virada para os dois lados; o braço muda conforme o lado do canhão. |
| 01:29 | Câmera | A câmera não mostra mais áreas fora do mapa no pulo alto. |
| 02:05 | Wall jump | Pulo na parede para alcançar plataformas altas. |

## Próximos vídeos

Conteúdo tirado do `CHANGELOG.md` e do `git log` desde 18/10/2026. Antes de gravar,
confirme na ROM atual (`out/rom.bin`) que cada cena funciona como descrito.

### Vídeo 2 — "Super Metroid on Mega Drive: Real Physics, Doors & Missiles"

Tema: deixar de "parecer" Super Metroid e passar a "se mover" como ele.

1. **Gancho (0–10 s)** — Lado a lado SNES × Mega Drive no mesmo pulo.
   "Last time Samus could run, shoot and wall jump. This time I made her move
   like the original — frame by frame."
2. **Física medida (≈60 s)** — "I recorded the original game's jump, gravity and
   running speeds and used those numbers on the Mega Drive. Jumping on slopes now
   keeps your speed, and crouch, morph ball and landings line up with the original.
   Each pose also uses the original hitbox size."
   Mostre: pulo, rampa, agachar → bola com dois toques para baixo.
3. **Power Beam (≈30 s)** — "The beam now has the original speed and
   acceleration, and the shots have impact animations."
4. **Mira em seis botões (≈30 s)** — "With a six-button pad you can aim
   diagonally, standing or crouched — like the shoulder buttons on the SNES."
5. **Landing Site (≈40 s)** — nave, arte do Landing Site, portas.
6. **Portas e transição de sala (≈60 s)** — "Shoot the door, it opens with the
   original animation timing, and only then can Samus go through. The screen
   fades and the next room loads, using a map of Crateria's rooms. Rooms that
   don't fit in video memory are refused at build time instead of crashing."
7. **Mísseis e super mísseis (≈40 s)** — troca de arma, som de troca.
8. **Fecho** — "Next: enemies. Subscribe to follow the port."

Cortes:
- **2a** "I measured Super Metroid's jump frame by frame" (comparação lado a lado).
- **2b** "Doors on the Mega Drive" — atirar, abrir, transição.
- **2c** "Missiles on a Mega Drive" — troca e disparo.
- **2d** "Diagonal aim with a 6-button pad".

### Vídeo 3 — "Super Metroid on Mega Drive: Enemies, Energy & HUD"

1. **Gancho** — Samus leva dano de um Geemer e pisca.
   "Crateria isn't empty anymore."
2. **HUD (≈30 s)** — "The status bar shows energy, missiles and super missiles."
3. **Energia e dano (≈30 s)** — dano, invulnerabilidade com piscar.
4. **Inimigos (≈90 s)** — um por vez, com o comportamento:
   - Zoomer e Geemer: andam pelas bordas das plataformas.
   - Skree: espera no teto e mergulha.
   - Ripper: voa em linha e é imune ao tiro normal (confirme antes de dizer).
   "All enemies share one fixed pool in memory — no dynamic allocation."
5. **Polimento de animação (≈40 s)** — wall jump sincronizado com o original,
   pouso atirando, moonwalk.
6. **Fecho** — próximo passo do `docs/PLANO.md`.

Cortes:
- **3a** "4 enemies in 60 seconds".
- **3b** "Moonwalk on the Mega Drive" (curto, bom para loop).
- **3c** "The HUD, tile by tile".

### Vídeo 4 (técnico) — "Why the Mega Drive dropped frames when Samus shot"

Bom para a comunidade de homebrew/SGDK.

1. **Gancho** — "Shooting while spin-jumping made the game drop frames. The cause
   was division."
2. **Problema** — "The 68000 has no fast 32-bit divide. Each shot traced the
   cannon with two multiplications and two divisions per step — about 150 of the
   frame's scanlines in one shot."
3. **Medição** — "I time code with the VDP's line counter and count missed
   VBlanks in BlastEm over 300 frames."
4. **Correção** — "Now the trace is incremental, with no division: ~150 → ~35
   lines. The HUD only redraws when its values change."
5. **Resultado** — Landing Site 30 → 2 missed frames, Crateria 16 → 0.
   "Measured in BlastEm NTSC, not yet on real hardware." (manter esta ressalva)
6. **Fecho** — "Dev tips for SGDK in the description."

Cortes:
- **4a** "The Mega Drive hates division" (30 s).
- **4b** "How I measure lag on a Mega Drive".

## Texto base para posts

X / legenda (ajuste por corte):

> Super Metroid on the Mega Drive — [feature]. Fan port in C with SGDK, physics
> measured from the original. Full video: [link]
> #MegaDrive #Genesis #SGDK #homebrew #SuperMetroid

Cuidados:
- Não divulgue nem envie a ROM: o porte usa recursos extraídos do jogo original.
  Mostre só vídeo.
- Mantenha a menção ao uso de IA; parte da comunidade valoriza a transparência.
- Use "fan port" / "porte não oficial".

## Onde divulgar

- YouTube (vídeo + Shorts), TikTok, X: um corte por dia, com link do vídeo longo.
- Reddit: r/SEGAGENESIS, r/megadrive, r/retrogaming, r/Metroid. Leia as regras de
  cada um sobre autopromoção e fan games antes de postar; um post por vídeo longo.
- Discord do SGDK e fórum SpritesMind: o Vídeo 4 (técnico) é o mais adequado.
- Bluesky e Mastodon (hashtags #SGDK #MegaDrive), onde há devs de homebrew.

## Agenda sugerida (por vídeo)

| Dia | Postagem |
|---|---|
| D0 | Vídeo longo no YouTube + post no X com o melhor corte |
| D1–D4 | Um corte por dia em Shorts, TikTok e X |
| D2 | Reddit (um subreddit por dia, sem repetir o mesmo texto) |
| D5 | Comunidades técnicas (se houver conteúdo técnico) |
