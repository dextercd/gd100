#include <iterator>
#include <algorithm>

namespace cw {

namespace {

struct code_point_range {
    char32_t begin;
    char32_t end;
};

code_point_range const double_width_ranges[] {

#include <double_width_table.inc>

};

code_point_range const zero_width_ranges[] {

#include <zero_width_table.inc>

};


// Misc zero width table
// from https://github.com/jquast/wcwidth
char32_t const zero_width_misc[]{
    // 0, Handled separately
    0x034f, // Combining grapheme joiner (Mn)
    0x200b, // Zero width space
    0x200c, // Zero width non-joiner
    0x200d, // Zero width joiner
    0x200e, // Left-to-right mark
    0x200f, // Right-to-left mark
    0x2028, // Line separator (Zl)
    0x2029, // Paragraph separator (Zp)
    0x202a, // Left-to-right embedding
    0x202b, // Right-to-left embedding
    0x202c, // Pop directional formatting
    0x202d, // Left-to-right override
    0x202e, // Right-to-left override
    0x2060, // Word joiner
    0x2061, // Function application
    0x2062, // Invisible times
    0x2063, // Invisible separator
};

} // anonymous namespace

int character_width(char32_t code)
{
    // Handle ASCII stuff seperately for better performance

    if (code == 0)
        return 0;

    // c0 c1 control codes
    if ((code >= 0x00 && code <= 0x1f) || (code >= 0x80 && code <= 0x9f))
        return -1;

    if (code < 0x80)
        return 1;

    // Handle the rest of Unicode

    auto const zero_width_misc_it = std::find(
        std::begin(zero_width_misc),
        std::end(zero_width_misc),
        code);

    if (zero_width_misc_it != std::end(zero_width_misc))
        return 0;

    auto const zero_width_it = std::lower_bound(
        std::begin(zero_width_ranges),
        std::end(zero_width_ranges),
        code,
        [](auto const range, auto const code) { return range.end < code; });

    if (zero_width_it != std::end(zero_width_ranges)) {
        if (code >= zero_width_it->begin)
            return 0;
    }

    auto const double_width_it = std::lower_bound(
        std::begin(double_width_ranges),
        std::end(double_width_ranges),
        code,
        [](auto const range, auto const code) { return range.end < code; });

    if (double_width_it != std::end(double_width_ranges)) {
        if (code >= double_width_it->begin)
            return 2;
    }

    return 1;
}

} // cw::
