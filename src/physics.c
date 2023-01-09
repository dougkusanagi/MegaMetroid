#include "../inc/physics.h"

s16 pixelToTile(s16 position) {
	return position >> 3;
}

s16 tileToPixel(s16 tile) {
	return tile << 3;
}

//Used to get the left edge of a tile by inputting the tile position
u16 getTileLeftEdge(u16 x) {
	return (x << 3);
}
//Used to get the right edge of a tile by inputting the tile position
u16 getTileRightEdge(u16 x) {
	return (x << 3) + 8;
}
//Used to get the top edge of a tile by inputting the tile position
u16 getTileTopEdge(u16 y) {
	return (y << 3);
}
//Used to get the bottom edge of a tile by inputting the tile position
u16 getTileBottomEdge(u16 y) {
	return (y << 3) + 8;
}
//Used to get the bounds of a tile by inputting the tile position
AABB getTileBounds(s16 x, s16 y) {
	return newAABB((x << 3), (x << 3) + 8, (y << 3), (y << 3) + 8);
}

//Used to get the tile position out of a pixel position
Vect2D_u16 posToTile(Vect2D_s16 position) {
	return newVector2D_u16((position.x >> 3), (position.y >> 3));
}
