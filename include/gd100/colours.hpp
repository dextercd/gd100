#ifndef GDTERM_COLORS_HPP
#define GDTERM_COLORS_HPP

#include "glyph.hpp"

namespace gd100 {

constexpr colour sgr_colours[16]{
    {30, 30, 30},    // black
    {177, 22, 22},   // red
    {42, 160, 38},   // green
    {198, 195, 13},  // yellow
    {18, 99, 170},   // blue
    {157, 55, 168},  // magenta
    {15, 180, 188},  // cyan
    {200, 200, 200}, // white
    {127, 127, 127}, // bright black
    {248, 24, 24},   // bright red
    {65, 205, 65},   // bright green
    {233, 237, 18},  // bright yellow
    {34, 123, 201},  // bright blue
    {177, 77, 188},  // bright magenta
    {13, 211, 221},  // bright cyan
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
