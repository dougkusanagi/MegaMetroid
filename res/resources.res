# Samus: rows are animations of 64x64 frames, built by scripts/build_player_sheet.py
# (row order in inc/player_anim.h). Palette: Power Suit, original SNES colors.
PALETTE		pal_samus_power_suit	"images/samus-original-sgdk/palettes/pal_samus_power_suit.png"
SPRITE		player_sprite		"images/player/samus_sheet.png"		8			8			FAST		5		NONE
# SPRITE		player_sprite		"images/samus_defaul_suit.png"				5			6			FAST		5		BOX
# SPRITE		player_sprite		"images/sonic.png"								6			6			FAST		5
# SPRITE		player_sprite		"images/penitent_full_anim.png"		10		10		NONE		5

# Sound effects from Super Metroid (res/sound/Sound-Effects); rescomp converts them to 13.3 kHz PCM.
WAV				beam_sfx				"sound/Sound-Effects/beam_shot.wav"							XGM2
WAV				spin_sfx				"sound/Sound-Effects/samus_flip.wav"						XGM2
WAV				land_sfx				"sound/Sound-Effects/samus_land.wav"						XGM2
