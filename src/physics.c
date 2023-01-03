#include "../inc/physics.h"

s16 pixelToTile(s16 position) {
	return position >> 3;
}

s16 tileToPixel(s16 tile) {
	return tile << 3;
}
