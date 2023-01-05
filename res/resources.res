SPRITE		player_sprite		"images/samus_defaul_suit.png"   	5			6			FAST		5		BOX
# SPRITE		player_sprite		"images/sonic.png"								6			6			FAST		5
# SPRITE		player_sprite		"images/penitent_full_anim.png"		10		10		NONE		5

IMAGE			level_image			"images/maps/crateria/tilemap1-smaller.png"			BEST
IMAGE			bg_image				"images/maps/crateria/bg1.png"					BEST

PALETTE		level_palette		"images/maps/crateria/tilemap1-smaller.png"
PALETTE		bg_palette			"images/maps/crateria/bg1.png"

TILESET		level_tileset		"images/maps/crateria/tilemap1-smaller.png"			BEST
TILESET		bg_tileset			"images/maps/crateria/bg1.png"					BEST

MAP				level_map				"images/maps/crateria/tilemap1-smaller.png"			level_tileset		BEST
MAP				bg_map					"images/maps/crateria/bg1.png"					bg_tileset			BEST

WAV				jump_sfx				"sound/jump.wav"												XGM
