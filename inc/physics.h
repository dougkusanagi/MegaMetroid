#include <genesis.h>

#include "types.h"

s16 pixelToTile(s16 position);
s16 tileToPixel(s16 tile);

u16 getTileLeftEdge(u16 x);
u16 getTileRightEdge(u16 x);
u16 getTileTopEdge(u16 y);
u16 getTileBottomEdge(u16 y);
AABB getTileBounds(s16 x, s16 y);
Vect2D_u16 posToTile(Vect2D_s16 position);