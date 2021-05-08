#ifndef GDTERM_TERMINAL_DATA_HPP
#define GDTERM_TERMINAL_DATA_HPP

namespace gd100 {

enum cursor_state_bit {
    wrap_next = 1 << 0,
};

class cursor_state : public bit_container<cursor_state_bit> {
    using bit_container::bit_container;
};

struct terminal_cursor {
    glyph_style style;
    cursor_state state;
    position pos;
};

enum class terminal_mode_bit {
    insert = 1 << 0,
};

class terminal_mode : public bit_container<terminal_mode_bit> {
    using bit_container::bit_container;
};

} // gd100::

#endif // header guard