#ifndef GDTERM_GLYPH_HPP
#define GDTERM_GLYPH_HPP

#include "bit_container.hpp"
#include "position.hpp"

enum class glyph_attr_bit {
    text_wraps = 1 << 0,
    wide       = 1 << 1,
};

class glyph_attribute : public bit_container<glyph_attr_bit> {};

class color {};

using code_point = std::uint32_t;

struct extend {
    int width;
    int height;
};

struct glyph_style {
    color fg;
    color bg;
    glyph_attribute mode;
};

struct glyph {
    glyph_style style;
    code_point code;
};

#endif // header guard
