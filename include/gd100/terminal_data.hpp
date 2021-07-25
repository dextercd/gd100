#ifndef GDTERM_TERMINAL_DATA_HPP
#define GDTERM_TERMINAL_DATA_HPP

namespace gd100 {

enum cursor_state_bit {
    wrap_next = 1 << 0,
};

class cursor_state : public bit_container<cursor_state_bit> {
    using bit_container::bit_container;
};

constexpr auto default_style = glyph_style{{255, 255, 255}, {0, 0, 0}, glyph_attribute{}};

struct terminal_cursor {
    glyph_style style = default_style;
    cursor_state state;
    position pos;
};

enum class terminal_mode_bit {
    insert = 1 << 0,
    extended_mouse = 1 << 1,
    bracketed_paste = 1 << 2,
};

enum class mouse_mode {
    none,
    x10,
    button,
    motion,
    many,
};

class terminal_mode : public bit_container<terminal_mode_bit> {
    using bit_container::bit_container;
};

enum class charset {
    usa,
    graphic0,
};

} // gd100::

#endif // header guard
