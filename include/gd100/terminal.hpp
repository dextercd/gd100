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
public:
    terminal_cursor cursor{};
    terminal_screen screen{};

public:
    terminal() = default;
    terminal(extend screen_size)
        : screen{screen_size}
    {
    }

    void newline(bool first_column);
    void write_char(code_point ch);
    void move_cursor(position pos);
    void move_cursor_forward(int width);
    void set_char(code_point ch, glyph_style style, position pos);
    glyph* glyph_at_cursor();
    void mark_dirty(int line);
    void mark_dirty(int line_beg, int line_end);
    void scroll_up(int keep_top=0, int down=1);
    void clear_lines(int line_beg, int line_end);
    void dump();

private:
    position clamp_pos(position p);
};

#endif // header guard
