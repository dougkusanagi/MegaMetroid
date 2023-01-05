SPRITE		player_sprite		"images/samus_defaul_suit.png"   	5			6			FAST		5		BOX
# SPRITE		player_sprite		"images/sonic.png"								6			6			FAST		5
# SPRITE		player_sprite		"images/penitent_full_anim.png"		10		10		NONE		5

IMAGE			level_image			"images/level-original.png"			BEST
IMAGE			bg_image				"images/bg.png"									BEST

PALETTE		level_palette		"images/level-original.png"
PALETTE		bg_palette			"images/bg.png"

TILESET		level_tileset		"images/level-original.png"			BEST
TILESET		bg_tileset			"images/bg.png"									BEST

MAP				level_map				"images/level-original.png"			level_tileset		BEST
MAP				bg_map					"images/bg.png"									bg_tileset			BEST

WAV				jump_sfx				"sound/jump.wav"								XGM
