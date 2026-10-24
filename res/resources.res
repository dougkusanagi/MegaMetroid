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
