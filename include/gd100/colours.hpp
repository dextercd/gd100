#ifndef GDTERM_COLORS_HPP
#define GDTERM_COLORS_HPP

#include "glyph.hpp"

namespace gd100 {

constexpr colour sgr_colours[16]{
    {0, 0, 0},       // black
    {177, 22, 22},   // red
    {17, 154, 18},   // green
    {198, 195, 13},  // yellow
    {12, 8, 140},    // blue
    {117, 24, 145},  // magenta
    {37, 106, 119},  // cyan
    {200, 200, 200}, // white
    {127, 127, 127}, // bright black
    {248, 24, 24},   // bright red
    {18, 237, 18},   // bright green
    {233, 237, 18},  // bright yellow
    {52, 22, 247},   // bright blue
    {207, 23, 214},  // bright magenta
    {23, 214, 207},  // bright cyan
    {255, 255, 255}  // bright white
};

constexpr colour eight_bit_lookup(int index)
{
    if (index < 16)
        return sgr_colours[index];

    if (index < 232) {
        constexpr std::uint8_t colour_steps[] {
            0x00, 0x5f, 0x87, 0xaf, 0xd7, 0xff
        };

        auto const cube_ix = index - 16;

        auto const r =  cube_ix / 36;
        auto const gb = cube_ix % 36;
        auto const g = gb / 6;
        auto const b = gb % 6;

        return {colour_steps[r], colour_steps[g], colour_steps[b]};
    }

    if (index < 256) {
        auto grey_ix = index - 232;
        auto grey = static_cast<std::uint8_t>(8 + 10 * grey_ix);
        return {grey, grey, grey};
    }

    return {0, 0, 0};
}

} // gd100::

#endif // header guard
