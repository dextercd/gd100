#ifndef GDTERM_TERMINAL_HPP
#define GDTERM_TERMINAL_HPP

#include <cstdint>

#include "bit_container.hpp"
#include "terminal_screen.hpp"

enum cursor_state_bit {
    wrap_next = 1 << 0,
};

class cursor_state : public bit_container<cursor_state_bit> {};

struct terminal_cursor {
    glyph_style style;
    cursor_state state;
    position pos;
};

class terminal {
private:
    terminal_cursor cursor{};
    terminal_screen screen{};

public:
    terminal() = default;
    terminal(extend screen_size)
        : screen{screen_size}
    {
    }

    void newline(int column=0);
    void write_char(code_point ch);
    void move_cursor_forward(int width);
    void set_char(code_point ch, glyph_style style, position pos);
    glyph* glyph_at_cursor();
    void mark_dirty(int line);
    void dump();
};

#endif // header guard
