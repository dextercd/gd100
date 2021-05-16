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
};

class glyph_attribute : public bit_container<glyph_attr_bit> {
    using bit_container::bit_container;
};

struct colour {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
};

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
