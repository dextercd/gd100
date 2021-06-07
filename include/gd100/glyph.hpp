#ifndef GDTERM_GLYPH_HPP
#define GDTERM_GLYPH_HPP

#include <cstdint>

#include "bit_container.hpp"
#include "position.hpp"

namespace gd100 {

enum class glyph_attr_bit {
    text_wraps = 1 << 0,
    wide       = 1 << 1,
    wdummy     = 1 << 2, // space occupied by previous wide character
    reversed   = 1 << 3,
    bold       = 1 << 4,
};

class glyph_attribute : public bit_container<glyph_attr_bit> {
    using bit_container::bit_container;
};

struct colour {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
};

inline std::uint32_t to_u32(colour c)
{
    auto result = std::uint32_t{};
    result |= static_cast<std::uint32_t>(c.r)  << 24;
    result |= static_cast<std::uint32_t>(c.g)  << 16;
    result |= static_cast<std::uint32_t>(c.b)  << 8;
    result |= static_cast<std::uint32_t>(0xff) << 0;

    return result;
}

using code_point = std::uint32_t;

struct extend {
    int width;
    int height;
};

struct glyph_style {
    colour fg;
    colour bg;
    glyph_attribute mode;
};

struct glyph {
    glyph_style style;
    code_point code;
};

} // gd100::

#endif // header guard
