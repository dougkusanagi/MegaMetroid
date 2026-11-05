# Samus: rows are animations of 64x64 frames, built by scripts/build_player_sheet.py
# (row order in inc/player_anim.h). Palette: Power Suit, original SNES colors.
PALETTE		pal_samus_power_suit	"images/samus-original-sgdk/palettes/pal_samus_power_suit.png"
SPRITE		player_sprite		"images/player/samus_sheet.png"		8			8			FAST		5		NONE
# SPRITE		player_sprite		"images/samus_defaul_suit.png"				5			6			FAST		5		BOX
# SPRITE		player_sprite		"images/sonic.png"								6			6			FAST		5
# SPRITE		player_sprite		"images/penitent_full_anim.png"		10		10		NONE		5

# Sound effects from Super Metroid (res/sound/Sound-Effects); rescomp converts them to 13.3 kHz PCM.
SPRITE beam_sprite "images/player/beam.png" 1 1 FAST 0 NONE
SPRITE beam_impact_sprite "images/player/beam_impact.png" 2 2 FAST 0 NONE
# Missile frames 0-7, super frames 8-15, eight directions each, one 12-colour
# palette. Built by scripts/build_missile_sheet.py from the original sheets.
SPRITE missile_sprite "images/player/missile.png" 2 2 FAST 0 NONE
# Door caps: frames 0 shut, 1-3 opening, 4 open jamb. Built by scripts/build_door_cap.py
# from doors_door_r00 frames 0-4, columns 16..31, in the Landing Site PAL1 door colors.
SPRITE door_cap "images/doors/door_cap.png" 4 8 FAST 0 NONE
# Zoomer: original strip remapped to the Landing Site PAL1 background palette by
# scripts/build_zoomer_sprite.py; entry 15 is the reserved enemy highlight.
SPRITE zoomer_sprite "images/enemies/zoomer.png" 3 3 FAST 8 NONE
# Skree: original strip remapped to the same PAL1 background palette by
# scripts/build_skree_sprite.py; frames 0-3 hang, 4-7 dive.
SPRITE skree_sprite "images/enemies/skree.png" 4 4 FAST 8 NONE
# Drops (scripts/build_drop_sprite.py): the energy orb is the original purple small health pickup
# animated through its four frames in the room PAL0; the missile orb (frame 0) is still provisional.
SPRITE drop_energy_sprite "images/objects/drops_energy.png" 1 1 FAST 0 NONE
SPRITE drop_sprite "images/objects/drops.png" 1 1 FAST 0 NONE
# Ripper: original strip remapped to the same PAL1 background palette by
# scripts/build_ripper_sprite.py; its three frames loop as A, B, A, C.
SPRITE ripper_sprite "images/enemies/ripper.png" 3 2 FAST 8 NONE
# Geemer: purple Geemer/Zoomer variant remapped to the same PAL1 background palette
# by scripts/build_geemer_sprite.py; its three frames loop as a walk cycle.
SPRITE geemer_sprite "images/enemies/geemer.png" 3 4 FAST 8 NONE
# Geemer wall strip: the floor frames rotated 90 degrees by the same script, so the legs
# point at the wall. The other wall is served by the horizontal flip.
SPRITE geemer_wall_sprite "images/enemies/geemer_wall.png" 4 3 FAST 8 NONE
WAV				beam_sfx				"sound/Sound-Effects/beam_shot.wav"							XGM2
WAV				missile_sfx				"sound/Sound-Effects/ammo_missile_shot.wav"					XGM2
WAV				super_sfx				"sound/Sound-Effects/ammo_supermissile_shot.wav"			XGM2
WAV				select_sfx				"sound/Sound-Effects/ammo_select.wav"							XGM2
WAV				spin_sfx				"sound/Sound-Effects/samus_flip.wav"						XGM2
WAV				land_sfx				"sound/Sound-Effects/samus_land.wav"						XGM2
# Music as PCM on XGM2 channel 1: the intro plays once, then the loop body repeats.
# Converted from res/sound/musics/*.mp3 by scripts/convert_music.py.
WAV				music_crateria_arrival_intro	"sound/music/music_crateria_arrival_intro.wav"	XGM2
WAV				music_crateria_arrival_loop		"sound/music/music_crateria_arrival_loop.wav"	XGM2
# Status bar (7.1): tiles in the Power Suit palette (PAL2) and SGDK's font with an opaque
# background, built by scripts/build_hud.py. NONE keeps every tile in order (inc/core/hud.h).
TILESET hud_tiles "images/hud/hud_tiles.png" NONE NONE
TILESET hud_font "images/hud/hud_font.png" NONE NONE
