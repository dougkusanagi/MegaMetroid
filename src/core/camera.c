#include "core/world.h"
static int clamp(int v, int low, int high)
{
    if (high < low) high = low;
    return v < low ? low : v > high ? high : v;
}
MmCamera mm_camera(int x, int y, int width, int height, MmRect bounds)
{
    MmCamera c;
    c.x = clamp(x - width / 2, bounds.left, bounds.right - width);
    c.y = clamp(y - height / 2, bounds.top, bounds.bottom - height);
    return c;
}
MmRect mm_camera_bounds(int x, int y, MmRect fallback, const MmCameraZone *zones, int count)
{
    int i;
    for (i = 0; i < count; ++i) {
        MmRect r = zones[i].trigger;
        if (x >= r.left && x < r.right && y >= r.top && y < r.bottom) return zones[i].bounds;
    }
    return fallback;
}
