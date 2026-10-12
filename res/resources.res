# Sprite sheets are 480x192 (12x4 grid of 40x48 frames). rescomp cannot read
# 32bpp RGB images, so the Dread sheet has an 8bpp indexed twin: same 16 colors,
# same pixels, magenta kept at index 0.
SPRITE		player_sprite		"images/samus_mega_metroid_dread_idx.png"   	5			6			FAST		5		BOX
# SPRITE		player_sprite		"images/samus_defaul_suit.png"				5			6			FAST		5		BOX
# SPRITE		player_sprite		"images/sonic.png"								6			6			FAST		5
# SPRITE		player_sprite		"images/penitent_full_anim.png"		10		10		NONE		5

WAV				jump_sfx				"sound/jump.wav"												XGM2
