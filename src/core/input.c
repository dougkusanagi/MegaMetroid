#include "core/world.h"
MmInput mm_input(MmInputState *s, unsigned short buttons,
                 unsigned short left, unsigned short right, unsigned short jump)
{
    MmInput result = {0};
    result.axis = ((buttons & right) != 0) - ((buttons & left) != 0);
    result.jump_pressed = (buttons & jump) && !(s->previous & jump);
    s->previous = buttons;
    return result;
}
